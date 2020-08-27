#include "KeyCalculator.h"

#define CHANGE_SIGN_OPERATOR    (uint8_t('`'))
#define BACKSPACE_OPERATOR      (uint8_t('B'))
#define MAX_ARR_MEM_TO_SHOW     8
#define CALC_NUMERIC_PRECISION  8

#define DEBUG_KEYCALC_STATE     0   // If non-zero, spew all state changes
#define DEBUG_KEYCALC_STACK     0   // If non-zero, spew all enter()
#define DEBUG_KEYCALC_MEMORY    0   // If non-zero, spew all memory operations


////////////////////////////////////////////////////////////////////////////////
//
//  Constructor
//
KeyCalculator::KeyCalculator() : TextCalculator(CALC_NUMERIC_PRECISION) {
  _state              = calcReadyForAny;
  _num_buffer_index   = 0;
  _mem_buffer_index   = 0;
  _clear_press_count  = 0;
}


////////////////////////////////////////////////////////////////////////////////
//
//  This is the workhorse routine for handling incoming keys from the keyboard.
//  Keys build values, memory specs, or represent commands.
//  _state controls how the key is handled, and is modified by key inputs.
//
bool KeyCalculator::key(uint8_t code) {
  if(DEBUG_KEYCALC_STATE) Serial.printf("Entering key() in %s state.\n", _state_to_name[_state]);

  // First, set error state if calculator has encountered exception.
  if(_calc.get_error_state()) _change_state(calcError);

  // If in error state, no keys are accepted except AC, which clears the error state
  // as well as all memory and stacks.
  if(calcError == _state) {
    if(CLEAR_OPERATOR == code) {
      _calc.clear_error_state();
      _calc.push_value(0.0);
      _change_state(calcReadyForAny);
      return true;
    }
    return false;
  }

  // See if we just pressed the AC key two times
  if(_clear_press_count && CLEAR_OPERATOR == code) {
    return _handle_clear(true);
  }
  _clear_press_count = 0;

  // Next, see if we are entering a multi-key memory command
  if(calcEnteringMemory == _state) {
    return _handle_memory_command(code);
  }

  // Special case for chaining mode:
  // On most calculators, if you press 1 + = + = + = you get 2, 4, 8, ...
  // Only do this for +, -, *, /, not %, square, etc.
  //
  if(EVALUATE_OPERATOR == code) {
    if(calcReadyForNumber == _state) {
      Op_ID op = _calc.peek_operator();
      if(ADDITION_OPERATOR      == op || SUBTRACTION_OPERATOR == op ||
        MULTIPLICATION_OPERATOR == op || DIVISION_OPERATOR    == op) {
        if(DEBUG_KEYCALC_STACK) Serial.printf("Pushing %.4f onto the value stack in key() (chaining mode)\n", _calc.get_value());
        enter(value());
        _change_state(calcReadyForOperator);  // So the '=' code is evaluated
      }
    }
  }

  // If we are in calcReadyForNumber state, and there's an operator on the stack, we must be waiting for
  // the second operator. If another operator comes in here, it should replace the operator on top of
  // the operator_stack. For example:
  // User inputs 15 + *.  Interpret this to mean: "I meant *, not +".
  // Don't do this for parens, you may want multiple open parens and close parens get weird.
  if((calcReadyForNumber == _state) && is_operator(code) && (OPEN_PAREN_OPERATOR != code) &&
     (CLOSE_PAREN_OPERATOR != code) && (0 != _calc.operator_stack.size())) {
    _calc.operator_stack.back() = code;
    return true;  // Do not change the state
  }

  // Handle numbers. This may include 0-9, decimal, B for backspace and the open paren
  if(calcReadyForAny == _state || calcReadyForNumber == _state || calcEnteringNumber == _state) {
    // Next, see if a number is being entered (with B for Backspace)
    if(('0' <= code && '9' >= code) || ('.' == code) || 'B' == code) {
      return _build_number(code);
    }
    if(OPEN_PAREN_OPERATOR == code) {
      enter(code);
      _change_state(calcReadyForNumber);
      return true;
    }
  }
  commit(); // If input is in progress, commit pushes it to the stack and sets state to calcReadyForOperator. Else, noop.

  // See if its a simple operator
  if((calcReadyForAny == _state && 0 < _calc.value_stack.size()) || calcReadyForOperator == _state) {
    if(is_operator(code)) {
      // Do not allow a CLOSE_PAREN_OPERATOR on the stack unless there's a matching OPEN_PAREN_OPERATOR
      if(CLOSE_PAREN_OPERATOR == code && 0 == _count_open_parens()) {
        if(DEBUG_KEYCALC_STACK) Serial.println("Rejecting close paren because there is no matching open paren.");
        return false;
      }
      if(DEBUG_KEYCALC_STACK) Serial.printf("Pushing %c onto the operator stack in key()\n", code);
      bool result = enter(code);
      if(result) {
        CalcState                        next = calcReadyForNumber;     // Normally, after entering an operator
        if(EVALUATE_OPERATOR    == code) next = calcReadyForAny;        // Operation completed, ready for anything
        if(CLOSE_PAREN_OPERATOR == code) next = calcReadyForOperator;   // Operation completed, ready for anything
        // If the operator was %, square or SquareRoot, evaluate it immediately (like most calculators do) and set state to any
        // (Basically, unary operators are evaluated immediately)
        if(PERCENT_OPERATOR == code || SQUARE_OPERATOR == code || SQUARE_ROOT_OPERATOR == code) {
          result = (NO_ERROR == _calc.evaluate_one());
          next   = calcReadyForAny;
        }
        _change_state(next);
      }
      return result;
    }
  }

  // These operations require more complex handling, and are permitted in any state
  switch(code) {
    case CHANGE_SIGN_OPERATOR:  return _handle_change_sign();
    case CLEAR_OPERATOR:        return _handle_clear(false);
    case MEMORY_OPERATOR:       return _handle_memory_command(code);
    default:                    break;
  }
  if(DEBUG_KEYCALC_STATE) Serial.printf("Dropping key %c\n", code);
  return false;
}


////////////////////////////////////////////////////////////////////////////////
//
//  If there is a number being input in the _num_buffer, push it to the stack
//  and clear the _num_buffer.
//  Return true if a value was pushed, false if nothing happened.
//
bool KeyCalculator::commit() {
  if(_num_buffer_index) {
    String str = _convert_num_buffer(true);
    if(DEBUG_KEYCALC_STACK) Serial.printf("Pushing %s onto the value stack in commit()\n", str.c_str());
    enter(str);
    _change_state(calcReadyForOperator);
    return true;
  }
  return false;
}


////////////////////////////////////////////////////////////////////////////////
//
//  Push val onto the value stack.
//
void KeyCalculator::set_value(String val) {
  _num_buffer_index = 0;                // so get_display goes to value() and not to buffer
  _num_buffer[0]    = 0;                // so it's not scanned even though index is zero
  if(DEBUG_KEYCALC_STACK) Serial.printf("Pushing %s onto the value stack in set_value()\n", val.c_str());
  enter(val);                           // push the value onto the stack
  _change_state(calcReadyForAny);       // ready for any after a push
}


////////////////////////////////////////////////////////////////////////////////
//
//  When inputing a number or memory, dump buffer and set state
//
void KeyCalculator::cancel_input() {
  if(calcEnteringNumber == _state) {
    _num_buffer[0]    = '\0';
    _num_buffer_index = 0;
    _change_state((0 < _calc.operator_stack.size()) ? calcReadyForNumber : calcReadyForAny);
  }
  else if(calcEnteringMemory == _state) {
    _mem_buffer[0]    = '\0';
    _mem_buffer_index = 0;
    _change_state(calcReadyForAny);
  }
}


////////////////////////////////////////////////////////////////////////////////
//
//  Return the KeyCalculator's current state
//
CalcState KeyCalculator::get_state() {
  return _state;
}


////////////////////////////////////////////////////////////////////////////////
//
//  Get some textual information for display, identified by id
//
String KeyCalculator::get_display(CalcDisplay id) {
  String str;
  switch(id) {
    case dispValue:     // If we're in number input mode, return the current _num_buffer. Else, return the value
      if(_num_buffer_index) {
        return _convert_num_buffer(false);
      }
      return value();

    case dispMemoryID:  // Get the memory address that's being built up if _entering_memory
      str = "M[";
      switch(_mem_buffer_index) {
        case 0:
          str += "__";
          break;
        case 1:
          str += "_";
          str += _mem_buffer;
          break;
        case 2:
          str += _mem_buffer;
          break;
        default: str += "?";  // error state
      }
      str += "]  (";
      str += double_to_string(_mem_buffer_index ? _calc.get_memory(atoi(_mem_buffer)) : _calc.get_memory());
      str += ")";
      return str;

    case dispStatus:          // Get a status display showing open parens and memory usage
      return _build_status_display();

    case dispOpStack:         // Get a representation of the operator stack
      str = "[ ";
      for(int i = 0; i < _calc.operator_stack.size(); i++) {
        str += char(_calc.operator_stack[i]);
        str += " ";
      }
      str += "]";
      return str;

    case dispValStack:        // Get a representation of the value stack
      str = "[ ";
      for(int i = 0; i < _calc.value_stack.size(); i++) {
        str += double_to_string(_calc.value_stack[i]);
        str += " ";
      }
      str += "]";
      return str;

    default:
      return String("Unexpected");
  }
}


////////////////////////////////////////////////////////////////////////////////
//
//  Always call this function to change _state; don't do it directly.
//  This permits logging, etc.
//
void KeyCalculator::_change_state(CalcState state) {
  if(DEBUG_KEYCALC_STATE) Serial.printf("Changing state from %s to %s\n", _state_to_name[_state], _state_to_name[state]);
  _state = state;
}


////////////////////////////////////////////////////////////////////////////////
//
//  Handle the AC key, with 1st & 2nd press actions
//
bool KeyCalculator::_handle_clear(bool all_clear) {
  _clear_press_count++;
  if(!_clear_press_count) _clear_press_count = 255; // Just an 8 bit #, don't let it roll over

  if(all_clear) {
    clear_all();
    _change_state(calcReadyForAny);
    return true;
  }
  else {
    // If the op stack is empty, or is open paren, we can enter anything.
    // Otherwise, we need a number
    bool any = (0 == _calc.operator_stack.size()) || (OPEN_PAREN_OPERATOR == _calc.operator_stack.back());
    _change_state(any ? calcReadyForAny : calcReadyForNumber);
    return (NO_ERROR == _calc.clear());
  }
  return false;
}


////////////////////////////////////////////////////////////////////////////////
//
//  Handle +/- key (Change Sign), which is an input action, not a command.
//
bool KeyCalculator::_handle_change_sign() {
  _change_state(calcReadyForNumber);
  if(0 < _calc.value_stack.size()) {
    if(0.0 != _calc.get_value()) {
      double val = _calc.pop_value();
      _calc.push_value(-val);
      return true;
    }
  }
  return false;
}


////////////////////////////////////////////////////////////////////////////////
//
//  Memory commands are complicated. There is a simple memory, a memory array, and a memory stack.
//  The memory array size is established with the MemoryCalculator template parameter M and can be
//  ascertained with _calc.get_mem_array_size().
//  Once M is pressed, it can be followed with another M for recall, a memory operator (+-*/%=)
//  to store the value in the simple memory, or (0-9) to build a memory address for the operator:
//  MM   Recall simple memory      (M -> Value)
//  M=   Store simple memory       (Value -> M)
//  MA   Clear simple memory       (0 -> M)
//  M+   Simple memory opperation  (M + Value -> M)
//  M0=  Store into M[0]           (Value -> M[0])
//  M9=  Store into M[9]           (Value -> M[9])
//  M.   Cancel out of memory mode (N/C)
//  Note that there may be 10 or thousands of memories.
//  The previous state is captured on entry and restored on exit from calcEnteringMemory
//
bool KeyCalculator::_handle_memory_command(uint8_t code) {
  if(DEBUG_KEYCALC_MEMORY) Serial.printf("_handle_memory_command('%c')\n", code);
  if(calcEnteringMemory != _state) {
    if(DEBUG_KEYCALC_MEMORY) Serial.println("Setting calcEnteringMemory state");
    assert(MEMORY_OPERATOR == code);
    _change_state(calcEnteringMemory);
    return true;
  }
  else {
    // If it's a '.', cancel calcEnteringMemory and return true
    if('.' == code) {
      _mem_buffer_index = 0;
      _mem_buffer[0]    = '\0';
      _change_state(calcReadyForAny);
      return true;
    }
    // If it's a backspace, reduce the memory name buffer by one
    if('B' == code) {
      if(_mem_buffer_index) {
        _mem_buffer_index--;
        _mem_buffer[_mem_buffer_index] = '\0';
        return true;
      }
      return false;
    }
    // Build memory name if it's a number
    if('0' <= code && '9' >= code) {
      // Make sure the number doesn't exceed the number of memories:
      _mem_buffer[_mem_buffer_index++] = code;
      _mem_buffer[_mem_buffer_index]   = '\0';
      if(NUM_CALC_MEMORIES - 1 < atoi(_mem_buffer)) {
        // roll back
        if(DEBUG_KEYCALC_MEMORY) Serial.printf("Error setting memory address: '%s' would be too large.\n", _mem_buffer);
        _mem_buffer[--_mem_buffer_index] = '\0';
        return false;
      }
      if(DEBUG_KEYCALC_MEMORY) Serial.printf("Building memory address: '%s'\n", _mem_buffer);
      return true;
    }

    // If it's a second 'M', we want to recall memory from the given location
    if(MEMORY_OPERATOR == code) {
      bool result = false;
      if(0 == _mem_buffer_index) {
        if(DEBUG_KEYCALC_MEMORY) Serial.println("Recalling simple memory");
        result = recall_memory();
      }
      else {
        uint8_t index = atoi(_mem_buffer);
        if(DEBUG_KEYCALC_MEMORY) Serial.printf("Recalling memory M[%d]\n", index);
        result = recall_memory(index);
      }
      _mem_buffer_index               = 0;
      _mem_buffer[_mem_buffer_index]  = '\0';
      _change_state(calcReadyForAny);
      return result;
    }

    // If it's a memory operation, call the memory_operation and terminate _entering_memory
    if(is_mem_operator(code)) {
      if(0 == _mem_buffer_index) {
        if(DEBUG_KEYCALC_MEMORY) Serial.printf("call memory_operation(Op_ID = '%c')\n\n", code);
        _calc.memory_operation(code);
      }
      else {
        if(DEBUG_KEYCALC_MEMORY) Serial.printf("call memory_operation: Op_ID = '%c', index = '%s'\n\n", code, _mem_buffer);
        _calc.memory_operation(code, atoi(_mem_buffer));
      }
      _mem_buffer_index               = 0;
      _mem_buffer[_mem_buffer_index]  = '\0';
     _change_state(calcReadyForAny);
      return true;
    }
  }
  // Some random command? Bail out of memory mode.
  _mem_buffer_index = 0;
  _mem_buffer[0]    = '\0';
  _change_state(calcReadyForAny);
  return false;
}


////////////////////////////////////////////////////////////////////////////////
//
//  Construct a string for a calculator to display showing status.
//  It may be entirely empty, or as complex as:  (( M[1,2,4,6,7,18.37,81,...]  S(5)  M=3.14159265
//
String KeyCalculator::_build_status_display() {
  String  str         = "";
  uint8_t paren_count = _count_open_parens();
  uint8_t arr_count   = 0;
  uint8_t stack_count = _calc.get_memory_depth();
  double  mem         = _calc.get_memory();

  // At least during development, show the KeyCalculator state first:
  switch(_state) {
    case calcReadyForAny      : str += "A "; break;
    case calcReadyForNumber   : str += "N "; break;
    case calcReadyForOperator : str += "O "; break;
    case calcEnteringNumber   : str += "> "; break;
    case calcEnteringMemory   : str += "M "; break;
    case calcError            : str += "X "; break;
  }

  // Show how many open parens there are on the stack (if any).
  // This part of the status string leads and looks like: (((
  if(paren_count) {
    for(int i = 0; i < paren_count; i++) str += "(";
    str += " ";
  }

  // Next, display info about indexed memories. Show the indexes of up to
  // eight; if there are more add ...
  // This part of the status string looks like: M[0,1,2,3]
  for(uint8_t i = 0; i < NUM_CALC_MEMORIES; i++) {
    if(0.0 != _calc.get_memory(i)) {
      if(0 == arr_count++) str += "M[";
      if(MAX_ARR_MEM_TO_SHOW > arr_count) {
        str += i;
        str += ',';
      }
      else {
        str += "...,";
        break;
      }
    }
  }
  if(arr_count) {
    str[str.length() - 1] = ']';
    str += "  ";
  }

  // Next, display info about the memory stack. If it is non-empty, add
  // S(n), where n is the number of items on the stack.
  // This part of the status string looks like: S(2)
  if(stack_count) {
    str += "S(";
    str += stack_count;
    str += ")  ";
  }

  // Finally, if simple memory is set, display its value.
  // This part of the status string looks like: M=3.14159265
  if(0.0 != mem) {
    str += "M=";
    str += double_to_string(mem);
  }
  return str;
}


////////////////////////////////////////////////////////////////////////////////
//
//  Build the display value from keystrokes
//
bool KeyCalculator::_build_number(uint8_t code) {
  _change_state(calcEnteringNumber);
  if('B' == code) {
    // backspace one space
    if(_num_buffer_index) {
      _num_buffer_index--;
      _num_buffer[_num_buffer_index] = '\0';
      return true;
    }
    return false;
  }
  if(KEYCAL_NUM_BUFFER_SIZE <= _num_buffer_index - 1) return false;
  // One decimal point per number
  if('.' == code) {
    if(nullptr != strchr(_num_buffer, '.')) {
      return false;
    }
  }
  _num_buffer[_num_buffer_index++] = code;
  _num_buffer[_num_buffer_index]   = '\0';
  if(DEBUG_KEYCALC_MEMORY) Serial.printf("_build_number: %s\n", _num_buffer);
  return true;
}


////////////////////////////////////////////////////////////////////////////////
//
//  Convert the buffer to a String and optionally clear it
//
String KeyCalculator::_convert_num_buffer(bool clear) {
  String str(_num_buffer);
  if(clear) {
    _num_buffer_index = 0;
    _num_buffer[0]    = '\0';
  }
  return str;
}


////////////////////////////////////////////////////////////////////////////////
//
//  Return the number of OPEN_PAREN operators on the operator_stack,
//  minus the number of close parens.
//
uint8_t KeyCalculator::_count_open_parens() {
  uint8_t open_count  = 0;
  uint8_t close_count = 0;
  for(int i = 0; i < _calc.operator_stack.size(); i++) {
    if(OPEN_PAREN_OPERATOR == _calc.operator_stack[i]) open_count++;
  }
  for(int i = 0; i < _calc.operator_stack.size(); i++) {
    if(CLOSE_PAREN_OPERATOR == _calc.operator_stack[i]) close_count++;
  }
  return open_count - close_count;
}

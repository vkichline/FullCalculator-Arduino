#include "TextCalculator.h"

#define DEBUG_PARSING   0
#define DEBUG_PURGE     0


TextCalculator::TextCalculator(uint8_t precision) {
  _precision  = precision;
  _ops        = { ADDITION_OPERATOR, SUBTRACTION_OPERATOR, MULTIPLICATION_OPERATOR, DIVISION_OPERATOR,
                  OPEN_PAREN_OPERATOR, CLOSE_PAREN_OPERATOR, EVALUATE_OPERATOR, PERCENT_OPERATOR,
                  SQUARE_OPERATOR, SQUARE_ROOT_OPERATOR };
  _mem_ops    = { ADDITION_OPERATOR, SUBTRACTION_OPERATOR, MULTIPLICATION_OPERATOR, DIVISION_OPERATOR,
                  EVALUATE_OPERATOR, PERCENT_OPERATOR, MEMORY_OPERATOR, CLEAR_OPERATOR };
  _nums       = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '.' };
  _wspace     = { ' ', '\t', '\n', '\r' };
  enter("0");   // Start with an empty value on the stack.
}


// Parse a string and return true if no errors encountered.
// Issue: This approach doesn't handle unary +/-
//
bool TextCalculator::parse(const char* statement) {
  int   index = 0;
  char  buffer[64];
  if(DEBUG_PARSING) Serial.printf("\nDebugging parse(%s)\n", statement);
  while(index < strlen(statement)) {
    char c = statement[index++];
    // Skip over whitespace
    if(!is_wspace(c)) {
      if(is_operator(c)) {
        if(DEBUG_PARSING) Serial.printf("Pushing operator %c\n", c);
        if(!enter(c)) {
          if(DEBUG_PARSING) Serial.println("Error pushing operator! Returning false\n");
          return false;
        }
      }
      if(is_numeric(c)) {
        int bi = 1;
        buffer[0] = c;
        if(DEBUG_PARSING) Serial.println("Parsing number");
        while(is_numeric(statement[index])) {
          buffer[bi++] = statement[index++];
        }
        buffer[bi] = '\0';
        if(DEBUG_PARSING) Serial.printf("Pushing value %s\n", buffer);
        if(!enter(buffer)) {
          if(DEBUG_PARSING) Serial.println("Error pushing value! Returning false\n");
          return false;
        }
      }
    }
  }
  if(DEBUG_PARSING) Serial.println("Parse complete");
  return true;
}


// String overload for the parse command
//
String TextCalculator::parse(String statement) {
  if(parse(statement.c_str())) {
    return value();
  }
  else {
    return String("Error");
  }
}


// It's not obvious, but if the operator_stack is empty, the value_stack should be cleared.
// Otherwise, 1+1= 1+1= 1+1= results in a useless value_stack containing [2 2 2]
//
bool TextCalculator::enter(const char* value) {
  if(0 == _calc.operator_stack.size()) {
    if(DEBUG_PURGE) Serial.printf("Clearing value stack before entering %s\n", value);
    _calc.value_stack.clear();
  }
  double val = _string_to_double(value);
  return NO_ERROR == _calc.push_value(val);
}


// String overload for enter value
//
bool TextCalculator::enter(String value) {
  return enter(value.c_str());
}


// Pushing the Open Paren operator is handled like pushing a value, because the paren will evaluate
// to a value. Clear the value stack in this case if the operator stack is empty, or we get values
// piled up which will never be evaluated.
//
bool TextCalculator::enter(Op_ID id) {
  if((OPEN_PAREN_OPERATOR == id) && (0 == _calc.operator_stack.size())) _calc.value_stack.clear();
  return (NO_ERROR == _calc.push_operator(id));
}


// Evaluate all operations (like pushing '=')
//
Op_Err TextCalculator::total() {
  return _calc.evaluate_all();
}


// Returns the current value from the top of the value stack as a String
String TextCalculator::value() {
  return double_to_string(_calc.get_value());
}


// Replace (do not push) current value
//
void TextCalculator::set_value(const char* value) {
  _calc.value_stack.back() = _string_to_double(value);
}


// Replace (do not push) current value
//
void TextCalculator::set_value(String value) {
  set_value(value.c_str());
}


// Copy current value() to M
//
void TextCalculator::copy_to_memory() {
  _calc.set_memory(_calc.get_value());
}


// Copy current value() to M[index]
//
bool TextCalculator::copy_to_memory(uint8_t index) {
  return NO_ERROR == _calc.set_memory(index, _calc.get_value());
}


// Push a copy of the current value() onto the memory stack
//
void TextCalculator::push() {
  return _calc.push_memory(_calc.get_value());
}


// Replace value() with M
//
bool TextCalculator::recall_memory() {
  _calc.value_stack.back() = _calc.get_memory();
  return true;
}


// Replace value() with M[index]
//
bool TextCalculator::recall_memory(uint8_t index) {
  if(NUM_CALC_MEMORIES - 1 <= index) return false;
  _calc.value_stack.back() = _calc.get_memory(index);
  return true;
}


// Pop a value off the memory stack and replace value() with it
//
void TextCalculator::pop() {
  _calc.value_stack.back() = _calc.pop_memory();
}


// Clear only M
//
void TextCalculator::clear_memory() {
  _calc.set_memory(0.0);
}


// Clear M, all M[], and the memory stack, plus op and value stack
//
void TextCalculator::clear_all() {
  _calc.clear_all_memory();
  _calc.operator_stack.clear();
  _calc.value_stack.clear();
  _calc.value_stack.push_back(0.0);
}


// Return true if id is in _ops
//
bool TextCalculator::is_operator(Op_ID id) {
  return (_ops.find(Op_ID(id)) != _ops.end());
}


// Return true if id is in _mem_ops
//
bool TextCalculator::is_mem_operator(Op_ID id) {
  return (_mem_ops.find(Op_ID(id)) != _mem_ops.end());
}


// Return true if c is in _nums
//
bool TextCalculator::is_numeric(char c) {
  return (_nums.find(Op_ID(c)) != _nums.end());
}


// Return true if c is in _wspace
//
bool TextCalculator::is_wspace(char c) {
  return (_wspace.find(Op_ID(c)) != _wspace.end());
}


// Get the current calculator global error state
//
Op_Err TextCalculator::get_error_state() {
  return _calc.get_error_state();
}


// Clear the calculator global error state
//
void TextCalculator::clear_error_state() {
  _calc.clear_error_state();
}


// Convert the value to a string, with no trailing decimal point, with specified precision.
// This algorithm only works for positive numbers, so for negative numbers
// insert a - in the buffer and invert val
//
String TextCalculator::double_to_string(double val) {
  if(0.0 == val) return String("0");
  double  threshold   = 1.0 / pow(10.0, _precision);  // Precision is a private member variable
  int     m           = log10(ceil(abs(val)));
  char    buffer[64]  = {0};
  int     digit       = 0;
  char*   p           = buffer;

  if(0.0 > val) {
    val    = -val;
    *(p++) = '-';
  }
  while((0 <= m) || (val > threshold)) {
    double weight = pow(10.0, m);
    digit = floor(val / weight);
    val  -= (digit * weight);
    *(p++)= '0' + digit;
    if(m == 0) *(p++) = '.';
    *p    = '\0';
    m--;
  }
  // if the last character is a '.', delete it
  if('.' == *(p-1)) *(p-1) = '\0';

  // Numbers like 9.9 produce the string 09.9. Also -09.9.
  // reset p to beginning of buffer
  p = buffer;
  if(3 <= strlen(buffer) && '-' == buffer[0] && '0' == buffer[1] && '.' != buffer[2]) {
    // We have -0N.NN Eliminate the second character of the buffer.
    p++;      // Advance the pointer from - to 0
    *p = '-'; // Replace the 0 with a -
  }
  else if(2 <= strlen(buffer) && '0' == buffer[0] && '.' != buffer[1]) {
    p++;      // We have 0N or 0NN.NNN Eliminate the first character of the buffer.
  }
  return String(p);
}


double TextCalculator::_string_to_double(const char* val) {
  return atof(val);     // trivial implementation; will use precision and base later
}

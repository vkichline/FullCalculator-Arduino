#pragma once

// This is a template for a no-frills, core calculator engine that handles any numeric type.
// It uses the shunting-yard algorithm to process infix notification, so operations can be pushed as
// one would enter them on an ordinary calculator: Push 1, Push +, Push 1, Eval, the stack contains 2.
// This implementation features plug-in operators so that the set of provided operators can easily be extended.
//
// By Van Kichline
// In the year of the plague


#include <vector>
#include <map>
#include <Arduino.h>


#define OP_ID_NONE                 0                            // Result of popping an empty operator_stack
#define NO_ERROR                   0                            // Good result from a calculation
#define ERROR_TOO_FEW_OPERANDS    -1                            // Calculation error: not enough operands on the value_stack
#define ERROR_UNKNOWN_OPERATOR    -2                            // Calculation error: the operator is not recognized
#define ERROR_DIVIDE_BY_ZERO      -3                            // Calculation error: divide by zero
#define ERROR_SET_NOERROR         -4                            // You cannot set the error state to NONE, you must use clear_error_state()
#define ERROR_NO_MATCHING_PAREN   -5                            // Evaluated more close parens than open parens
#define ERROR_OVERFLOW            -6

#define DEBUG_OPERATORS           0                             // 0 is quiet, 1 spews info for debugging operators
#define DEBUG_EVALUATION          0                             // 0 is quiet, 1 spews info for debugging evaluations

#define ADDITION_OPERATOR         (uint8_t('+'))
#define SUBTRACTION_OPERATOR      (uint8_t('-'))
#define MULTIPLICATION_OPERATOR   (uint8_t('*'))
#define DIVISION_OPERATOR         (uint8_t('/'))
#define PERCENT_OPERATOR          (uint8_t('%'))
#define OPEN_PAREN_OPERATOR       (uint8_t('('))
#define CLOSE_PAREN_OPERATOR      (uint8_t(')'))
#define SQUARE_OPERATOR           (uint8_t('s'))
#define SQUARE_ROOT_OPERATOR      (uint8_t('r'))
#define EVALUATE_OPERATOR         (uint8_t('='))                // This is a special operator that is not implemented as a class or added to _operators


template<typename T> class        Operator;                     // Forward declaration to operator template
typedef int16_t                   Op_Err;                       // Signed error; 0 is no error, errors are normally negative. See #define ERROR_*
typedef uint16_t                  Op_ID;                        // Operators are normally identified by a single char like '+'. More may be needed.


// Tempate for the lowest level of the calculator engine, which manages evaluation of the operator_stack and value_stack.
// Using the shunting-yard algorithm, values and operators can be pushed in the order of ordinary infix notation (1 + 1).
//
template <typename T>
class CoreCalculator {
  public:
    CoreCalculator();
    Op_Err                        push_value(T value);          // Push a value onto the value_stack
    T                             pop_value();                  // Return the top value of the value_stack after removing it from the stack
    T                             peek_value();                 // Return the top value of the value_stack without changing the stack
    Op_Err                        push_operator(Op_ID id);      // Push an operator onto the operator_stack (may cause evaluation of higher precedence items on stack)
    Op_ID                         pop_operator();               // Return the top value of the operator_stack after removing it from the stack (OP_ID_NONE if empty)
    Op_ID                         peek_operator();              // Return the top value of the operator_stack without changing the stack (OP_ID_NONE if empty)
    Op_Err                        evaluate_one();               // Evaluate the top operator on the operator_stack (if any)
    Op_Err                        evaluate_all();               // Evaluate the operator_stack until its empty
    T                             get_value();                  // Top of the operand_stack, or 0.0 if stack is empty
    Op_Err                        clear();                      // Change value to zero
    Op_Err                        set_error_state(Op_Err err);  // Set the global error state. Return the previous error state. (Cannot be set to NO_ERROR)
    Op_Err                        get_error_state();            // Return the global error state
    void                          clear_error_state();          // Set _error_state to NO_ERROR and clear operator and value stacks.
    std::vector<T>                value_stack;                  // Pushdown stack for values. Unfortunately, <stack> is broken
    std::vector<Op_ID>            operator_stack;               // pushdown stack for operators
  protected:
    void                          _initialize_operators();      // Fill the _operators map with instances of available Operators
    std::map<Op_ID, Operator<T>*> _operators;                   // A map of all the operators (extensible!)
    Op_Err                        _error_state;                 // Global error state.
public:
    void                          _spew_stacks();               // Writes operator_stack and value_stack to Serial port for debugging
};


// Base class for all other operators. This approach makes operators extensible.
// Operator precedence: highest numbers are highest precedence. Values are unsigned 8 bits.
// Grouping   250   - pairs of (), [], etc.
// + -        200   - unary + and - prefixes (not currently used)
// exp root   150   - exponents and roots
// * / MOD    100   - multiplication, division and modulus
// + -         50   - addition and subtraction
// Evaluate     0   - always evaluate (not an operator; handled in push_operator)
//
// The Operator must pop the operands, but not the operator.
//
template <typename T>
class Operator {
  public:
    Operator(CoreCalculator<T>* host) : _host(host) {}
    Op_ID             id                = OP_ID_NONE; // Is often a character, like '+' (43)
    uint8_t           precedence        = 0;          // Determines order of evaluation
    virtual bool      enough_values()   = 0;          // Override to determine if there are enough operands on the _operand_stack
    virtual Op_Err    operate()         = 0;          // Override to do what the operator does
  protected:
    CoreCalculator<T>*  _host;                        // for calling CoreCalculator<T> functions from the operator
};


////////////////////////////////////////////////////////////////////////////////
//
//  Operators Implementation
//
////////////////////////////////////////////////////////////////////////////////

// Convenience class to make binary operators (n OP n) trivial to implement.
// Usage:
// In your derived template's constructor:
//  BinaryOperator<T>::set_id_and_precedence('-', 100);
// In your derived template's operate():
//  BinaryOperator<T>::err = BinaryOperator<T>::prepare();
//  return BinaryOperator<T>::push_result(BinaryOperator<T>::op2 [BINARY_OPERATOR] BinaryOperator<T>::op1);
//
template <typename T>
class BinaryOperator: public Operator<T> {
  public:
    BinaryOperator(CoreCalculator<T>* host) : Operator<T>(host) {}
    void set_id_and_precedence(Op_ID id, uint8_t precedence) {
      this->id          = id;
      this->precedence  = precedence;
    }
    bool enough_values() {
      return (2 <= Operator<T>::_host->value_stack.size());
    }
    // Call this function to set up member variables before evaluation
    Op_Err prepare() {
      if(enough_values()) {
        op1 = Operator<T>::_host->pop_value();
        op2 = Operator<T>::_host->pop_value();
        return NO_ERROR;
      }
      return ERROR_TOO_FEW_OPERANDS;
    }
    // This expects err to be set by the call to prepare. Pattern is:
    Op_Err push_result(double result) {
      if(NO_ERROR == err) {
        return Operator<T>::_host->push_value(result);
      }
      return err;
    }
  protected:
    double  op1     = 0.0;  // The top operand from the value_stack
    double  op2     = 0.0;  // The second opeand from the value_stack
    Op_Err  err;            // Shared error
};

template <typename T>
class AdditionOperator : public BinaryOperator<T> {
  public:
    AdditionOperator(CoreCalculator<T>* host) : BinaryOperator<T>(host) {
      BinaryOperator<T>::set_id_and_precedence(ADDITION_OPERATOR, 50);
    }
    Op_Err operate() {
      BinaryOperator<T>::err = BinaryOperator<T>::prepare();
      return BinaryOperator<T>::push_result(BinaryOperator<T>::op2 + BinaryOperator<T>::op1);
    }
};

template <typename T>
class SubtractionOperator : public BinaryOperator<T> {
  public:
    SubtractionOperator(CoreCalculator<T>* host) : BinaryOperator<T>(host) {
      BinaryOperator<T>::set_id_and_precedence(SUBTRACTION_OPERATOR, 50);
    }
    Op_Err operate() {
      BinaryOperator<T>::err = BinaryOperator<T>::prepare();
      return BinaryOperator<T>::push_result(BinaryOperator<T>::op2 - BinaryOperator<T>::op1);
    }
};

template <typename T>
class MultiplicationOperator : public BinaryOperator<T> {
  public:
    MultiplicationOperator(CoreCalculator<T>* host) : BinaryOperator<T>(host) {
      BinaryOperator<T>::set_id_and_precedence(MULTIPLICATION_OPERATOR, 100);
    }
    Op_Err operate() {
      BinaryOperator<T>::err = BinaryOperator<T>::prepare();
      return BinaryOperator<T>::push_result(BinaryOperator<T>::op2 * BinaryOperator<T>::op1);
    }
};

template <typename T>
class DivisionOperator : public BinaryOperator<T> {
  public:
    DivisionOperator(CoreCalculator<T>* host) : BinaryOperator<T>(host) {
      BinaryOperator<T>::set_id_and_precedence(DIVISION_OPERATOR, 100);
    }
    Op_Err operate() {
      BinaryOperator<T>::err = BinaryOperator<T>::prepare();
      if(T(0) == BinaryOperator<T>::op1) return ERROR_DIVIDE_BY_ZERO;  // Check for divide by zero before proceeding
      return BinaryOperator<T>::push_result(BinaryOperator<T>::op2 / BinaryOperator<T>::op1);
    }
};

// Beginning of grouping does nothing, just puts a token on the stack
template <typename T>
class OpenParenOperator : public Operator<T> {
  public:
    OpenParenOperator(CoreCalculator<T>* host) : Operator<T>(host) {
      Operator<T>::id          = OPEN_PAREN_OPERATOR;
      Operator<T>::precedence  = 250;
    }
    bool    enough_values() { return true;     }
    Op_Err  operate()       { return NO_ERROR; }
};

// End of grouping: keep evaluating until an open paren if found.
// Pop the open paren operator, the result is already on the value_stack.
//
template <typename T>
class CloseParenOperator : public Operator<T> {
  public:
    CloseParenOperator(CoreCalculator<T>* host) : Operator<T>(host) {
      Operator<T>::id          = CLOSE_PAREN_OPERATOR;
      Operator<T>::precedence  = 250;
    }
    bool    enough_values() { return true; }
    Op_Err  operate() {
      while(OPEN_PAREN_OPERATOR != Operator<T>::_host->peek_operator()) {
        Op_Err err = Operator<T>::_host->evaluate_one();
        if(err) return err;
        // It's not an error to evaluate an empty operator_stack, but it means
        // there is no matching OPEN_PAREN and we'd be stuck here forever.
        if(0 == Operator<T>::_host->operator_stack.size())
          return ERROR_NO_MATCHING_PAREN;
      }
      Operator<T>::_host->pop_operator();
      return NO_ERROR;
    }
};

// The calculator % operator is unusual and more complicated than most.
// It doesn't really make sense if T is an integer.
// If no operator on stack, divide Value by T(100)
// In other words, 30 %  ==>  30 / T(100)
// If +, -, * or / on stack, +-*/ value % from stack
// In other words, 30 + 5 %  ==>  30 + 30 * 5 / T(100)
//
template <typename T>
class PercentOperator : public Operator<T> {
  public:
    PercentOperator(CoreCalculator<T>* host) : Operator<T>(host) {
      Operator<T>::id          = PERCENT_OPERATOR;
      Operator<T>::precedence  = 100;
    }
    bool    enough_values() { return (1 <= Operator<T>::_host->value_stack.size()); }
    Op_Err  operate() {
      // If the perator stack is empty, or has an open paren on top...
      if(0 == Operator<T>::_host->operator_stack.size() || OPEN_PAREN_OPERATOR == Operator<T>::_host->operator_stack.back()) {
        Operator<T>::_host->push_operator('/');
        Operator<T>::_host->push_value(T(100));
        Operator<T>::_host->evaluate_one();
      }
      else {
        // BUGBUG: HOW SHOULD 30 / 6 % = ACT?
        T temp = Operator<T>::_host->pop_value();
        Operator<T>::_host->push_value(Operator<T>::_host->get_value());
        Operator<T>::_host->push_operator('*');
        Operator<T>::_host->push_value(temp);
        Operator<T>::_host->push_operator('/');
        Operator<T>::_host->push_value(T(100));
        Operator<T>::_host->evaluate_one();
        Operator<T>::_host->evaluate_one();
      }
      return NO_ERROR;
    }
};

// Square the number on the stack
//
template <typename T>
class SquareOperator : public Operator<T> {
  public:
    SquareOperator(CoreCalculator<T>* host) : Operator<T>(host) {
      Operator<T>::id          = SQUARE_OPERATOR;
      Operator<T>::precedence  = 150;
    }
    bool    enough_values() { return (1 <= Operator<T>::_host->value_stack.size()); }
    Op_Err  operate() {
      if(!enough_values()) return ERROR_TOO_FEW_OPERANDS;
      T temp = Operator<T>::_host->pop_value();
      temp = temp * temp;
      Operator<T>::_host->push_value(temp);
      return NO_ERROR;
    }
};

// Calculate the square root of the number on the stack
//
template <typename T>
class SquareRootOperator : public Operator<T> {
  public:
    SquareRootOperator(CoreCalculator<T>* host) : Operator<T>(host) {
      Operator<T>::id          = SQUARE_ROOT_OPERATOR;
      Operator<T>::precedence  = 150;
    }
    bool    enough_values() { return (1 <= Operator<T>::_host->value_stack.size()); }
    Op_Err  operate() {
      if(!enough_values())  return ERROR_TOO_FEW_OPERANDS;
      T temp = Operator<T>::_host->pop_value();
      temp = T(sqrt(double(temp)));
      Operator<T>::_host->push_value(temp);
      return NO_ERROR;
    }
};

////////////////////////////////////////////////////////////////////////////////
//
//  CoreCalculator Implementation
//
////////////////////////////////////////////////////////////////////////////////

template <typename T> CoreCalculator<T>::CoreCalculator() {
  set_error_state(NO_ERROR);
  _initialize_operators();
}

// Push a value onto the stack to be processed later.
//
template <typename T> Op_Err CoreCalculator<T>::push_value(T value) {
  value_stack.push_back(value);
  return NO_ERROR;
}

// Return the top value from the stack and pop it off.
//
template <typename T> T CoreCalculator<T>::pop_value() {
  T value = value_stack.back();
  value_stack.pop_back();
  return value;
}

// Return the top value from the stack without modifying the stack.
//
template <typename T> T CoreCalculator<T>::peek_value() {
  return value_stack.back();
}

// Push an operator onto the operator_stack.
// The operator must exist in _operators.
// Shunting-yard algorithm for evaluating infix notation (ref: Dijkstra):
// If the operator on the operator_stack is the same or greater precedence
// than this operator, evaluate it first.
//
template <typename T> Op_Err CoreCalculator<T>::push_operator(Op_ID id) {
  Op_Err result = NO_ERROR;
  // The EVALUATE_OPERATOR is handled specially here. It's not in _operators
  if(EVALUATE_OPERATOR == id) {
    if(DEBUG_OPERATORS) Serial.println("\nOperator = : evaluating the entire stack\n");
    result =  evaluate_all();
    if(DEBUG_OPERATORS) Serial.printf("evaluate_all() returned %d\n", result);
    return result;
  }
  if(DEBUG_OPERATORS) Serial.printf("\nPushing operator %c\n", id);
  if(_operators.count(id)) {  // If it's a valid operator
    Operator<T>* incoming = _operators[id];
    if(DEBUG_OPERATORS) Serial.println("Operator valid");
    while(true) {
      Op_ID top = peek_operator();
      if(!_operators.count(top)) break;
      if(OPEN_PAREN_OPERATOR == top) break;   // Only the CloseParenOperator removes the OpenParenOperator
      Operator<T>* top_op = _operators[top];
      if(top_op->precedence < incoming->precedence) break;
      if(DEBUG_OPERATORS) Serial.printf("Forcing operator %c\n", top);
      result = evaluate_one();
      if(result) return result;
      if(DEBUG_OPERATORS) Serial.println("Forced operation successful\n");
    }
    operator_stack.push_back(id);
    return result;
  }
  return ERROR_UNKNOWN_OPERATOR;
}

// Return the top value of the operator_stack after popping it.
// Return OP_ID_NONE if the stack is empty.
// Note: OP_ID_NONE is not in _operators, by design.
//
template <typename T> Op_ID CoreCalculator<T>::pop_operator() {
  if(0 == operator_stack.size()) return OP_ID_NONE;
  Op_ID id = operator_stack.back();
  operator_stack.pop_back();
  return id;
}

// Return the top value of the operator_stack without changing the stack (OP_ID_NONE if empty)
//
template <typename T> Op_ID CoreCalculator<T>::peek_operator() {
  if(0 == operator_stack.size()) return OP_ID_NONE;
  Op_ID id = operator_stack.back();
  return id;
}

// Evaluate the operator_stack until its empty and there's only one operand
// (Typical response to the = key)
//
template <typename T> Op_Err CoreCalculator<T>::evaluate_all() {
  while(1 <= operator_stack.size()) {
    Op_Err err = evaluate_one();
    if(err) {
      if(DEBUG_EVALUATION) Serial.printf("evaluate returning error: %d\n", err);
      return err;
    }
  }
  return NO_ERROR;
}

// Top of the value_stack, or 0 if stack is empty
// (What would display on a calculator screen)
//
template <typename T> T CoreCalculator<T>::get_value() {
  if(0 == value_stack.size()) return T(0);
  return peek_value();
}

// Set the value to zero. Do not change size of the stack unless it's empty,
//
template <typename T> Op_Err CoreCalculator<T>::clear() {
  if(0 != value_stack.size()) value_stack.pop_back();
  value_stack.push_back(T(0));
  return NO_ERROR;
}

// Evaluate the top operator on the operator_stack (if any)
// If an error occurs, set the global error state.
//
template <typename T> Op_Err CoreCalculator<T>::evaluate_one() {
  if(DEBUG_EVALUATION) { Serial.print("\nevaluate_one: "); _spew_stacks(); }
  if(1 <= operator_stack.size()) {
    Op_ID id = pop_operator();
    if(DEBUG_EVALUATION) Serial.printf("popped operator %c\n", id);
    if(!_operators.count(id)) {
      if(DEBUG_EVALUATION) Serial.println("operator unknown; returning false");
      set_error_state(ERROR_UNKNOWN_OPERATOR);
      return ERROR_UNKNOWN_OPERATOR;
    }
    Operator<T>* op = _operators[id];
    if(!op->enough_values()) {
      if(DEBUG_EVALUATION) Serial.println("not enough operands; returning false");
      set_error_state(ERROR_TOO_FEW_OPERANDS);
      return ERROR_TOO_FEW_OPERANDS;
    }
    Op_Err result = op->operate();
    if(DEBUG_EVALUATION) Serial.printf("Result of %c operation: %d.  ", id, result);
    if(DEBUG_EVALUATION) _spew_stacks();
    if(result) set_error_state(result);
    return result;
  }
  if(DEBUG_EVALUATION) Serial.println("operator stack empty");
  return NO_ERROR;  // Nothing to do; that's not an error
}

// Set the global error state. Return the previous error state.
// You cannot set the error to NO_ERROR, you must use clear_error_state();
//
template <typename T> Op_Err CoreCalculator<T>::set_error_state(Op_Err err) {
  if(err == NO_ERROR) return ERROR_SET_NOERROR;
  Op_Err old_error_state = _error_state;
  Serial.printf("Setting Global Error State to %d\n", err);
  _error_state = err;
  return old_error_state;
}

// Return the global error state
//
template <typename T> Op_Err CoreCalculator<T>::get_error_state() {
  return _error_state;
}

// Clear the global error state, empty the operator and value stack.
// (Make it safe to use the calculator again.)
//
template <typename T> void CoreCalculator<T>::clear_error_state() {
  _error_state = NO_ERROR;
  value_stack.clear();
  operator_stack.clear();
}

// Place all the operators we plan to use in the _operators vector.
// This design makes it trivial to add additional operators.
// Note: OP_ID_NONE and EVALUATE_OPERATOR is not put in _operators, by design.
//
template <typename T> void CoreCalculator<T>::_initialize_operators() {
  _operators.insert(std::pair<Op_ID, Operator<T>*>(ADDITION_OPERATOR,       new AdditionOperator<T>(this)));
  _operators.insert(std::pair<Op_ID, Operator<T>*>(SUBTRACTION_OPERATOR,    new SubtractionOperator<T>(this)));
  _operators.insert(std::pair<Op_ID, Operator<T>*>(MULTIPLICATION_OPERATOR, new MultiplicationOperator<T>(this)));
  _operators.insert(std::pair<Op_ID, Operator<T>*>(DIVISION_OPERATOR,       new DivisionOperator<T>(this)));
  _operators.insert(std::pair<Op_ID, Operator<T>*>(OPEN_PAREN_OPERATOR,     new OpenParenOperator<T>(this)));
  _operators.insert(std::pair<Op_ID, Operator<T>*>(CLOSE_PAREN_OPERATOR,    new CloseParenOperator<T>(this)));
  _operators.insert(std::pair<Op_ID, Operator<T>*>(PERCENT_OPERATOR,        new PercentOperator<T>(this)));
  _operators.insert(std::pair<Op_ID, Operator<T>*>(SQUARE_OPERATOR,         new SquareOperator<T>(this)));
  _operators.insert(std::pair<Op_ID, Operator<T>*>(SQUARE_ROOT_OPERATOR,    new SquareRootOperator<T>(this)));
}

// Writes operator_stack and value_stack to Serial for debugging
//
template <typename T> void CoreCalculator<T>::_spew_stacks() {
  String str = "Op Stack: [ ";
  if(0 == operator_stack.size()) {
    str += "EMPTY ";
  }
  else {
    for(int i = 0; i < operator_stack.size(); i++) { str += char(operator_stack[i]); str += ' '; }
  }
  str += "]  Val Stack: [ ";
  if(0 == value_stack.size()) {
    str += "EMPTY ";
  }
  else {
    for(int i = 0; i < value_stack.size(); i++) { str += value_stack[i]; str += ' '; }
  }
  str += "]";
  Serial.println(str.c_str());
}

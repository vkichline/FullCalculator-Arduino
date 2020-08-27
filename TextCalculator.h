#pragma once

// This class contains the MemoryCalculator and provides an implementation that uses
// strings to represent numeric values. It can be used for a command-line
// calculator, or further augmented to respond to individual keystrokes.
// Rather than getting a value from memory, you retrieve a value, and it
// replaces the value on the top of the value stack.
//
// By Van Kichline
// In the year of the plague


#include <set>
#include "MemoryCalculator.h"

#define NUM_CALC_MEMORIES   100
#define MEMORY_OPERATOR     (uint8_t('M'))

// enum CalcMode { Calc_Mode_FP, Calc_Mode_Integer };
// enum CalcBase { Calc_Base_Binary = 2, Calc_Base_Octal = 8, CalcBase_Decimal = 10, Calc_Base_Hexidecimal = 16 };
// enum CalcTrigMode { Calc_Trig_Mode_Degrees, Calc_Trig_Mode_Radians, Calc_Trig_Mode_Grads };


class TextCalculator {
  public:
    TextCalculator(uint8_t precision = 8);
    bool                parse(const char* statement);       // Evaluate a statement, like: "1 + 5 / 3.2 * 7.3167 - 8 * 33.33 ="
    String              parse(String statement);            // Overload for String data type
    bool                enter(const char* value);           // Push a numeric value (expressed in characters) onto the value stack
    bool                enter(String value);                // Push numeric value overload for String data type
    bool                enter(Op_ID id);                    // Enter an operator, like '+', '-', '='
    Op_Err              total();                            // Evaluate all operations (like pushing '=')
    String              value();                            // Returns the current value from the top of the value stack as a String
    void                set_value(const char* value);       // Replace (do not push) current value
    void                set_value(String value);            // String overload for set_value

    void                copy_to_memory();                   // Copy current value() to M
    bool                copy_to_memory(uint8_t index);      // Copy current value() to M[index]
    void                push();                             // Push a copy of the current value() onto the memory stack
    bool                recall_memory();                    // Replace value() with M
    bool                recall_memory(uint8_t index);       // Replace value() with M[index]
    void                pop();                              // Pop a value off the memory stack and replace value() with it
    void                clear_memory();                     // Clear only M
    void                clear_all();                        // Clear M, all M[], and the memory stack, plus op and value stack

    bool                is_operator(Op_ID id);              // Return true if id is in _ops
    bool                is_mem_operator(Op_ID id);          // Return true if id is in _mem_ops
    bool                is_numeric(char c);                 // Return true if c is in _nums
    bool                is_wspace(char c);                  // Return true if c is in _wspace

    Op_Err              get_error_state();                  // Get the current calculator global error state
    void                clear_error_state();                // Clear the calculator global error state
    String              double_to_string(double val);       // Convert to a display string, eliminating unneeded characters
    MemoryCalculator<double, NUM_CALC_MEMORIES>   _calc;    // The calculator engine embedded within
  protected:
    std::set<Op_ID>     _ops;                               // A set of all the known Op_IDs
    std::set<Op_ID>     _mem_ops;                           // A set of all the Op_IDs for memory mode
    std::set<char>      _nums;                              // A set of all numeric characters (including . but not + -)
    std::set<char>      _wspace;                            // A set of all whitespace characters
    uint8_t             _precision;                         // Precision to use in double_to_string()
    double              _string_to_double(const char* val); // Convert string to a value
};

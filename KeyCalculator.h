#pragma once

// The KeyCalculator is a state machine whose state is driven by the key() and set_value() commands.
// Its state is available from get_state() and get_display(), the later of which returns a number of
// different state descriptions defined by the CalcDisplay enumeration.
// The state machine implements a hand calculator.
// Details and documentation to come when it's all worked out.
//
// By Van Kichline
// In the year of the plague

#include "TextCalculator.h"

#define KEYCAL_NUM_BUFFER_SIZE  64
#define KEYCAL_MEM_BUFFER_SIZE   8

// KeyCalculator states, changed by key inputs, accessible by get_state()
//
enum CalcState {
  calcReadyForAny,      // A - Ready for any input; initial state
  calcReadyForNumber,   // N - The calculator is waiting for a digit, a period, +/- or an open paren
  calcReadyForOperator, // O - The calculator is waiting for +-*/%=sr
  calcEnteringNumber,   // > - The number in the display is being entered. Expect digit, period, +/-
  calcEnteringMemory,   // M - A memory command is being entered. Expect M=A+-*/%
  calcError             // X - The calculator is in Global Error mode. Only AC accepted.
};

// get_display() selectors
//
enum CalcDisplay {
  dispValue,            // Display the current input buffer if in flux, or the current value()
  dispMemoryID,         // Get the current _mem_buffer, augmented. Empty when not in calcEnteringMemory
  dispStatus,           // Get a status display showing open parens and memory usage
  dispOpStack,          // Get a representation of the operator stack
  dispValStack          // Get a representation of the value stack
};


class KeyCalculator : public TextCalculator {
  public:
    KeyCalculator();
    bool        key(uint8_t code);                                // Process a key from the keyboard
    bool        commit();                                         // If there is a value in the buffer, commit it to the stack
    void        set_value(String val);                            // Set the display value to val. Used by aggregator to input value and set state.
    void        cancel_input();                                   // When inputing a number or memory, dump buffer and return to calcReadyForAny state
    CalcState   get_state();                                      // Get the current state of the KeyCalculator
    String      get_display(CalcDisplay id);                      // Return the specified string representation

  protected:
    const char* _state_to_name[6]                       = { "calcReadyForAny", "calcReadyForNumber", "calcReadyForOperator", "calcEnteringNumber", "calcEnteringMemory", "calcError" };
    char        _num_buffer[KEYCAL_NUM_BUFFER_SIZE]     = {0};    // Buffer for building a numeric entry
    char        _mem_buffer[KEYCAL_MEM_BUFFER_SIZE]     = {0};    // Buffer for building a memory address
    uint8_t     _num_buffer_index                       =  0;     // Current position in _num_buffer
    uint8_t     _mem_buffer_index                       =  0;     // Current position in mem_buffer
    uint8_t     _clear_press_count                      =  0;     // The number of times in a row the AC key has been pressed.
    CalcState   _state;                                           // Current state of the KeyCalculator

    void      _change_state(CalcState state);                     // Always call this function to change _state; don't do it directly
    bool      _handle_clear(bool all_clear = false);              // Handle the AC key, with 1st & 2nd press actions
    bool      _handle_change_sign();                              // Handle +/- key, which is an input action, not a command
    bool      _handle_memory_command(uint8_t code);               // Handle the rather complicated memory commands
    String    _build_status_display();                            // Build the (complicated) dispStatus string
    bool      _build_number(uint8_t code);                        // Build the display value from keystrokes
    String    _convert_num_buffer(bool clear);                    // Convert the buffer to a String and clear it
    uint8_t   _count_open_parens();                               // Return the number of OPEN_PAREN operators on the operator_stack
};

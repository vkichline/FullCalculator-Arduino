// M5 Calculator
//
// This program represents the fifth and top level of the calculator, and provides a specifically M5Stack implementation.
// The structure of the code at this point is:
// CoreCalculator   template, used to generate a double-precision floating point calculator
// MemoryCalculator template, used to add 101 memories plus a memory stack
// TextCalculator,            which provides a calculator that understand textual representations of numbers
// KeyCalculator,             a TextCalculator specialized for calculator keyboard input, one key at a time
// This program,              which is designed specifically for an M5Stack, and extends the calculator using its A/B/C buttons
//
// By Van Kichline
// In the year of the plague


#include <M5ez.h>
#include "KeyCalculator.h"
#include "screen_layout.h"
#include "screen_ui.h"
#include "menu_ui.h"


#define KEYBOARD_I2C_ADDR     0X08            // I2C address of the Calculator FACE
#define KEYBOARD_INT          5               // Data ready pin for Calculator FACE (active low)


KeyCalculator calc;
TFT_eSprite   sprite          = TFT_eSprite(&M5.Lcd);
String        button_sets[]   = { BUTTONS_NORMAL_0, BUTTONS_NORMAL_1, BUTTONS_NORMAL_2, BUTTONS_NORMAL_3, BUTTONS_NORMAL_4 };
uint8_t       button_set      = 0;
bool          cancel_bs       = false;        // If true, override displaying the BS buttons
bool          stacks_visible  = true;         // Can be turned off in settings menu


////////////////////////////////////////////////////////////////////////////////
//
//  Read a key from the calculator keyboard.
//  Return true and change ref to input char if key is available, otherwise return false.
//
bool read_key(char& input) {
  if(digitalRead(KEYBOARD_INT) == LOW) {      // If a character is ready
    Wire.requestFrom(KEYBOARD_I2C_ADDR, 1);   // request 1 byte from keyboard
    if(Wire.available()) {
      char key_val = Wire.read();             // receive a byte as character
      if(0 != key_val) {
        input = key_val;
        return true;
      }
    }
  }
  return false;
}


////////////////////////////////////////////////////////////////////////////////
//
//  Read a key from the calculator keyboard, or a button from the M5Stack.
//
bool process_input() {
  char input;

  // Process keyboard input. calc does all the work.
  if(read_key(input)) {
    if(calc.key(input) || calc.get_error_state()) {
      display_all();
      return true;
    }
  }

  // See if any buttons have been pressed and if so, dispatch the indicated function
  String result = ez.buttons.poll();
  if(result.length()) {
    if (result == "right") {
      if(!cancel_bs && calcEnteringNumber == calc.get_state()) {
        // Special case: we're displaying the BUTTONS_NUM_MODE menu, and want to get out of it.
        cancel_bs = true;
      }
      else {
        button_set++;
        if(NUM_BUTTON_SETS <= button_set)
        button_set = 0;
      }
    }
    // memory mode
    else if(result == "get")    calc.key('M');  // In memory mode: retrieve
    else if(result == "set")    calc.key('=');  // In memory mode: st
    else if(result == "clear")  calc.key('A');  // In memory mode: clear
    else if(result == "M")      calc.key('M');  // In memory mode: retrieve
    else if(result == "=")      calc.key('=');  // In memory mode: st
    else if(result == "AC")     calc.key('A');  // In memory mode: clear
    // number entry mode
    else if(result == "BS")     calc.key('B');  // KeyCalculator command for backspace
    else if(result == "cancel") calc.cancel_input();
    // normal 0
    else if(result == "help")   help_screen();
    else if(result == "menu")   menu_menu();
    // normal 1
    else if(result == "(")      calc.key(OPEN_PAREN_OPERATOR);
    else if(result == ")")      calc.key(CLOSE_PAREN_OPERATOR);
    // normal 2
    else if(result == "pi")     calc.set_value("3.14159265");
    else if(result == "e")      calc.set_value("2.71828182");
    // normal 3
    else if(result == "push")   { calc.commit();  calc.push(); }
    else if(result == "pop")    { calc.commit();  calc.pop(); }
    // normal 4
    else if(result == "square") calc.key(SQUARE_OPERATOR);
    else if(result == "sqroot") calc.key(SQUARE_ROOT_OPERATOR);

    if (result != "right") {
      cancel_bs = false; // get out of cancel_bs as soon as any non-right button pressed.
    }

    display_all();
    return true;
  }
  return false;
}


////////////////////////////////////////////////////////////////////////////////
//
//  See if the calculator keyboard is attached. Show a message if it is not.
//  Assumes Wire.begin() has been called.
//
bool test_for_keyboard() {
  Wire.beginTransmission(KEYBOARD_I2C_ADDR);
  bool found = (0 == Wire.endTransmission ());
  if(!found) ez.msgBox("Keyboard Error", "ERROR: No FACES Calculator Keyboard found at I2C address 0X08.\nNo Keyboard,\nNo Calculator!");
  return found;
}


////////////////////////////////////////////////////////////////////////////////
//
//  Arduino setup function, called once at beginning of program
//
void setup() {
  ez.begin();
  Wire.begin();
  pinMode(KEYBOARD_INT, INPUT_PULLUP);
  test_for_keyboard();
  sprite.createSprite(SCREEN_WIDTH - LEFT_MARGIN - RIGHT_MARGIN, NUM_HEIGHT);
  M5.Lcd.setTextSize(1);
  display_all();
}


////////////////////////////////////////////////////////////////////////////////
//
// Arduino loop function, called repeatedly
//
void loop() {
  process_input();
  delay(10);
}

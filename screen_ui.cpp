#include <M5ez.h>
#include "KeyCalculator.h"
#include "screen_layout.h"
#include "screen_ui.h"

// This file contains the functions that render the screen display.
// Only display_all() is of interest to the main program.


////////////////////////////////////////////////////////////////////////////////
//
//  Display the calculator status in small text above the value.
//
void display_status() {
  M5.Lcd.setTextFont(STAT_FONT);
  M5.Lcd.setTextDatum(TL_DATUM);
  M5.Lcd.setTextColor(STAT_FG_COLOR, STAT_BG_COLOR);  // Blank space erases background w/ background color set
  M5.Lcd.fillRect(0, STAT_TOP, SCREEN_WIDTH, STAT_HEIGHT, STAT_BG_COLOR);
  M5.Lcd.drawString(calc.get_display(dispStatus), STAT_LEFT_MARGIN, STAT_TOP, STAT_FONT);
}


////////////////////////////////////////////////////////////////////////////////
//
//  Main numeric display; show the number being entered or the current value.
//  Since the number can be wider than the display, and text wrapping always wraps to zero,
//  and a left margin is desired, a sprite is used to render the characters to the screen.
//
void display_value() {
  bool is_err = calc.get_error_state();
  sprite.fillSprite(NUM_BG_COLOR);
  sprite.setTextFont(NUM_FONT);
  sprite.setTextColor(is_err ? ERROR_COLOR :NUM_FG_COLOR, NUM_BG_COLOR);  // Blank space erases background w/ background color set
  sprite.setTextWrap(true);

  String   disp_value = calc.get_display(dispValue);
  uint16_t margin     = 0;
  uint16_t wid        = sprite.textWidth(disp_value);
  if(sprite.width() > wid) margin = sprite.width() - wid;
  sprite.setCursor(margin, 0);
  sprite.print(disp_value);
  sprite.pushSprite(LEFT_MARGIN, NUM_TOP);
}


////////////////////////////////////////////////////////////////////////////////
//
//  Dual use area: only rendered when required.
//  If the user has pressed the M key, show the memory location we're building up (M, M[n], M[nn])
//  along with the current value at that location.
//  If we're in global error mode, show the error instead.
//
void display_memory_storage() {
  String disp_value;
  M5.Lcd.fillRect(0, MEM_TOP, SCREEN_WIDTH, MEM_HEIGHT, MEM_BG_COLOR);
  Op_Err err = calc.get_error_state();
  if(err) {
    M5.Lcd.setTextFont(MEM_FONT);
    M5.Lcd.setTextColor(ERROR_COLOR, MEM_BG_COLOR);
    switch (err) {
      case ERROR_TOO_FEW_OPERANDS:  disp_value = "Too Few Operands";      break;
      case ERROR_UNKNOWN_OPERATOR:  disp_value = "Unknown Operator";      break;
      case ERROR_DIVIDE_BY_ZERO:    disp_value = "Divide by Zero";        break;
      case ERROR_NO_MATCHING_PAREN: disp_value = "No Matching (";         break;
      case ERROR_OVERFLOW:          disp_value = "Overflow";              break;
      default:                      disp_value = "Unknown Error: " + err; break;
    }
    M5.Lcd.drawCentreString(disp_value.c_str(), SCREEN_H_CENTER, MEM_TOP + MEM_V_MARGIN, MEM_FONT);
  }
  else {
    M5.Lcd.setTextFont(MEM_FONT);
    if(calcEnteringMemory == calc.get_state()) {
      disp_value = calc.get_display(dispMemoryID);
      M5.Lcd.setTextColor(MEM_FG_COLOR, MEM_BG_COLOR);  // Blank space erases background w/ background color set
      M5.Lcd.drawCentreString(disp_value.c_str(), SCREEN_H_CENTER, MEM_TOP + MEM_V_MARGIN, MEM_FONT);
    }
  }
}


////////////////////////////////////////////////////////////////////////////////
//
//  Display the operator stack at the lower left, and value stack at the lower right.
//  This feature is non-standard and can be turned off, but it makes it easy to spot errors
//  that would otherwise be difficult to track down.
//
void display_stacks() {
  bool   is_err    = calc.get_error_state();
  String op_stack  = calc.get_display(dispOpStack);
  String val_stack = calc.get_display(dispValStack);
  M5.Lcd.fillRect(0, STACK_TOP, SCREEN_WIDTH, STACK_HEIGHT,STACK_BG_COLOR);
  if(stacks_visible && (3 < op_stack.length() || 3 < val_stack.length())) {
    M5.Lcd.setTextDatum(TL_DATUM);
    M5.Lcd.setTextFont(STACK_FONT);
    M5.Lcd.setTextColor(is_err ? ERROR_COLOR : STACK_FG_COLOR, STACK_BG_COLOR);  // Blank space erases background w/ background color set
    M5.Lcd.drawString(op_stack.c_str(), LEFT_MARGIN, STACK_TOP + STACK_V_MARGIN, STACK_FONT);
    M5.Lcd.setTextDatum(TR_DATUM);
    M5.Lcd.drawString(val_stack.c_str(), SCREEN_WIDTH - RIGHT_MARGIN, STACK_TOP + STACK_V_MARGIN, STACK_FONT);
    M5.Lcd.setTextDatum(TL_DATUM);
  }
}


////////////////////////////////////////////////////////////////////////////////
//
//  Set the buttons at the bottom of the screen appropriately, depending on the mode
//
void set_buttons() {
  if(calcEnteringMemory == calc.get_state()) {
    ez.buttons.show(BUTTONS_MEM_MODE);
  }
  else if(!cancel_bs && calcEnteringNumber == calc.get_state()) {
    ez.buttons.show(BUTTONS_NUM_MODE);
  }
  else {
    ez.buttons.show(button_sets[button_set]);
  }
}


////////////////////////////////////////////////////////////////////////////////
//
//  Consolidated function to call repeatedly to render the calculator screen.
//
void display_all() {
  display_value();
  display_status();
  display_memory_storage();
  set_buttons();
  display_stacks();
  ez.header.show("Calculator");   // restore the header after its been reused
  ez.yield();
}

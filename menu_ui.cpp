#include <M5ez.h>
#include "KeyCalculator.h"
#include "menu_ui.h"
#include "help_text.h"

// This file displays all the menus and text boxes associated with the UI, using M5ez UI.


////////////////////////////////////////////////////////////////////////////////
//
//  Display a menu of ten indexed memory values, starting at index
//
void show_indexed_memory_group(uint8_t index) {
  ezMenu menu("Indexed Memory");
  menu.txtSmall();
  menu.buttons("up # back # down");
  for(int i = 0; i < 10; i++) {
    menu.addItem(String("M[") + String(index) + "]\t" + String(calc._calc.get_memory(index)));
    index++;
  }
  menu.run();
}


////////////////////////////////////////////////////////////////////////////////
//
//  Display a menu for selecting a sub-group of indexed memories, broken up
//  by groups of ten. By each group, display how many memories in that group
//  are non-zero. Display the sub-group if selected.
//  Changes no values; display only.
//
void show_indexed_memory() {
  int index = 0;
  ezMenu menu("Indexed Memory");
  menu.txtSmall();
  menu.buttons("up # back # select ## down#");
  // Add ten menus, reading like: "M[50] - M[59]    3"
  for(int i = 0; i < 10; i++) {
    int count = 0;
    // count how many in this group are non-zero values
    for(int j = 0; j < 10; j++) if(00 != calc._calc.get_memory(i*10+j)) count++;
    menu.addItem(String("M[") + index + "] - M[" + (index+9) + "]\t" + (count ? String(count) : ""));
    index += 10;
  }
  menu.addItem("back | Back to Calculator Settings");
  while(menu.runOnce()) {
    show_indexed_memory_group((menu.pick() - 1) * 10);
  }
}


////////////////////////////////////////////////////////////////////////////////
//
//  If the memory stack is empty, show a notice. If not, show the entire stack.
//
void show_memory_stack() {
  if(0 == calc._calc.get_memory_depth()) {
    ez.msgBox("Memory Stack Empty", "There are no values in the memory stack.");
  }
  else {
    ezMenu menu("Memory Stack");
    menu.txtSmall();
    menu.buttons("up # back #  down");
    for(int i = calc._calc.get_memory_depth() - 1; i >= 0; i--) {
      menu.addItem(calc.double_to_string(calc._calc.memory_stack[i]));
    }
    menu.run();
  }
}


////////////////////////////////////////////////////////////////////////////////
//
//  Perform a few simple operations on the memory stack
//
void memory_stack_operations() {
  ezMenu menu("Memory Stack Ops");
  menu.txtSmall();
  menu.buttons("up # back # select ## down #");
  menu.addItem("Clear");
  menu.addItem("Sum");
  menu.addItem("Average");
  menu.addItem("Count");
  menu.addItem("back | Back to Calculator Settings");
  while(menu.runOnce()) {
    if(menu.pickName() == "Clear") calc._calc.clear_memory_stack();
    else if(menu.pickName() == "Sum") {
      double total = 0.0;
      for(int i = 0; i < calc._calc.memory_stack.size(); i++)
        total += calc._calc.memory_stack[i];
      calc.set_value(calc.double_to_string(total));
    }
    else if(menu.pickName() == "Average") {
      double total = 0.0;
      int    depth = calc._calc.memory_stack.size();
      if(depth) {
        for(int i = 0; i < depth; i++)
          total += calc._calc.memory_stack[i];
        total /= depth;
      }
      calc.set_value(calc.double_to_string(total));
    }
    else if(menu.pickName() == "Count") {
      calc.set_value(calc.double_to_string(calc._calc.memory_stack.size()));
    }
  }
}


////////////////////////////////////////////////////////////////////////////////
//
//  Display a menu of miscellaneous functions
//
void menu_menu() {
  ezMenu menu("Calculator Settings");
  menu.txtSmall();
  menu.buttons("up # back # select ## down #");
  menu.addItem(String("Stacks | Display Calculator Stacks\t") + (stacks_visible ? "On" : "Off"));
  menu.addItem("View Indexed Memory");
  menu.addItem("View Memory Stack");
  menu.addItem("Memory Stack Operations");
  menu.addItem("Exit | Back to Calculator");
  while(menu.runOnce()) {
    if(menu.pickName() == "Exit") return;
    else if(menu.pickName() == "Stacks") {
      if(menu.pickCaption().endsWith("On")) {
        menu.setCaption("Stacks", "Display Calc Stacks\tOff");
        stacks_visible = false;
      }
      else {
        menu.setCaption("Stacks", "Display Calc Stacks\tOn");
        stacks_visible = true;
      }
    }
    else if(menu.pickName() == "View Indexed Memory") {
      show_indexed_memory();
    }
    else if(menu.pickName() == "View Memory Stack") {
      show_memory_stack();
    }
    else if(menu.pickName() == "Memory Stack Operations") {
      memory_stack_operations();
    }
  }
}


////////////////////////////////////////////////////////////////////////////////
//
//  Respond to the "?" button with some instructions
//
void help_screen() {
  ez.textBox("Calculator Help", HELP_TEXT, true);
}

#pragma once

// Dimensions of elements in the screen layout
// At the top of the screen is the M5ez Header with the program title.
// The top row is a status display, which is only shown when there are open parens or stored memory.
// The next row is the main calculator value display. It's rendered from a sprite and can be two lines long.
// The next area is shared by the Memory Mode display, and Global Error display. They are mutually exclusive.
// The lowest row shows the operator and value stacks. It's normally visible, but can be disabled.
// At the bottom of the screen are three buttons, whose captions change based on context.


#define SCREEN_WIDTH          320           // Horizontal screen size
#define SCREEN_H_CENTER       160           // Horizontal center of screen
#define FG_COLOR              BLACK         // Arbitrary foreground color
#define BG_COLOR              0xEF7D        // Arbitrary background color
#define ERROR_COLOR           RED

#define LEFT_MARGIN           11            // Indentation used by value display
#define RIGHT_MARGIN          11

#define STAT_TOP              24            // Top of the status display
#define STAT_HEIGHT           20            // Height of the status display
#define STAT_FONT             2             // Font for the status display
#define STAT_LEFT_MARGIN      8             // Left margin used by the status display
#define STAT_FG_COLOR         BLUE          // Foreground (text) color of the status display
#define STAT_BG_COLOR         BG_COLOR      // Background color of the status display

#define NUM_TOP               44            // Top of the number display, where the sum is shown
#define NUM_HEIGHT            96            // Height of the number display
#define NUM_V_MARGIN          4             // Offset from top to top text
#define NUM_H_MARGIN          16            // Left/right margin of the number
#define NUM_FONT              6             // Number font
#define NUM_FG_COLOR          FG_COLOR      // Number display foreground color
#define NUM_BG_COLOR          BG_COLOR      // Number display background color

#define MEM_TOP               135           // Top of the memory storage display
#define MEM_HEIGHT            32            // Height of the memory storage display
#define MEM_V_MARGIN          4             // Offset from top to top text
#define MEM_FONT              4             // Font used for the memory storage display
#define MEM_FG_COLOR          BLUE          // Foreground (text) color of the memory storage display
#define MEM_BG_COLOR          BG_COLOR      // Background color of the memory storage display

#define STACK_TOP             185           // Top of the calculator stacks display
#define STACK_HEIGHT          24            // Height of the calculator stacks display
#define STACK_V_MARGIN        4             // Vertical margin used by the calculator stacks display
#define STACK_FONT            2             // Font used by the calculator stacks display
#define STACK_FG_COLOR        BLUE          // Foreground (text) color of the calculator stacks display
#define STACK_BG_COLOR        BG_COLOR      // Background color of the calculator stacks display

# BetterM5Calculator

## Note

This is an Arduino project for the M5Stack ESP32 computer. It's a fork of the [full project](https://github.com/vkichline/BetterM5Calculator) which is built with PlatoformIO. PlatformIO's unit testing capabilities were a huge help developing this project.  
However, I realized that many people would just pass by if there weren't a way to use the calculator program without including a new IDE learning curve, so I've copied the files here, made a couple tiny changes, and tested it a bit by hand.  
Just download the project, open the ino file, and build and upload as you would any Arduino project.


![Whole Device](https://github.com/vkichline/BetterM5Calculator/raw/master/img/WholeDevice.jpg)

## Goal

I purchased the [M5STACK Faces Kit Pocket Computer with Keyboard/Game/Calculator](https://m5stack.com/collections/m5-core/products/face) and really liked the Calculator Keyboard.
I loaded the [Arduino sample program](https://github.com/m5stack/M5-ProductExampleCodes/Module/CALCULATOR/CALCULATOR.ino) and saw that it demonstrated how to use the
keyboard, without doing much to implement an actual calculator.  
A look through the forums revealed that others had looked for a calculator program and had failed to find anything. So I wrote a very simple calculator program in a day:
[M5Calc-Arduino](https://github.com/vkichline/M5Calc-Arduino). This looks like a calculator, and acts sort of like one, but as the readme says, it's simplified.

I decided to write a real calculator program for the M5Stack; one which would be actually usable, expose some advantages of having a powerful processor to work with, and serve as an extensible base for future development. The foundation layer I'm working on here does only double-precision floating-point math in base 10 and has no trigonometric functions. However, it's designed to be capable
of growing into an excellent scientific calculator, even with the simple keyboard, using UI enhancements.

![Start Screen](https://github.com/vkichline/BetterM5Calculator/raw/master/img/StartScreen.jpg)

## How to Calculate

This calculator works like you would expect a calculator to, with two notable additions; the 3 M5 buttons and Memory.  
The buttons rotate their captions as you press Button C (right arrow), eventually starting over again. When entering
a number, a special set of buttons is displayed, permitting you to backspace or cancel number input. If you want to
get back to the ordinary buttons, just press Button C (right arrow). When inputting memory, a special set of buttons is
shown with shortcuts for get, set, and clear.

Memory is very powerful on the M5 Calculator. You get one 'simple' memory, 100 numbered memories (0-99), and an
unlimited push-down memory stack. Once you press the keyboard's 'M' key, you are in Memory Mode, and the next key is
interpreted specially:

* `MM` :   Recalls simple memory
* `M=` :  Stores simple memory
* `MA` :  Clears simple memory
* `M+` :   Adds to simple memory
* `M-` :   Subtracts from simple memory
* `M*` :   Multiplies simple memory
* `M%` :   Takes percent of simple memory
* `M.` :   Cancels memory mode
* `M3M` :  Recalls M[3]
* `M99+` : Adds to M[99]

On the screen below, I've pressed the 'M' key to enter Memory Mode, and have pressed a 5 to indicate memory address 5. I could continue and press another digit to select memory address 53, or press any of the second keys listed above to preform a memory operation.

![Entering Memory](https://github.com/vkichline/BetterM5Calculator/raw/master/img/EnteringMemory.jpg)

The AC key clears the current value. Pressing AC twice in a row clears all memory as well.  
As with many calculators, you must use the +/- button to enter a negative number. Press +/- anytime that you are
in number entry mode, and the leading '-' sign will appear or disappear.  
The display line just above the buttons shows the operator stack on the left, and the value stack on the right. You
may find it interesting to see how the calculator calculates, but it's mainly there to reveal invisible bugs

![Status Display](https://github.com/vkichline/BetterM5Calculator/raw/master/img/Status.jpg)

The Status is displayed above the main value. First, the state value of the KeyCalculator is displayed:

* `A` :  Ready for Any input
* `N` :  Ready for Numeric input
* `O` :  Ready for Operator input
* `>` : Currently building a number in the display
* `M` :  In "Memory Mode", building a memory address
* `X` :  In global error mode

If any numbered memories are in use, up to eight
indexes will be shown, as in M[3,7,19]. If more than eight are in use, the indexes will be followed by '...'.  
Next, if the memory stack has any values on it, the status display will show S(n), where n is the depth of the stack.  
Finally, if simple memory is set, that status display will show M=nnn, where nnn is the value of memory.

Beyond simple calculator features, there's a help screen and an extensible menu system to provide custom functionality:

![Help](https://github.com/vkichline/BetterM5Calculator/raw/master/img/Help.jpg)
![Menu](https://github.com/vkichline/BetterM5Calculator/raw/master/img/Menu.jpg)

All memory locations can be inspected. Simple memory is displayed in the Status Line. Indexed and Stack Memory can be inspected from menu selections:

![Indexed Memory](https://github.com/vkichline/BetterM5Calculator/raw/master/img/IndexedMemory.jpg)
![Memory Stack](https://github.com/vkichline/BetterM5Calculator/raw/master/img/MemoryStack.jpg)

Finally, a few examples of stack operations are included. YOu may want to add more to this menu.

![Memory Stack Operations](https://github.com/vkichline/BetterM5Calculator/raw/master/img/StackOperations.jpg)


## Design

The calculator is built with strict separation of concerns as a guide.

* The lowest level, the CoreCalculator, is a state-free stack evaluator, designed as a class template so that any numeric type can be used for computation.
* The next level, the MemoryCalculator, is another class template which add memories and memory operations.
* The TextCalculator is a concrete class, currently aggregating a single double-precision CoreCalculator, adding textual conversion needed for UI.
* The KeyCalculator wraps the TextCalculator in a state-driven object that handles input one character at a time from a keyboard.
* And finally, the files in src create an M5Stack-aware KeyCalculator and provide display and extra input options.

### `CoreCalculator<T>`

This template takes a single typename parameter and creates a basic, no-frills calculator engine with a value stack, an operand stack, and and evaluator. It includes functions for manipulating the two stacks, evaluating
the stacks, and processing the Global Error State.  Currently, only divide by zero errors are handled; overflow and underflow are planned.  
Operators are implemented as objects which are added to an _operator array. You can remove, replace, or add additional operators derived from `Operator<T>`, whose tye must match the calculator's type.  
Type-specific operators (for example, operators that work only on integers or on floating-point numbers) can be added in type-specific calculators derived from this template.

### `MemoryCalculator<T, M>`

MemoryCalculator adds a "simple" memory, and array of M indexed memories (ste to 100 in this example), and a memory stack limited only by RAM. Memories must match the data type of the CoreCalculator.  
By keeping memory operations out of the CoreCalculator, and calculations out of the MemoryCalculator implementations, they're much simpler and more cohesive.

### `TextCalculator`

Ultimately the calculator must use human-readable data. This layer converts numbers to text and back.  Concepts such as number base (binary, octal, decimal, hexadecimal) belong in this layer,
as do trigonometric modes (degree, radian, rads) but are not yet implemented at this point.  
This layer includes an extremely simple parser. This can be leveraged for simplifying test creation, or for use in other programs. It's not used in the calculator.

### `KeyCalculator`

The KeyCalculator is a state-driven processor for keystrokes. While operators are generally one keystroke, numbers and memory addresses must be composed. Special keys like AC may have special semantics.
This layer of the engine actively rejects keys it sees as inappropriate (like a close parentheses when there's no open parentheses) in order to keep errors from occurring. Ideally, only valid key combinations would
be accepted. Utility routines are provided so that hosts can easily provide complete and accurate state information.

### `M5Calculator`

Finally, a small program is wrapped around the KeyCalculator which interacts with the M5Stack computer, using its buttons and screen as well as the calculator keyboard extension.  
A status display is supplied at the top of the screen, followed by a value display, an area used to display memory selections of error message, and a view of the operator and value stacks.  Under the screen display are button labels for the A, B and C buttons, whose labels change based on state and user selection.  The menu selection, in particular, allows access to settings and additional functions.

## Future Plans, or Opportunities for the Enthusiast

* Overflow, Underflow(s) and inexact zero display handling
* I'd like to use a more powerful numeric base class, like Python's Huge Numbers.
* Trigonometric functions
* A parallel integer calculator for Binary, Octal and Hexadecimal modes, with appropriate operators
* A history display
* Save and Restore entire machine state
* A programable calculator with web interface

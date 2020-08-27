#pragma once

extern KeyCalculator          calc;
extern TFT_eSprite            sprite;
extern String                 button_sets[];
extern uint8_t                button_set;
extern bool                   cancel_bs;
extern bool                   stacks_visible;

#define BUTTONS_NUM_MODE      "BS # cancel # right"
#define BUTTONS_MEM_MODE      "get # M # set # = # clear # AC"
#define BUTTONS_NORMAL_0      "help # menu # right"
#define BUTTONS_NORMAL_1      "( # ) # right"
#define BUTTONS_NORMAL_2      "pi # e # right"
#define BUTTONS_NORMAL_3      "push # pop # right"
#define BUTTONS_NORMAL_4      "square # sqroot # right"
#define NUM_BUTTON_SETS       5


void display_all();

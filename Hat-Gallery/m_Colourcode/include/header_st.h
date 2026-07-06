#pragma once

#define StageCount 2
#define PasswordAmount 6
#define MaxPassLen 12


// may aswell move this into the Oled lib?
#define headLineMaxSize 16

// build to fit into a legacy system so those are not consistent
#define open   0
#define closed 1


enum brains {
    colour_brain,
    brain_count
};

enum relays {
    safe,
    leds,
    relayAmount
};

enum relayInits {
    safe_init = closed,
    leds_init = closed
};

int relayPinArray[relayAmount] = {
    safe,
    leds_init
};

int relayInitArray[relayAmount] = {
    safe_init,
    leds_init
};


enum stages{
    gameLive = 1,
    serviceMode = 2
};

// the sum of all stages sprinkled with a bit of black magic
int stageSum = ~( ~0 << StageCount );


// could have multiple brains listed here making up a matrix
int flagMapping[StageCount]{
    keypadFlag,
    keypadFlag
};


char passwords[PasswordAmount][MaxPassLen] = {
    "rggbwgrbwg",
};


char stageTexts[StageCount][headLineMaxSize] = {
};

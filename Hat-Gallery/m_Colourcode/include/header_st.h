#pragma once

#define PasswordAmount 1
#define MaxPassLen 12


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
    leds
};

int relayInitArray[relayAmount] = {
    safe_init,
    leds_init
};


enum stages{
    gameLive = 1,
    solved = 2,
    serviceMode = 4,
    StageCount = 3
};

// the sum of all stages sprinkled with a bit of black magic
int stageSum = ~( ~0 << stages::StageCount );


// could have multiple brains listed here making up a matrix
int flagMapping[stages::StageCount]{
    keypadFlag,
    keypadFlag
};


char passwords[PasswordAmount][MaxPassLen] = {
    "rggbwgrbwg",
};

// defines what password/RFIDCode is used at what stage, if none is used its -1
int passwordMap[PasswordAmount] = {
    gameLive
};


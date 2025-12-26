#pragma once

#define StageCount 2
#define PasswordAmount 6
#define MaxPassLen 10


// may aswell move this into the Oled lib?
#define headLineMaxSize 16

// build to fit into a legacy system so those are not consistent
#define open   0
#define closed 1


enum brains {
    colour_brain,
};

enum relays {
    safe,
    relayAmount
};

enum relayInits {
    safe_init = closed
};

int relayPinArray[relayAmount] = {
    safe
};

int relayInitArray[relayAmount] = {
    safe_init
};


enum stages{
    gameLive = 1,
    serviceMode = 2
};

// the sum of all stages sprinkled with a bit of black magic
int stageSum = ~( ~0 << StageCount );


// could have multiple brains listed here making up a matrix
int flagMapping[StageCount]{
    keypadFlag
};


char passwords[PasswordAmount][MaxPassLen] = {
    "1708",
};


char stageTexts[StageCount][headLineMaxSize] = {
};

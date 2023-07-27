#pragma once

#define StageCount 2
#define PasswordAmount 6
#define MaxPassLen 10


// may aswell move this into the Oled lib?
#define headLineMaxSize 16

#define open   0
#define closed 1

enum relays {
    light,
    exitDoor,
    relayAmount, 
};

enum relayInits {
    light_init = closed,
    exit_init = closed,
};

int relayPinArray[relayAmount] = {
    light,
    exitDoor,
};

int relayInitArray[relayAmount] = {
    light_init,
    exit_init,
};


enum stages{
    gameLive = 1,
    serviceMode = 2
};

// the sum of all stages sprinkled with a bit of black magic
int stageSum = ~( ~0 << StageCount );


// could have multiple brains listed here making up a matrix
int flagMapping[StageCount]{
    keypadFlag + oledFlag,
    keypadFlag + oledFlag
};


char passwords[PasswordAmount][MaxPassLen] = {
    "1708",
    "2381984"
};


char stageTexts[StageCount][headLineMaxSize] = {
    "Enter Code",
    "Enter Code"
};

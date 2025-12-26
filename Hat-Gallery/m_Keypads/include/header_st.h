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
    light_brain,
    chimney_brain,
    exit_brain, 
    brain_count
};

enum relays {
    light,
    chimney,
    exitDoor,
    relayAmount, 
};

enum relayInits {
    light_init = closed,
    chimney_init = closed,
    exit_init = open,
};

int relayPinArray[relayAmount] = {
    light,
    chimney,
    exitDoor,
};

int relayInitArray[relayAmount] = {
    light_init,
    chimney_init,
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
    "5314",
    "2381984",
    "20162023",     // service code
};


char stageTexts[StageCount][headLineMaxSize] = {
    "Enter Code",
    "Enter Code"
};

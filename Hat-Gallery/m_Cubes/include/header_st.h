#pragma once

#define StageCount 11
#define PasswordAmount 3
#define MaxPassLen 10
// may aswell move this into the Oled lib?
#define headLineMaxSize 16

#define relayAmount 2
#define open        0
#define closed      1
#define Hamburg     1

#define doorOpen    0
#define doorClosed  1

#define ledCnt 3

// FL lamps shall be some industrial reddish light 
//static constexpr int clrLight[3] = {255,200,120};
static constexpr int clrLight[3] = {255,0,0};

enum brains {
    socket_rfid_1,
    socket_rfid_2,
    socket_rfid_3,
    leds,
    brain_cnt
};


enum IOpins {
    IO_1,
    IO_2,
    IO_3,
    IO_4,
    IO_5, 
    IO_6,
    IO_7,                       
    IO_8,                                            
};

// 15 values 4 IOs
enum inputValues {  
    roomBoot = 1,            
};

// 7 Values 3 IOs
enum outputValues {
    david = 1,
};



#define outputCnt 3
#define inputCnt 5


int intputArray[inputCnt] = {
    IO_1,
};

int outputArray[outputCnt] = {
    IO_5,                  
};


// -- relays
enum relays {
    uv,
    light
};

enum relayInits {
    uvInit = doorClosed,
    lightInit = doorClosed,
};

int relayPinArray[relayAmount] = {
    uv, 
    light,
};

int relayInitArray[relayAmount] = {
    uvInit,
    lightInit
};


enum stages {
    live = 1, 
    solved = 2, 
};

// the sum of all stages sprinkled with a bit of black magic
int stageSum = ~( ~0 << StageCount );


// could have multiple brains listed here making up a matrix
// for now its only an Access module mapped here
int flagMapping[StageCount] {
    rfidFlag,
    0
};

char passwords[PasswordAmount][MaxPassLen] = {
    "SD",   // David
    "AH"    // Rachel
};

// defines what password/RFIDCode is used at what stage, if none is used its -1
int passwordMap[PasswordAmount] = {
    live,
    live,
    live
};
// make a mapping of what password goes to what stage


char stageTexts[StageCount][headLineMaxSize] = {
    "",                     // setupStage
    "",                     // idle 
};
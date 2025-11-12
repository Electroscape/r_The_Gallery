#pragma once

#define StageCount 2
#define PasswordAmount 1
#define MaxPassLen 10

#define open   0
#define closed 1


char passwords[PasswordAmount][MaxPassLen] = {
    "RGGBWGRBWG"   
};


enum stages{
    unsolved = 1,
    solved = 2,
};


enum relayInits {
    safe_init = closed,
};

int relayPinArray[relayAmount] = {
    safe
};

int relayInitArray[relayAmount] = {
    safe_init
};

// CONSTANTS
#define SAFE_HIDDEN 0
#define SAFE_VISIBLE 1

// by default enabled
//#define OLED_DISABLE 1

// Standards der Adressierung (Konvention)
#define RELAY_I2C_ADD 0x3F   // Relay Expander																							*/
#define OLED_I2C_ADD 0x3C    // Ist durch Hardware des OLEDs vorgegeben
#define KEYPAD_I2C_ADD 0x39  // möglich sind 0x38, 39, 3A, 3B, 3D


int flagMapping[StageCount]{
    keypadFlag,
    0
};


int passwordMap[PasswordAmount] = {
    unsolved,
};
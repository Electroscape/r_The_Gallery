#pragma once

/*==KEYPAD I2C============================================================*/
#define KEYPAD_ADD 0x38

char dummyPassword[] = "";

const byte KEYPAD_ROWS = 1;  // Zeilen
const byte KEYPAD_COLS = 4;  // Spalten
const byte KEYPAD_CODE_LENGTH = 16;
const byte KEYPAD_CODE_LENGTH_MAX = 16;


char KeypadKeys[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'w', 'r', 'g', 'b'}
};


byte KeypadColPins[KEYPAD_COLS] = {1, 2, 3, 4};     // Spalten - Steuerleitungen (abwechselnd HIGH)
byte KeypadRowPins[KEYPAD_ROWS] = {0};  // Zeilen  - Messleitungen


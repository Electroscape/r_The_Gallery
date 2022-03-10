/**
* To configure a relay:
*   - rename REL_X_PIN to informative name
*   - rename and set REL_X_INIT with your init value
*   - create array with the used pins and benefit from loops :D
*/
#pragma once

String title = "GL_ST_COLOR_CODE";
String versionDate = "10.05.2021";
String version = "version 1.0ST";
String brainName = String("BrCOLOR");
String relayCode = String("HID");
const unsigned long heartbeatFrequency = 5000;

//==KEYPAD I2C================================/
const byte KEYPAD_ROWS = 1;  // Zeilen
const byte KEYPAD_COLS = 4;  // Spalten
const char KeypadKeys[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'R', 'G', 'B', 'W'}};

// Passwort
// char* sercet_password = (char*)"RGGBWGRBWG";
char *secret_password = (char *)"RGGBWGRBWG";  

unsigned long keypadCheckingInterval = 8000;  // Check the password after set time of inactivity
unsigned int keypadDebounceTime = 50;      // Time for debouncing resolution

const int oledUpdateInterval = 10000;  // time between refreshing of the oled


// CONSTANTS
#define SAFE_HIDDEN 0
#define SAFE_VISIBLE 1

#define MAX485_READ LOW
#define MAX485_WRITE HIGH

// by default enabled
//#define OLED_DISABLE 1

// Standards der Adressierung (Konvention)
#define RELAY_I2C_ADD 0x3F   // Relay Expander																							*/
#define OLED_I2C_ADD 0x3C    // Ist durch Hardware des OLEDs vorgegeben
#define KEYPAD_I2C_ADD 0x39  // möglich sind 0x38, 39, 3A, 3B, 3D

// RELAY
enum REL_PIN {
    REL_0_PIN,          // 0 First room light
    REL_1_PIN,          // 1 Light 2nd room
    REL_2_PIN,          // 2 UV Light
    REL_3_PIN,          // 3 Alarm
    REL_4_PIN,          // 4 Empty
    REL_5_PIN,          // 5 Fireplace valve
    REL_SAFE_PIC_PIN,  // 6 valve holding the painting
    REL_7_PIN           // 7 Exit door lock
};

enum REL_INIT {
    REL_0_INIT = 1,                    // DESCRIPTION OF THE RELAY WIRING
    REL_1_INIT = 1,                    // DESCRIPTION OF THE RELAY WIRING
    REL_2_INIT = 1,                    // DESCRIPTION OF THE RELAY WIRING
    REL_3_INIT = 1,                    // DESCRIPTION OF THE RELAY WIRING
    REL_4_INIT = 1,                    // DESCRIPTION OF THE RELAY WIRING
    REL_5_INIT = 1,                    // DESCRIPTION OF THE RELAY WIRING
    REL_SAFE_PIC_INIT = SAFE_HIDDEN,  // NC = Empty | COM = 24V  | NO = Valve
    REL_7_INIT = 1                     // DESCRIPTION OF THE RELAY WIRING
};

// == constants
#define REL_AMOUNT 1
const enum REL_PIN relayPinArray[] = {
    REL_SAFE_PIC_PIN};
const byte relayInitArray[] = {
    REL_SAFE_PIC_INIT};

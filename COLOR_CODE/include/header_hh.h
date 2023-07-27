/**
* To configure a relay:
*   - rename REL_X_PIN to informative name
*   - rename and set REL_X_INIT with your init value 
*/
#pragma once

String title = "GL_HH_STEERINGWHEEL";
String versionDate = "17.02.2021";
String version = "version 1.0HH";
String brainName = String("BrWheel");
String relayCode = String("HID");
const unsigned long heartbeatFrequency = 5000;

//==KEYPAD I2C================================/
const byte KEYPAD_ROWS = 1;  // Zeilen
const byte KEYPAD_COLS = 4;  // Spalten
const char KeypadKeys[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'O', 'U', 'L', 'R'}};

// Passwort
char* secret_password = (char*)"OULR";  // Simple sample password

unsigned long keypadCheckingInterval = 5000;  // Zeit bis Codereset
unsigned int keypadDebounceTime = 50;      // Time for debouncing resolution

const int oledUpdateInterval = 10000;  // time between refreshing of the oled

// CONSTANTS
#define SAFE_HIDDEN 0
#define SAFE_VISIBLE 1

#define MAX485_READ LOW
#define MAX485_WRITE HIGH

// Standards der Adressierung (Konvention)
#define RELAY_I2C_ADD 0x3F   // Relay Expander																							*/
#define OLED_I2C_ADD 0x3C    // Ist durch Hardware des OLEDs vorgegeben
#define KEYPAD_I2C_ADD 0x39  // möglich sind 0x38, 39, 3A, 3B, 3D

// RELAY
enum REL_PIN {
    REL_1_PIN,       // 0 First room light
    REL_2_PIN,       // 1 Exit door
    REL_3_PIN,       // 2 Secret door
    REL_SAFE_PIC_PIN,  // 3 MAGNET holding the painting
    REL_5_PIN,       // 4 UV light
    REL_6_PIN,       // 5 Second room light
    REL_7_PIN,       // 6 Empty
    REL_8_PIN        // 7 Peripherals powersupply
};

enum REL_INIT {
    REL_1_INIT = 1,                 // DESCRIPTION OF THE RELAY WIRING
    REL_2_INIT = 1,                 // DESCRIPTION OF THE RELAY WIRING
    REL_3_INIT = 1,                 // DESCRIPTION OF THE RELAY WIRING
    REL_SAFE_PIC_INIT = SAFE_HIDDEN,  // NC = Empty | COM = Magnet +Ve | NO = 12V
    REL_5_INIT = 1,                 // DESCRIPTION OF THE RELAY WIRING
    REL_6_INIT = 1,                 // DESCRIPTION OF THE RELAY WIRING
    REL_7_INIT = 1,                 // DESCRIPTION OF THE RELAY WIRING
    REL_8_INIT = 1                  // DESCRIPTION OF THE RELAY WIRING
};


// == constants
#define REL_AMOUNT 1
const enum REL_PIN relayPinArray[] = {
    REL_SAFE_PIC_PIN};
const byte relayInitArray[] = {
    REL_SAFE_PIC_INIT};

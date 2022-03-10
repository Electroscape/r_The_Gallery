#pragma once
#include "stb_namespace.h"
using namespace stb_namespace;

String title = "Hamburg Gallery";
String versionDate = "02.02.2021";
String version = "version 1.0HH";
String brainName = String("BrRFID");
String relayCode = String("UVL");

#define LIGHT_ON 1
#define LIGHT_OFF 0
#define MAX485_WRITE HIGH
#define MAX485_READ LOW

// I2C Addresses
#define LCD_I2C_ADD 0x27    // Predefined by hardware
#define OLED_ADD 0x3C   // Predefined by hardware
#define RELAY_I2C_ADD 0x3F  // Relay Expander

#define CLR_ORDER NEO_GRB

// RFIDs
#define RFID_AMOUNT 4

//Cards Data
#define RFID_SOLUTION_SIZE 3  // Length of Char data on NFC tag + char '\n' at the end
static char RFID_solutions[4][RFID_SOLUTION_SIZE] = {"AH", "SD", "GF", "PA"};  //

const uint16_t UpdateSignalAfterDelay = 5000; /* Zeit, bis Serial print als Online Signal			*/

// == constants
// RELAY
enum REL_PIN {
    REL_0_PIN,        // 0 First room light
    REL_1_PIN,        // 1 Exit door
    REL_2_PIN,        // 2 Secret door
    REL_3_PIN,        // 3 MAGNET holding the painting
    REL_SCHW_LI_PIN,  // 4 UV light
    REL_ROOM_LI_PIN,  // 5 Second room light
    REL_6_PIN,        // 6 Empty
    REL_7_PIN         // 7 Peripherals powersupply
};

enum REL_INIT {
    REL_0_INIT = 1,                // DESCRIPTION OF THE RELAY WIRING
    REL_1_INIT = 1,                // DESCRIPTION OF THE RELAY WIRING
    REL_2_INIT = 1,                // DESCRIPTION OF THE RELAY WIRING
    REL_3_INIT = 1,                // DESCRIPTION OF THE RELAY WIRING
    REL_SCHW_LI_INIT = LIGHT_OFF,  // NC = Empty | COM = UV +Ve    | NO = 230V
    REL_ROOM_LI_INIT = LIGHT_ON,   // NC = Empty | COM = Light +Ve | NO = 230V
    REL_6_INIT = 1,                // DESCRIPTION OF THE RELAY WIRING
    REL_7_INIT = 1                 // DESCRIPTION OF THE RELAY WIRING
};

#define REL_AMOUNT 2
const enum REL_PIN relayPinArray[] = {
    REL_ROOM_LI_PIN,
    REL_SCHW_LI_PIN};
const enum REL_INIT relayInitArray[] = {
    REL_ROOM_LI_INIT,
    REL_SCHW_LI_INIT};

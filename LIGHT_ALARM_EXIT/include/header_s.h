#pragma once
#include "stb_namespace.h"

String title = "Light & Exit & RFID S v0.5";
String versionDate = "31.05.2021";
String version = "version 1.7";

String brainName = String("Br");

enum riddles {
    LIGHT_RIDDLE,
    EXIT_RIDDLE,
    ALARM_RIDDLE
};
char *secret_passwords[] = {(char *)"1708", (char *)"2381984"};
String relayCodes[] = {String("LIT"), String("EXT"), String("ALA")};
const unsigned long heartbeatFrequency = 5000;

// I2C Addresses
// Relayboard und OLED
#define RELAY_I2C_ADD 0x3F  /* Relay Expander */
#define LIGHT_OLED_ADD 0x3C /* Ist durch Hardware des OLEDs 0x3C */
#define EXIT_OLED_ADD 0x3D  /* Ist durch Hardware des OLEDs */

// Keypad Addresses
#define LIGHT_KEYPAD_ADD 0x38 /* möglich sind 0x38, 39, 3A, 3B, 3D                         */
#define EXIT_KEYPAD_ADD 0x39  /* möglich sind 0x38, 39, 3A, 3B, 3D                         */
// ______________________EINSTELLUNGEN______________________
// RFID

// Relay config
#define REL_AMOUNT 3
// relay BASICS
enum REL_PIN {
    REL_LICHT_PIN,  // 0 First room light
    REL_1_PIN,      // 1 Light 2nd room
    REL_2_PIN,      // 2 UV Light
    REL_ALARM_PIN,  // 3 Alarm
    REL_4_PIN,      // 4 Empty
    REL_5_PIN,      // 5 Fireplace valve
    REL_6_PIN,      // 6 valve holding the painting
    REL_EXIT_PIN    // 7 Exit door lock
};
enum REL_INIT {
    REL_LICHT_INIT = 1,  // DESCRIPTION OF THE RELAY WIRING
    REL_1_INIT = 0,      // NC = Empty | COM = Light +Ve | NO = 230V
    REL_2_INIT = 0,      // NC = Empty | COM = UV +Ve    | NO = 230V
    REL_ALARM_INIT = 1,  // DESCRIPTION OF THE RELAY WIRING
    REL_4_INIT = 1,      // DESCRIPTION OF THE RELAY WIRING
    REL_5_INIT = 1,      // DESCRIPTION OF THE RELAY WIRING
    REL_6_INIT = 1,      // NC = Empty | COM = 24V  | NO = Valve
    REL_EXIT_INIT = 0    // DESCRIPTION OF THE RELAY WIRING
};

const enum REL_PIN relayPinArray[] = {REL_LICHT_PIN, REL_EXIT_PIN, REL_ALARM_PIN};
const enum REL_INIT relayInitArray[] = {REL_LICHT_INIT, REL_EXIT_INIT, REL_ALARM_INIT};

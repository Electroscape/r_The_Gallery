/*==========================================================================================================*/
/**		2CP - TeamEscape - Engineering
 *		by Martin Pek & Abdullah Saei
 *
 *		based on HH  keypad-light-exit v 1.7
 *		- Block after correct solution
 *		- Heartbeat messages
 *		- C++11 arrays coding style
 */
/*==========================================================================================================*/

#include "header_s.h"
using namespace stb_namespace;

// #define RFID_DISABLE 1
// #define OLED_DISABLE 1

//Watchdog timer
#include <avr/wdt.h>
// RFID

// Keypad
#include <Keypad.h> /* Standardbibliothek                                                 */
#include <Keypad_I2C.h>
#include <Password.h>
#include <Wire.h> /* Standardbibliothek                                                 */

// I2C Port Expander
#include <PCF8574.h> /* https://github.com/skywodd/pcf8574_arduino_library - modifiziert!  */

#ifndef RFID_DISABLE
#include <Adafruit_PN532.h>
#endif
// OLED
// #include "SSD1306Ascii.h"             /* https://github.com/greiman/SSD1306Ascii                            */
#ifndef OLED_DISABLE
#include "SSD1306AsciiWire.h" /* https://github.com/greiman/SSD1306Ascii                            */
#endif

#ifndef RFID_DISABLE
#define RFID_1_SS_PIN 8 /* Per Konvention ist dies RFID-Port 1  */
#define RFID_2_SS_PIN 7 /* Per Konvention ist dies RFID-Port 2  */
#define RFID_3_SS_PIN 4 /* Per Konvention ist dies RFID-Port 3  */
#define RFID_4_SS_PIN 2 /* Per Konvention ist dies RFID-Port 4  */
// If using the breakout with SPI, define the pins for SPI communication.
#define PN532_SCK 13
#define PN532_MOSI 11
#define PN532_MISO 12

const byte RFID_SSPins[] = {RFID_1_SS_PIN};

Adafruit_PN532 RFID_0(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[0]);

#define RFID_AMOUNT 1
Adafruit_PN532 RFID_READERS[1] = {RFID_0};  //

int rfid_ticks = 0;
int rfid_last_scan = millis();
const int rfid_scan_delay = 500;
const int rfid_ticks_required = 3;
#endif

bool compass_status = true;

/*==OLED====================================================================================================*/

#ifndef OLED_DISABLE
SSD1306AsciiWire oleds[] = {SSD1306AsciiWire(), SSD1306AsciiWire()};
#endif

/*==KEYPAD I2C==============================================================================================*/
const byte KEYPAD_ROWS = 4;  // Zeilen
const byte KEYPAD_COLS = 3;  // Spalten
const byte KEYPAD_CODE_LENGTH = 4;
const byte KEYPAD_CODE_LENGTH_MAX = 7;

char KeypadKeys[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};

byte KeypadRowPins[KEYPAD_ROWS] = {1, 6, 5, 3};  // Zeilen  - Messleitungen
byte KeypadColPins[KEYPAD_COLS] = {2, 0, 4};     // Spalten - Steuerleitungen (abwechselnd HIGH)

const int keypad_reset_after = 3000;
static unsigned long update_timers[] = {millis(), millis()};

Keypad_I2C keypads[] = {Keypad_I2C(makeKeymap(KeypadKeys), KeypadRowPins, KeypadColPins, KEYPAD_ROWS, KEYPAD_COLS, LIGHT_KEYPAD_ADD, PCF8574),
                        Keypad_I2C(makeKeymap(KeypadKeys), KeypadRowPins, KeypadColPins, KEYPAD_ROWS, KEYPAD_COLS, EXIT_KEYPAD_ADD, PCF8574)};
static int usedkeypad = -1;

// Passwort
Password passwords[] = {Password(secret_passwords[LIGHT_RIDDLE]),
                        Password(secret_passwords[EXIT_RIDDLE])};

// endgame flags
bool endgame[] = {false, false};

/*==PCF8574=================================================================================================*/
Expander_PCF8574 relay;
unsigned long lastHeartbeat = millis();
/*============================================================================================================
//===BASICS===================================================================================================
//==========================================================================================================*/

/**
    * prints header messages on start
    *
    * @return void
    * @param void void
*/
void print_serial_header() {
    printWithHeader("!header_begin", "SYS");
    printWithHeader(title, "SYS");
    printWithHeader(versionDate, "SYS");
    printWithHeader(version, "SYS");
}
/**
    * display oled homescreen
    *
    * @return void
    * @param oled (SSD1306AsciiWire *) pointer to oled object
*/
void oledHomescreen(SSD1306AsciiWire *oled) {
    delay(2);
    oled->clear();
    oled->setFont(Adafruit5x7);
    oled->print("\n\n\n");
    oled->setFont(Verdana12_bold);
    oled->println("  Enter code..");
    delay(2);
}

/*=========================================
//===MOTHER================================
//=========================================*/
/**
    * Initialise relay PCF on Smother board
    *
    * @return true on success
    * @param void void
*/
bool relay_init() {
    Serial.println("initializing relay");
    relay.begin(RELAY_I2C_ADD);

    for (int i = 0; i < REL_AMOUNT; i++) {
        relay.pinMode(relayPinArray[i], OUTPUT);
        relay.digitalWrite(relayPinArray[i], relayInitArray[i]);
    }

    Serial.print(F("\n successfully initialized relay\n"));
    return true;
}

/*========================================
//===KEYPAD===============================
//========================================*/
/**
    * Interrupt based keypad listener
    *
    * @return void
    * @param eKey (KeypadEvent) contains the pressed button
    * @remarks recheck deprecations!
*/
void keypadEvent(KeypadEvent eKey) {
    // ignore inputs from solved riddles
    if (endgame[usedkeypad]) return;

    KeyState state = IDLE;

    state = keypads[usedkeypad].getState();

    switch (state) {
        case PRESSED:
            update_timers[usedkeypad] = millis();

            switch (eKey) {
                case '#':
                    checkPassword();
                    break;
                case '*':
                    passwordReset(usedkeypad);
                    break;
                default:
                    passwords[usedkeypad].append(eKey);
                    printWithHeader(passwords[usedkeypad].guess, relayCodes[usedkeypad]);
                    #ifndef OLED_DISABLE
                        delay(2);
                        oleds[usedkeypad].clear();
                        oleds[usedkeypad].setFont(Adafruit5x7);
                        oleds[usedkeypad].print("\n\n\n");
                        oleds[usedkeypad].setFont(Verdana12_bold);
                        oleds[usedkeypad].print("         ");
                        oleds[usedkeypad].println(passwords[usedkeypad].guess);
                        delay(2);
                    #endif
                    break;
            }
            break;
        default:
            break;
    }
}

/**
    * callback function for keypad press
    *
    * @return void
    * @param eKey (KeypadEvent) the pressed button
*/
void LightKeypadEvent(KeypadEvent eKey) {
    usedkeypad = LIGHT_RIDDLE;
    keypadEvent(eKey);
}
/**
    * callback function for keypad press
    *
    * @return void
    * @param eKey (KeypadEvent) the pressed button
*/
void ExitKeypadEvent(KeypadEvent eKey) {
    usedkeypad = EXIT_RIDDLE;
    keypadEvent(eKey);
}
/**
    * Initialise two keypads
    *
    * @return void
    * @param void void
*/
bool keypad_init() {
    keypads[LIGHT_RIDDLE].addEventListener(LightKeypadEvent);  // Event Listener erstellen
    keypads[EXIT_RIDDLE].addEventListener(ExitKeypadEvent);    // Event Listener erstellen
    for (Keypad_I2C &keypad : keypads) {
        keypad.begin(makeKeymap(KeypadKeys));
        keypad.setHoldTime(5000);
        keypad.setDebounceTime(20);
    }
    return true;
}

/**
    * Listen to keypad press
    *
    * @return void
    * @param void void
*/
void keypad_update() {
    usedkeypad = -1;
    keypads[LIGHT_RIDDLE].getKey();
    keypads[EXIT_RIDDLE].getKey();
}
/**
    * Clears password and displays oled homescreen
    *
    * @return void
    * @param riddle (int) Which riddle to clear {Light, Exit}
*/
void passwordReset(int riddle) {
    passwords[riddle].reset();
#ifndef OLED_DISABLE
    oledHomescreen(&oleds[riddle]);
#endif
}

/**
    * - Checks the password and update the flags
    * - Makes relays action and block the game
    * - Updates the OLEDS
    *
    * @return void
    * @param void void
    * @remarks game is blocked inside at the end
    * @note todo seperate functionalities
*/
void checkPassword() {
    // don't check if there is no password entered
    if (strlen(passwords[usedkeypad].guess) < 1) return;
    // ignore solved riddles
    if (endgame[usedkeypad]) return;

    if (passwords[usedkeypad].evaluate()) {
        printWithHeader("!Correct", relayCodes[usedkeypad]);
#ifndef OLED_DISABLE
        delay(2);
        oleds[usedkeypad].clear();
        oleds[usedkeypad].setFont(Adafruit5x7);
        oleds[usedkeypad].print("\n\n\n");
        oleds[usedkeypad].setFont(Verdana12_bold);
        oleds[usedkeypad].println("   ACCESS GRANTED!");
        delay(2);
#endif
        //0.1 sec delay between correct msg and relay switch
        delay(100);
        relay.digitalWrite(relayPinArray[usedkeypad], !relayInitArray[usedkeypad]);
        endgame[usedkeypad] = true;
        // Turn off the alarm before endgame
        if (usedkeypad == EXIT_RIDDLE) {
            wdt_reset();
            delay(3000);
            relay.digitalWrite(relayPinArray[ALARM_RIDDLE], relayInitArray[ALARM_RIDDLE]);
            // Block arduino after correct solution
            Serial.println("End Game, Please restart arduino!");
            while (true) {
                wdt_reset();
                delay(1000);
            }
        }
    } else {
        printWithHeader("!Wrong", relayCodes[usedkeypad]);
#ifndef OLED_DISABLE
        delay(2);
        oleds[usedkeypad].println("    ACCESS DENIED!");
        delay(2);
#endif
        // Wait to show wrong for a second
        delay(1000);
    }
    if (!endgame[usedkeypad])
        passwordReset(usedkeypad);
}

/*============================================================================================================
//===RFID====================================================================================================
//==========================================================================================================*/

void print_compass_status() {
    if (compass_status) {
        printWithHeader("COMPASS IN SAFE", relayCodes[ALARM_RIDDLE]);
    } else {
        printWithHeader("NO COMPASS", relayCodes[ALARM_RIDDLE]);
    }
}

#ifndef RFID_DISABLE
/**
    * Initialise the RFID
    *
    * @return true in success
    * @param void void
*/
bool RFID_init() {
    for (int i = 0; i < RFID_AMOUNT; i++) {
        delay(20);

        Serial.print(F("initializing reader: "));
        Serial.println(i);
        RFID_READERS[i].begin();
        RFID_READERS[i].setPassiveActivationRetries(5);

        int retries = 0;
        while (true) {
            uint32_t versiondata = RFID_READERS[i].getFirmwareVersion();
            if (!versiondata) {
                Serial.print(F("Didn't find PN53x board\n"));
                if (retries > 5) {
                    Serial.print(F("PN532 startup timed out, restarting\n"));
                    softwareReset();
                }
            } else {
                Serial.print(F("Found chip PN5"));
                Serial.println((versiondata >> 24) & 0xFF, HEX);
                Serial.print(F("Firmware ver. "));
                Serial.print((versiondata >> 16) & 0xFF, DEC);
                Serial.print('.');
                Serial.println((versiondata >> 8) & 0xFF, DEC);
                break;
            }
            retries++;
        }
        // configure board to read RFID tags
        RFID_READERS[i].SAMConfig();
        delay(20);
    }
    return true;
}

/**
    * Checks if there is card exist on the RFID or not
    *
    * @return void
    * @param void void
    * @remarks it contains the msg and relays action
    * @note todo it should only check and return bool
*/
void RFID_alarm_check() {
    delay(2);
    uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};
    uint8_t uidLength;
    uint8_t success = RFID_READERS[0].readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

    if (success) {
        if (rfid_ticks > 0) {
            compass_status = true;
            print_compass_status();
            Serial.print(F("resetting alarm\n"));
            relay.digitalWrite(REL_ALARM_PIN, REL_ALARM_INIT);
        }
        rfid_ticks = 0;
    } else {
        if (rfid_ticks == rfid_ticks_required) {
            compass_status = false;
            print_compass_status();
            Serial.print(F("compass removed, activating alarm\n"));
            relay.digitalWrite(REL_ALARM_PIN, !REL_ALARM_INIT);
            rfid_ticks++;
        } else if (rfid_ticks < rfid_ticks_required + 1) {
            rfid_ticks++;
        }
    }
    delay(2);
}
#endif

/*============================================================================================================
//===OLED=====================================================================================================
//==========================================================================================================*/

#ifndef OLED_DISABLE
/**
    * Initialise two OLEDs
    *
    * @return true on success
    * @param void void
*/
bool oled_init() {
    // &SH1106_128x64 &Adafruit128x64
    Serial.print(F("Oled init\n"));
    oleds[LIGHT_RIDDLE].begin(&SH1106_128x64, LIGHT_OLED_ADD);
    oleds[EXIT_RIDDLE].begin(&SH1106_128x64, EXIT_OLED_ADD);
    for (SSD1306AsciiWire &oled : oleds) {
        oled.set400kHz();
        oled.setScroll(true);
        oledHomescreen(&oled);
        delay(1000);
    }
    return true;
}
#endif

/**
    * Reset password after timeout
    *
    * @return void
    * @param void void
    * @remarks it checks the entered password before clearing it
*/
void keypad_reset() {
    int outdated = 0;
    for (int keypad_no = 0; keypad_no < 2; keypad_no++) {
        if (millis() - update_timers[keypad_no] >= keypad_reset_after) {
            outdated++;
        }
    }
    if (outdated >= 2) {
        for (int keypad_no = 0; keypad_no < 2; keypad_no++) {
            update_timers[keypad_no] = millis();

            if (endgame[keypad_no]) {
                // do nothing if riddle is already solved
                continue;
            } else if (strlen(passwords[keypad_no].guess) > 0) {
                usedkeypad = keypad_no;
                // send result before resetting password
                checkPassword();
                Serial.print("!Timeout ");
                Serial.println(keypad_no);
                printWithHeader("!Reset", relayCodes[keypad_no]);
            } else {
                oledHomescreen(&oleds[keypad_no]);
            }
        }
    }
}

void setup() {
    brainSerialInit();
    Serial.println("WDT endabled");
    wdt_enable(WDTO_8S);
    wdt_reset();

    Serial.println("!setup_begin");

    i2cScanner();

#ifndef OLED_DISABLE
    Serial.print(F("Oleds: ..."));
    if (oled_init()) {
        Serial.println(" light OLED ok...");
    }
#endif

    Serial.print(F("Relay: ..."));
    if (relay_init()) {
        Serial.println(" ok");
    }
    wdt_reset();

#ifndef RFID_DISABLE
    Serial.print(F("RFID: ..."));
    if (RFID_init()) {
        Serial.print(F(" ok\n"));
    }
#endif

    wdt_reset();

    delay(50);

    Serial.print(F("Keypad: ..."));
    if (keypad_init()) {
        Serial.print(F(" ok\n"));
    }
    wdt_reset();
    delay(5);

    Serial.println();
    Serial.println("===================START=====================");
    Serial.println();

    Serial.print(F("!setup_end\n\n"));
    delay(2000);
}

void loop() {
    wdt_reset();
    // Serial.println("keypad_update");
    keypad_update();
    // Serial.println("oled");
    keypad_reset();
    /*
	checkPassword();
	passwordReset();
	*/

#ifndef RFID_DISABLE
    if (millis() - rfid_last_scan > rfid_scan_delay) {
        rfid_last_scan = millis();
        RFID_alarm_check();
    }
#endif

    // Riddle statuses update heartbeat
    if (millis() - lastHeartbeat >= heartbeatFrequency) {
        lastHeartbeat = millis();

        if (!endgame[LIGHT_RIDDLE])
            printWithHeader(passwords[LIGHT_RIDDLE].guess, relayCodes[LIGHT_RIDDLE]);
        if (!endgame[EXIT_RIDDLE]) {
            printWithHeader(passwords[EXIT_RIDDLE].guess, relayCodes[EXIT_RIDDLE]);
            print_compass_status();
        }
    }
}

/*=========================================================================*/
/**		2CP - TeamEscape - Engineering
 *		by Martin Pek & Abdullah Saei
 *
 *		based on HH  keypad-light-exit v 1.5
 *		- Reset only on timeout
 *		- Block brain after correct solution
 *		- add heartbeat pulse if no input
 *		- enable WDT
 *		- add oled homescreen
 */
/*==========================================================================*/

#include "header_st.h"
#include <stb_common.h>

// Watchdog
#include <avr/wdt.h>
// RFID

// Keypad
#include <Keypad.h> /* Standardbibliothek                                                 */
#include <Keypad_I2C.h>
#include <Password.h>                                             

// I2C Port Expander
#include <PCF8574.h> /* https://github.com/skywodd/pcf8574_arduino_library - modifiziert!  */

#define OLED_DISABLE 1

STB STB;

/*==KEYPAD I2C============================================================*/
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

unsigned long updateTimer = millis();
const int keypadResetInterval = 3000;

Keypad_I2C Keypad(makeKeymap(KeypadKeys), KeypadRowPins, KeypadColPins, KEYPAD_ROWS, KEYPAD_COLS, KEYPAD_ADD, PCF8574_MODE);

// Passwort
Password passKeypad = Password(secret_password);

/*==PCF8574============================================================*/
PCF8574 relay;


void keypadEvent(KeypadEvent eKey) {
    KeyState state = IDLE;

    state = Keypad.getState();

    switch (state) {
        case PRESSED:
            updateTimer = millis();
            switch (eKey) {
                case '#':
                    checkPassword();
                    break;

                case '*':
                    passwordReset();
                    break;

                default:
                    passKeypad.append(eKey);
#ifndef OLED_DISABLE
                    oled.clear();
                    oled.setFont(Adafruit5x7);
                    oled.print("\n\n\n");
                    oled.setFont(Verdana12_bold);
                    oled.print("         ");
                    oled.println(passKeypad.guess);
#endif
                    // printWithHeader(passKeypad.guess, relayCode);
                    break;
            }
            break;

        default:
            break;
    }
}

bool keypadInit() {
    Keypad.addEventListener(keypadEvent);  // Event Listener erstellen
    Keypad.begin(makeKeymap(KeypadKeys));
    Keypad.setHoldTime(5000);
    Keypad.setDebounceTime(20);
    return true;
}

void passwordReset() {
    if (strlen(passKeypad.guess) > 0) {
        passKeypad.reset();
        // printWithHeader("!Reset", relayCode);
// Homescreen
#ifndef OLED_DISABLE
        oledHomescreen();
#endif
    }
}

/**
 * Evaluates guessed password
 *
 * @param void
 * @return (bool) true if correct password, false otherwise
 * @remark it stucks the code on correct solution
 * TODO: It should not make relay changes, only evaluates and return true or false!!
 */
bool checkPassword() {
    if (strlen(passKeypad.guess) < 1) return false;
    bool result = passKeypad.evaluate();
    if (result) {
        // printWithHeader("!Correct", relayCode);
#ifndef OLED_DISABLE
        oled.clear();
        oled.setFont(Adafruit5x7);
        oled.print("\n\n\n");
        oled.setFont(Verdana12_bold);
        oled.println("   ACCESS GRANTED!");
#endif
        //0.1 sec delay between correct msg and relay switch
        delay(100);
        relay.digitalWrite(REL_DOOR_PIN, !REL_DOOR_INIT);
        // Brain stuck after riddle solved
        Serial.println("Riddle Solved, Please restart brain!");
        // Block code
        while (true) {
            wdt_reset();
            delay(10);
        }
    } else {
        // printWithHeader("!Wrong", relayCode);
#ifndef OLED_DISABLE
        oled.println("    ACCESS DENIED!");
#endif
        // Wait to show wrong for a second
        delay(1000);
        passwordReset();
    }
    return result;
}

/*============================================================*/
//===OLED=====================================================
//============================================================*/

#ifndef OLED_DISABLE
/**
 * Initialize Oled
 *
 * @param void
 * @return true (bool) on success
 */
bool oled_init() {
    // &SH1106_128x64 &Adafruit128x64
    Serial.print(F("Oled init\n"));
    oled.begin(&SH1106_128x64, OLED_ADD);
    oled.set400kHz();
    oled.setScroll(true);
    oledHomescreen();
    delay(1000);
    return true;
}

void oledHomescreen() {
    oled.clear();
    oled.setFont(Adafruit5x7);
    oled.print("\n\n\n");
    oled.setFont(Verdana12_bold);
    oled.println("  Type your code..");
}
#endif

/**
 * Resets the password after timeout
 *
 * @param void
 * @return void
 * @note Sends No Input heartbeat-like message
 */
void keypadReset() {
    if (millis() - updateTimer >= keypadResetInterval) {
        updateTimer = millis();

        if (strlen(passKeypad.guess) > 0) {
            checkPassword();
            // printWithHeader("!Reset", relayCode);
            Serial.println("!Timeout");
        } else {
            // Act as heartbeat pulse
            // printWithHeader("", relayCode);
        }
        passwordReset();
    }
}

void setup() {

    STB.begin();
    STB.dbgln("WDT endabled");
    wdt_enable(WDTO_8S);

    STB.i2cScanner();

#ifndef OLED_DISABLE
    Serial.print(F("Oleds: ..."));
    if (oled_init()) {
        Serial.println(" light OLED ok...");
    }
#endif

     STB.relayInit(relay, relayPinArray, relayInitArray, REL_AMOUNT);
    wdt_reset();

    delay(50);

    Serial.print(F("Keypad: ..."));
    if (keypadInit()) {
        Serial.print(F(" ok\n"));
    }
    wdt_reset();
    delay(5);

    STB.printSetupEnd();
}

void loop() {
    wdt_reset();

    Keypad.getKey();
    keypadReset();
}

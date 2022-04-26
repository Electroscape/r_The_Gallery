/**
 * @file SECRET_DOOR.ino
 * @author Martin Pek (martin.pek@web.de)
 * @brief 
 * @version 0.1
 * @date 2022-04-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */


#include "header_st.h"
#include <stb_common.h>

// Watchdog
#include <avr/wdt.h>
// RFID

// Keypad
#include <Keypad.h> /* Standardbibliothek                                                 */
#include <Keypad_I2C.h>
#include <Password.h>                                             

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

    STB.defaultOled.setFont(Verdana12_bold);

    STB.printSetupEnd();
    wdt_reset();
    delay(3000);
    oledHomescreen();
}

void loop() {
    wdt_reset();

    Keypad.getKey();
    keypadReset();
}

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
                    STB.defaultOled.clear();
                    STB.defaultOled.setFont(Adafruit5x7);
                    STB.defaultOled.print("\n\n\n");
                    STB.defaultOled.setFont(Verdana12_bold);
                    STB.defaultOled.print("         ");
                    STB.dbgln(passKeypad.guess);
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
        oledHomescreen();
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
        STB.printWithHeader("!Correct", relayCode);

        STB.defaultOled.clear();
        STB.defaultOled.setFont(Adafruit5x7);
        STB.defaultOled.print("\n\n\n");
        STB.defaultOled.setFont(Verdana12_bold);
        STB.dbgln("   ACCESS GRANTED!");

        //0.1 sec delay between correct msg and relay switch
        delay(100);
        relay.digitalWrite(REL_DOOR_PIN, !REL_DOOR_INIT);
        // Brain stuck after riddle solved
        Serial.println("Riddle Solved, Please restart brain!");
        // Block code
        while (true) {
            wdt_reset();
            delay(500);
        }
    } else {
        // printWithHeader("!Wrong", relayCode);
        STB.dbgln("    ACCESS DENIED!");
        // Wait to show wrong for a second
        delay(1000);
        passwordReset();
    }
    return result;
}

/*============================================================*/
//===OLED=====================================================
//============================================================*/

void oledHomescreen() {
    Serial.println("Homescreen");
    STB.defaultOled.clear();
    STB.defaultOled.setFont(Adafruit5x7);
    STB.defaultOled.print("\n\n\n");
    STB.defaultOled.setFont(Verdana12_bold);
    STB.defaultOled.println("  Type your code..");
}

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


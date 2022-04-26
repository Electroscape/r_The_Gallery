/**
 * @file COLOR_CODE.ino
 * @author Martin Pek (martin.pek@web.de)
 * @brief 
 * @version 0.1
 * @date 2022-04-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <stb_common.h>
#include "header_st.h"

/*==INCLUDE==============================================*/

// OLED
#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>

// Keypad
#include <Keypad.h>
#include <Keypad_I2C.h>
#include <Wire.h>

// Password
#include <Password.h>

/*==OLED======================================*/

STB STB;

bool UpdateOLED = true;
unsigned long oledLastUpdate = millis();

// Konfiguration fuer Sparkfun-Keypads
// Keypad 1 2 3 4 5 6 7
// Chip   0 1 2 3 4 5 6
byte KeypadRowPins[KEYPAD_ROWS] = {4};           // Zeilen  - Messleitungen
byte KeypadColPins[KEYPAD_COLS] = {0, 1, 2, 3};  // Spalten - Steuerleitungen (abwechselnd HIGH)

bool KeypadCodeCorrect = false;
bool KeypadCodeWrong = false;
bool endGame = false;                    // Only true when correct solution after smiley face
unsigned long keypadActivityTimer = millis();  // ResetTimer

Keypad_I2C MyKeypad(makeKeymap(KeypadKeys), KeypadRowPins, KeypadColPins,
                    KEYPAD_ROWS, KEYPAD_COLS, KEYPAD_I2C_ADD, PCF8574_MODE);
Password passLight = Password(secret_password);  // Schaltet das Licht im Büro an

/*==PCF8574===================================*/
PCF8574 relay, KeypadFix;

unsigned long lastHeartbeat = millis();


void setup() {
    STB.begin();

    STB.i2cScanner();

    STB.dbg("Keypad init...");
    if (KeypadInit()) {STB.dbgln("success");}
    
    STB.relayInit(relay, relayPinArray, relayInitArray, REL_AMOUNT);

    delay(2000);
    STB.printSetupEnd();
}

void loop() {
    // Heartbeat message
    if (millis() - lastHeartbeat >= heartbeatFrequency) {
        lastHeartbeat = millis();
        // printWithHeader(passLight.guess, relayCode);
    }

    KeypadUpdate();
#ifndef OLED_DISABLE
    OLEDUpdate();  // periodic refresh
#endif

    // Block the arduino if correct solution
    if (endGame) {
        // printWithHeader("Game Complete", "SYS");
        STB.dbgln("Game Complete\nWaiting for restart");
        Serial.println("End Game, Please restart the arduino!");
        while (true) {
            delay(500);
        }
    }
}

void OLEDUpdate() {
    if (((millis() - oledLastUpdate) < oledUpdateInterval)) { return;}

    oledLastUpdate = millis();
    if (strlen(passLight.guess) >= 1) {
        OLEDshowPass();
    } else {
        OLEDIdlescreen();
    }
}

void OLEDIdlescreen() {
    STB.defaultOled.clear();
    STB.defaultOled.setFont(Adafruit5x7);
    STB.defaultOled.print("\n\n\n");
    STB.defaultOled.setFont(Arial_bold_14);
    STB.defaultOled.println("  Enter Code..");
    oledLastUpdate = millis();
}

// Update Oled with current password guess
void OLEDshowPass() {
    STB.dbgln(passLight.guess);
    oledLastUpdate = millis();
}

/*=========================================================
//===KEYPAD================================================
//=======================================================*/
/**
 * Initialize Keypad
 *
 * @param void
 * @return void
 * @note set PCF to high input to activate Pull-up resistors first, then initialize keypad library
 */
bool KeypadInit() {
    KeypadFix.begin(KEYPAD_I2C_ADD);
    for (int i = 0; i <= 7; i++) {
        KeypadFix.pinMode(i, INPUT);
        KeypadFix.digitalWrite(i, HIGH);
    }
    delay(100);
    MyKeypad.addEventListener(keypadEvent);  // Event Listener erstellen
    MyKeypad.begin(makeKeymap(KeypadKeys));
    MyKeypad.setHoldTime(5000);
    MyKeypad.setDebounceTime(keypadDebounceTime);
    return true;
}

/**
 * Evaluates password after TIMEOUT and makes relay action
 *
 * @param void
 * @return void
 */
void KeypadUpdate() {
    MyKeypad.getKey();

    if (passLight.evaluate() || (millis() - keypadActivityTimer) > keypadCheckingInterval) {
        keypadActivityTimer = millis();
        if (strlen(passLight.guess) >= 1) {
            checkPassword();
        } else {
            OLEDIdlescreen();
        }
    }
}

/**
 * Listens to keypad inputs
 *
 * @param eKey Stores the pressed button
 * @return void
 */
void keypadEvent(KeypadEvent eKey) {
    switch (MyKeypad.getState()) {
        case PRESSED:
            UpdateOLED = true;
            keypadActivityTimer = millis();

            switch (eKey) {
                default:
                    passLight.append(eKey);
                    STB.dbgln(passLight.guess);
                    break;
            }
            break;

        case HOLD:
            Serial.print("HOLD: ");
            Serial.println(eKey);
            break;

        default:
            break;
    }
}

/**
 * Initialise 8 Relays on I2C PCF
 *
 * @param void
 * @return true when done
 */
bool relay_init() {
    Serial.println("initializing relay");
    relay.begin(RELAY_I2C_ADD);

    // init all 8,, they are physically disconnected anyways
    for (int i = 0; i < REL_AMOUNT; i++) {
        relay.pinMode(relayPinArray[i], OUTPUT);
        relay.digitalWrite(relayPinArray[i], relayInitArray[i]);
        Serial.print("     ");
        Serial.print("Relay [");
        Serial.print(relayPinArray[i]);
        Serial.print("] set to ");
        Serial.println(relayInitArray[i]);
    }
    Serial.println();
    Serial.println("successfully initialized relay");
    return true;
}

/**
 * Evaluates guessed password
 *
 * @param void
 * @return (bool) true if correct password, false otherwise
 * @remark It only evaluates and return true or false!!
 */
void checkPassword() {
    if (passLight.evaluate()) {
        // printWithHeader("!Correct", relayCode);
        delay(4000);
		relay.digitalWrite(REL_SAFE_PIC_PIN, SAFE_VISIBLE);
        STB.dbgln("\nCorrect");
        endGame = true;
    } else {
        STB.dbgln("\nWrong");
        passwordReset();
        // printWithHeader("!Wrong", relayCode);
        oledLastUpdate = millis();
    }
}

/**
 * Clear password guess
 *
 * @param void
 * @return void
 */
void passwordReset() {
	// printWithHeader("!Reset", relayCode);
	passLight.reset();
}

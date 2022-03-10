/*=======================================================*/
/**		2CP - TeamEscape - Engineering
 *		by Abdullah Saei & Martin Pek
 *
 *		v2.0 beta
 *      - Accept passwords longer than length
 *      - Logic separation OLED and keypad
 *		- Use OLED SH1106
 *      - Remove deprecations
 *      - Block after correct solution
 *
 */
/*=======================================================*/
#include <stb_namespace.h>

#include "header_s.h"
using namespace stb_namespace;

/*==INCLUDE==============================================*/
// I2C Port Expander
#include <PCF8574.h>

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
SSD1306AsciiWire oled;
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
                    KEYPAD_ROWS, KEYPAD_COLS, KEYPAD_I2C_ADD, PCF8574);
Password passLight = Password(secret_password);  // Schaltet das Licht im Büro an

/*==PCF8574===================================*/
Expander_PCF8574 relay, KeypadFix;

unsigned long lastHeartbeat = millis();

/*================================================
//===SETUP========================================
//==============================================*/
void setup() {
    brainSerialInit();
    Serial.println(title);
    Serial.println(version);

    i2cScanner();

#ifndef OLED_DISABLE
    OLED_Init();
#endif

    Keypad_Init();
    relay_init();

    delay(2000);
    printWithHeader("Setup Complete", "SYS");
}

/*=================================================
//===LOOP==========================================
//===============================================*/
void loop() {
    // Heartbeat message
    if (millis() - lastHeartbeat >= heartbeatFrequency) {
        lastHeartbeat = millis();
        printWithHeader(passLight.guess, relayCode);
    }

    Keypad_Update();
#ifndef OLED_DISABLE
    OLED_Update();  // periodic refresh
#endif

    // Block the arduino if correct solution
    if (endGame) {
        printWithHeader("Game Complete", "SYS");
        Serial.println("End Game, Please restart the arduino!");
        while (true) {
            delay(500);
        }
    }
}

/*===================================================
//===OLED============================================
//=================================================*/

#ifndef OLED_DISABLE

void OLED_Init() {
    oled.begin(&SH1106_128x64, OLED_I2C_ADD);
    oled.set400kHz();
    oled.setScroll(true);
    oled.setFont(Arial_bold_14);
    oled.println("\n\n   booting.... \n   farbcode");
}

void OLED_Update() {
    if (((millis() - oledLastUpdate) < oledUpdateInterval)) { return;}

    oledLastUpdate = millis();
    if (strlen(passLight.guess) >= 1) {
        OLED_showPass();
    } else {
        OLED_Idlescreen();
    }
}

void OLED_Idlescreen() {
    oled.clear();
    oled.setFont(Adafruit5x7);
    oled.print("\n\n\n");
    oled.setFont(Arial_bold_14);
    oled.println("  Enter Code..");
    oledLastUpdate = millis();
}

// Update Oled with current password guess
void OLED_showPass() {
    oled.clear();
    oled.setFont(Adafruit5x7);
    oled.println();
    oled.setFont(Arial_bold_14);
    oled.println();
    oled.print(F("  "));
    oled.print(passLight.guess);
    oledLastUpdate = millis();
}

#endif

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
void Keypad_Init() {
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
}

/**
 * Evaluates password after TIMEOUT and makes relay action
 *
 * @param void
 * @return void
 */
void Keypad_Update() {
    MyKeypad.getKey();

    if (passLight.evaluate() || (millis() - keypadActivityTimer) > keypadCheckingInterval) {
        keypadActivityTimer = millis();
        if (strlen(passLight.guess) >= 1) {
            checkPassword();
        } else {
            OLED_Idlescreen();
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
                    printWithHeader(passLight.guess, relayCode);
					OLED_showPass();
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
        printWithHeader("!Correct", relayCode);
        delay(4000);
		relay.digitalWrite(REL_SAFE_PIC_PIN, SAFE_VISIBLE);
        oled.println("          Correct");
        endGame = true;
    } else {
        oled.println("\n      Wrong");
        passwordReset();
        printWithHeader("!Wrong", relayCode);
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
	printWithHeader("!Reset", relayCode);
	passLight.reset();
}

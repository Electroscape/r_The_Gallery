/**
 * @file m_Acces.ino
 * @author Martin Pek (martin.pek@web.de)
 * @brief access module supports RFID and Keypad authentication with oled&buzzer feedback
 * @version 0.1
 * @date 2022-09-09
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "header_st.h"                                     
#include <Keypad_I2C.h>
#include <stb_brain.h>
#include <avr/wdt.h>
#include <PCF8574.h> /* https://github.com/skywodd/pcf8574_arduino_library - modifiziert!  */

#include <Password.h>
#include <stb_rfid.h>
#include <stb_keypadCmds.h>
#include <stb_oledCmds.h>


/*
build for lib_arduino 0.6.7 onwards
TODO:
 - periodic updates on the password? everytime its polled? 

🔲✅
Fragen and access module Requirements
 - ✅ Dynamischer Headline text wie "Enter Code", "Welcome" etc over cmd from Mother
 - ✅ Necessity toggle between RFID and Keypad
 - ✅  wrong rfid may trigger twice
*/


STB_BRAIN Brain;

Keypad_I2C Keypad(makeKeymap(KeypadKeys), KeypadRowPins, KeypadColPins, KEYPAD_ROWS, KEYPAD_COLS, KEYPAD_ADD, PCF8574_MODE);

// the Evaluation is done on the Mother, may be kept for convenience or removed later
Password passKeypad = Password(dummyPassword);
unsigned long lastKeypadAction = millis();




void loop() {

    wdt_reset();
    Keypad.getKey();


    if (!Brain.slaveRespond()) {
        return;
    }

    while (Brain.STB_.rcvdPtr != NULL) {
        // Serial.println("Brain.STB_.rcvdPtr");
        interpreter();
        Brain.nextRcvdLn();
    }
}


void interpreter() {
    if (checkForValid()) {return;}
}



// checks keypad feedback, its only correct/incorrect
bool checkForValid() {

    // Serial.print("checking: ");
    // Serial.println(Brain.STB_.rcvdPtr);
    
    if (strncmp(keypadCmd.c_str(), Brain.STB_.rcvdPtr, keypadCmd.length()) == 0) {
        Brain.sendAck();
        // Serial.println("incoming keypadCmd");
        // do i need a fresh char pts here?
        char *cmdPtr = strtok(Brain.STB_.rcvdPtr, KeywordsList::delimiter.c_str());
        cmdPtr = strtok(NULL, KeywordsList::delimiter.c_str());
        int cmdNo;
        sscanf(cmdPtr, "%d", &cmdNo);

        if (cmdNo == KeypadCmds::correct) {
            STB_OLED::writeToLine(&Brain.STB_.defaultOled, 3, F("valid"), true);
            // tone(BUZZER_PIN, 1700, 1000);
        } else {

            STB_OLED::writeToLine(&Brain.STB_.defaultOled, 3, F("invalid"), true);
            passKeypad.reset();
        }
        return true;
    }
    return false;
}


// --- Keypad


void keypad_init() {
    Brain.STB_.dbgln(F("Keypad: ..."));
    Keypad.addEventListener(keypadEvent);  // Event Listener erstellen
    Keypad.begin(makeKeymap(KeypadKeys));
    Keypad.setHoldTime(5000);
    Keypad.setDebounceTime(20);
    Brain.STB_.dbgln(F(" ok\n"));
}


void keypadEvent(KeypadEvent eKey) {
    KeyState state = IDLE;

    state = Keypad.getState();

    if (state == PRESSED) {
        lastKeypadAction = millis();
    }

    switch (state) {
        case PRESSED:
            switch (eKey) {

                default:
                    if (strlen(passKeypad.guess) <= KEYPAD_CODE_LENGTH_MAX) {
                        passKeypad.append(eKey);
                        // Serial.println(passKeypad.guess);
                        checkPassword();
                    }
                    // STB_OLED::writeToLine(&Brain.STB_.defaultOled, 2, passKeypad.guess, true);
                    break;
            }
            break;

        default:
            break;
    }
}

/**
 * @brief sends the password to the Mother for evaluation
 */
void checkPassword() {
    if (strlen(passKeypad.guess) < 1) return;
    Brain.clearBuffer();
    char msg[20] = ""; 
    strcpy(msg, keypadCmd.c_str());
    strcat(msg, KeywordsList::delimiter.c_str());
    char noString[3] = "";
    sprintf(noString, "%i", KeypadCmds::evaluate);
    strcat(msg, noString);
    strcat(msg, KeywordsList::delimiter.c_str());
    strcat(msg, passKeypad.guess);
    Brain.addToBuffer(msg, true); 
}


void setup() {
    
    // starts serial and default oled
    
    Brain.begin();
    wdt_enable(WDTO_8S);

    Brain.setSlaveAddr(0);
    keypad_init();
    Brain.STB_.printSetupEnd();
}




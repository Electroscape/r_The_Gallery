/**
 * @file m_access_mother.cpp
 * @author Martin Pek (martin.pek@web.de)
 * @brief 
 * @version 0.1
 * @date 2022-09-09
 * 
 * @copyright Copyright (c) 2022
 * 
 *  TODO: 
 */


#include <stb_mother.h>
#include <stb_keypadCmds.h>
#include <stb_mother_IO.h>

#include "header_st.h"

STB_MOTHER_IO MotherIO;

STB_MOTHER Mother;
int stage = gameLive;
// since stages are single binary bits and we still need to d some indexing
int stageIndex = 0;
// doing this so the first time it updates the brains oled without an exta setup line
int lastStage = -1;


const unsigned long PASSWORD_TIMEOUT_MS = 2000;

size_t lastPasswordLength = 0;
unsigned long passwordLastChanged = 0;
bool passwordPending = false;


// since stages are binary bit being shifted we cannot use them to index
void setStageIndex() {
    for (int i=0; i<StageCount; i++) {
        if (stage <= 1 << i) {
            stageIndex = i;
            /*            
            Serial.print("stageIndex:");
            Serial.println(stageIndex);
            delay(1000);
            */
            return;
        }
    }
    Serial.println(F("STAGEINDEX ERRROR!"));
    delay(16000);
}


void blinkIncorrect() {
    Mother.STB_.defaultOled.println(F("blinkIncorrect"));
    Mother.relayWrite(relays::leds, closed);
    delay(200); 
    Mother.relayWrite(relays::leds, open);
    delay(100);
    Mother.relayWrite(relays::leds, closed);
    delay(200); 
    Mother.relayWrite(relays::leds, open);
    delay(100);
    Mother.relayWrite(relays::leds, closed);
    delay(200); 
    Mother.relayWrite(relays::leds, open);
    delay(100);
    Mother.relayWrite(relays::leds, closed);
    delay(200); 
    Mother.relayWrite(relays::leds, open);
}

void blinkCorrect() {
    Mother.STB_.defaultOled.println(F("blinkIncorrect"));
    Mother.relayWrite(relays::leds, closed);
    delay(300); 
    Mother.relayWrite(relays::leds, open);
    delay(200);
    Mother.relayWrite(relays::leds, closed);
    delay(300); 
    Mother.relayWrite(relays::leds, open);
    delay(200);
    Mother.relayWrite(relays::leds, closed);
}

void displayInput(char* password) {
    Mother.STB_.defaultOled.clear();
    Mother.STB_.defaultOled.println(F("Colourcode"));
    Mother.STB_.defaultOled.println(password);
}

// @todo: consider refcatoring on wrong pass?
void checkPasswordTimeout() {
    if (!passwordPending) {
        return;
    }

    if (millis() - passwordLastChanged < PASSWORD_TIMEOUT_MS) {
        return;
    }

    char msg[32] = "";
    char noString[12] = "";

    strcpy(msg, keypadCmd.c_str());
    strcat(msg, KeywordsList::delimiter.c_str());

    sprintf(noString, "%d", KeypadCmds::wrong);

    strcat(msg, noString);

    msg[strlen(msg) - 1] = '\0';
    strcat(msg, noString);

    Mother.sendCmdToSlave(msg);
    blinkIncorrect();


    passwordPending = false;
    lastPasswordLength = 0;
}

bool passwordInterpreter(char* password) {
    Mother.STB_.defaultOled.clear();

    Serial.print(F("passwordInterpreter: ["));
    Serial.print(password);
    Serial.println(F("]"));

    for (int passNo = 0; passNo < PasswordAmount; passNo++) {

        Serial.print(F("Checking "));
        Serial.print(passNo);
        Serial.print(F(": "));
        Serial.println(passwords[passNo]);

        if (passwordMap[passNo] & stage) {
            if (strcmp(passwords[passNo], password) == 0) {
                stage = stage << 1;
                return true;
            }
            Serial.println(F("  Password mismatch"));
        }
    }

    Serial.println(F("No password matched"));
    return false;
}


/**
 * @brief handles evalauation of codes and sends the result to the access module
 * @param cmdPtr 
*/
void handleResult(char *cmdPtr) {
    cmdPtr = strtok(NULL, KeywordsList::delimiter.c_str());
    // && (cmdPtr != NULL was in here before

    if (cmdPtr == NULL || cmdPtr[0] == '\0') {
        return;
    }

    size_t len = strlen(cmdPtr);
    // replaces whitespace with nullterminator
    while (cmdPtr[strlen(cmdPtr) - 1] == ' ') {
        cmdPtr[strlen(cmdPtr) - 1] = '\0';
    }

    if (len == 0) { return; }

    passwordPending = true;

    if (len != lastPasswordLength) {
        lastPasswordLength = len;
        passwordLastChanged = millis();
        

        Serial.print(F("Password length changed: "));
        Serial.println(len);
    }

    if (passwordInterpreter(cmdPtr)) {
        delay(1000);
        blinkCorrect();
    }
}

void checkForKeypad() {
    Serial.println(Mother.STB_.rcvdPtr);
    if (strncmp(KeywordsList::keypadKeyword.c_str(), Mother.STB_.rcvdPtr, KeywordsList::keypadKeyword.length() ) != 0) {
        char *cmdPtr = strtok(Mother.STB_.rcvdPtr, KeywordsList::delimiter.c_str());
        handleResult(cmdPtr);
        wdt_reset();
    } 
}



void interpreter() {
    while (Mother.nextRcvdLn()) {
        checkForKeypad();
    }
}



void stageActions() {
    wdt_reset();
 
    switch (stage) {
        case stages::solved:
            Mother.STB_.defaultOled.println(F("Riddle Solved!"));
            blinkCorrect();
        break;
    }
    wdt_reset();
}


/**
 * @brief  triggers effects specific to the given stage, 
 * room specific excecutions can happen here
 * TODO: avoid reposts of setflags, but this is an optimisation
*/
void stageUpdate() {
    if (lastStage == stage) { return; }
    setStageIndex();
        
    // check || stageIndex >= int(sizeof(stages))
    if (stageIndex < 0 || stageIndex > 1 << StageCount) {
        Serial.println(F("Stages out of index!"));
        delay(5000);
        wdt_reset();
    }

    lastStage = stage;
    stageActions();
}


void setup() {
    // starts serial and default oled
    Mother.begin();
    Mother.relayInit(relayPinArray, relayInitArray, relayAmount);
    Mother.STB_.defaultOled.clear();
    Mother.STB_.defaultOled.println(F("Farbrätsel"));
    // MotherIO.ioInit(intputArray, sizeof(intputArray), outputArray, sizeof(outputArray));

    Serial.println(F("WDT endabled"));
    wdt_enable(WDTO_8S);

    // technicall 2 but no need to poll the 2nd as it only receives the colour
    Mother.rs485SetSlaveCount(1);
    wdt_reset();
}

void loop() {
    Mother.rs485PerformPoll();
    interpreter();
    checkPasswordTimeout();
    stageUpdate();
    wdt_reset();
    delay(5);
}





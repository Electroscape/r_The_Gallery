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
int lastInput = 0;


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
    Mother.motherRelay.digitalWrite(relays::leds, open);
    delay(200); 
    Mother.motherRelay.digitalWrite(relays::leds, closed);
    delay(100);
    Mother.motherRelay.digitalWrite(relays::leds, open);
    delay(200); 
    Mother.motherRelay.digitalWrite(relays::leds, closed);
    delay(100);
    Mother.motherRelay.digitalWrite(relays::leds, open);
    delay(200); 
    Mother.motherRelay.digitalWrite(relays::leds, closed);
    delay(100);
    Mother.motherRelay.digitalWrite(relays::leds, open);
    delay(200); 
    Mother.motherRelay.digitalWrite(relays::leds, closed);
}

void blinkCorrect() {
    Mother.motherRelay.digitalWrite(relays::leds, open);
    delay(300); 
    Mother.motherRelay.digitalWrite(relays::leds, closed);
    delay(200);
    Mother.motherRelay.digitalWrite(relays::leds, open);
    delay(300); 
    Mother.motherRelay.digitalWrite(relays::leds, closed);
    delay(200);
    Mother.motherRelay.digitalWrite(relays::leds, open);
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
    blinkIncorrect();
    return false;
}


/**
 * @brief handles evalauation of codes and sends the result to the access module
 * @param cmdPtr 
*/
void handleResult(char *cmdPtr) {
    cmdPtr = strtok(NULL, KeywordsList::delimiter.c_str());
    // && (cmdPtr != NULL was in here before

    // replaces whitespace with nullterminator
    while (cmdPtr[strlen(cmdPtr) - 1] == ' ') {
        cmdPtr[strlen(cmdPtr) - 1] = '\0';
    }
    
    passwordInterpreter(cmdPtr);
}

void checkForKeypad() {


    Serial.println(Mother.STB_.rcvdPtr);
    if (strncmp(KeywordsList::keypadKeyword.c_str(), Mother.STB_.rcvdPtr, KeywordsList::keypadKeyword.length() ) != 0) {
        return;
    } 
    char *cmdPtr = strtok(Mother.STB_.rcvdPtr, KeywordsList::delimiter.c_str());
    handleResult(cmdPtr);
    wdt_reset();
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
    stageUpdate();
    wdt_reset();
    delay(5);
}





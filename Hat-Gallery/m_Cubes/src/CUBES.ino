/**
 * @file m_Cubes.cpp
 * @author Martin Pek (martin.pek@web.de)
 * @brief controls the sockets with RFIDs and Lights, switches regular and UV light 
 * @version 0.1
 * @date 2025-12-29
 * 
 * @copyright Copyright (c) 2022
 * 
 *  TODO: 
 * 
 */


#include <stb_mother.h>
#include <stb_keypadCmds.h>
#include <stb_oledCmds.h>
#include <stb_mother_ledCmds.h>
#include <stb_mother_IO.h>

#include "header_st.h"

// using the reset PCF for this
STB_MOTHER Mother;
STB_MOTHER_IO MotherIO;

int stage = live;
//int stage = idle; //for debugging
// since stages are single binary bits and we still need to do some indexing
int stageIndex = 0;
// doing this so the first time it updates the brains oled without an exta setup line
int lastStage = -1;

int cardsPresent = 0;
int cardsCorrect = 0;

const unsigned long cardResetTimeout = 1000; // ms
unsigned long lastCardReport[brain_cnt] = {0};


/**
 * @brief Set the Stage Index object
*/
void setStageIndex() {
    for (int i=0; i<stages::stagecount; i++) {
        if (stage <= 1 << i) {
            stageIndex = i;
            Serial.print(F("stageIndex:"));
            Serial.println(stageIndex);
            delay(1000);
            return;
        }
    }
    Serial.println(F("STAGEINDEX ERRROR!"));
    wdt_reset();
    delay(16000);
}


/**
 * @brief  consider just using softwareReset
*/
void gameReset() {
    stage = live;
    for (int relayNo=0; relayNo < relayAmount; relayNo++) {
        Mother.motherRelay.digitalWrite(relayNo, relayInitArray[relayNo]);
    }
    LED_CMDS::setAllStripsToClr(Mother, brains::leds, LED_CMDS::clrBlack, 100);
}


void displayCardStatus() {
    Mother.STB_.defaultOled.clear();

    Mother.STB_.defaultOled.println(F("CARD STATUS"));

    Mother.STB_.defaultOled.print(F("Present: "));
    for (int i = brain_cnt - 1; i >= 0; i--) {
        Mother.STB_.defaultOled.print((cardsPresent >> i) & 1);
    }
    Mother.STB_.defaultOled.println();

    Mother.STB_.defaultOled.print(F("Correct: "));
    for (int i = brain_cnt - 1; i >= 0; i--) {
        Mother.STB_.defaultOled.print((cardsCorrect >> i) & 1);
    }
    Mother.STB_.defaultOled.println();
}

/**
 * @brief  
 * check if the given card is on the correct spot or not, also switches the colour of the sockets 
 * @param passNo 
*/
void checkSolution(int passNo, int slave) {

    LED_CMDS::setStripToClr(Mother, brains::leds, LED_CMDS::clrYellow, 100, slave);

    lastCardReport[slave] = millis();
    cardsPresent |= (1 << slave);
    if (passNo == slave) {
        cardsCorrect |= (1 << slave);
    } else {
        cardsCorrect &= ~(1 << slave);
    }
    displayCardStatus();
}


bool passwordInterpreter(char* password) {
    int slave = Mother.getPolledSlave();
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
                checkSolution(passNo, slave);
                return true;
            }
            Serial.println(F("  Password mismatch"));
        }
    }

    Serial.println(F("No password matched"));
    return false;
}


void resetCardStatusIfTimeout() {

    for (int slave = 0; slave < brain_cnt; slave++) {

        if (millis() - lastCardReport[slave] >= cardResetTimeout) {

            // Only do anything if this reader currently has a card registered
            if (cardsPresent & (1 << slave)) {

                Serial.print(F("Reader "));
                Serial.print(slave);
                Serial.println(F(" timeout - clearing card"));

                // reseeting the bits of affected reader
                cardsPresent &= ~(1 << slave);
                cardsCorrect &= ~(1 << slave);
                LED_CMDS::setStripToClr(Mother, brains::leds, LED_CMDS::clrBlack, 100, slave);

                displayCardStatus();
            }
        }
    }
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
    if (cardsCorrect == (1 << brain_cnt) - 1 ) {
        // Serial.println(F("ALL CARDS CORRECT -> SOLVED"));
        stage = solved;
        // setting to 0 does not stop its, need to update motehr in library
        // Mother.rs485SetSlaveCount(0);
    }
}



// again good candidate for a mother specific lib
bool checkForRfid() {

    if (strncmp(KeywordsList::rfidKeyword.c_str(), Mother.STB_.rcvdPtr, KeywordsList::rfidKeyword.length() ) != 0) {
        return false;
    } 
    char *cmdPtr = strtok(Mother.STB_.rcvdPtr, KeywordsList::delimiter.c_str());
    handleResult(cmdPtr);
    wdt_reset();
    return true;
}


void interpreter() {
    while (Mother.nextRcvdLn()) {
        checkForRfid();
    }
}


void stageActions() {
    wdt_reset();
 
    switch (stage) {
        case stages::solved:
            Mother.STB_.defaultOled.clear();
            Mother.STB_.defaultOled.println(F("Riddle Solved!"));
            Mother.relayWrite(uv, !uvInit);
            Mother.relayWrite(light, !lightInit);
            LED_CMDS::setAllStripsToClr(Mother, brains::leds, LED_CMDS::clrGreen, 100);
            
            delay(5000);
            Mother.relayWrite(light, lightInit);
            LED_CMDS::setAllStripsToClr(Mother, brains::leds, LED_CMDS::clrGreen, 100);
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
    MotherIO.outputReset();
    Serial.print("Stage is:");
    Serial.println(stage);

    setStageIndex();
        
    // check || stageIndex >= int(sizeof(stages))
    if (stageIndex < 0) {
        Serial.println(F("Stages out of index!"));
        delay(5000);
        wdt_reset();
    }
    // important to do this before stageActions! otherwise we skip stages
    lastStage = stage;

    // Mother.setFlags(0, flagMapping[stageIndex]);
    delay(100);
    stageActions();
}

void setup() {

    Mother.begin();
    // starts serial and default oled
    Mother.relayInit(relayPinArray, relayInitArray, relayAmount);
    // MotherIO.ioInit(intputArray, sizeof(intputArray), outputArray, sizeof(outputArray));

    // Serial.println("WDT endabled");
    wdt_enable(WDTO_8S);

    // technicall 2 but no need to poll the 2nd 
    Mother.rs485SetSlaveCount(3);

    setStageIndex();

    /*
    Mother.setFlags(0, flagMapping[stageIndex]);
    Mother.setupComplete(0);
    */
    /*
    int argsCnt = 2;
    int ledCount[argsCnt] = {0, 3};
    Mother.sendSetting(1, settingCmds::ledCount, ledCount, argsCnt);
    Mother.setupComplete(1);
    */

    // smalle dealay to not load up the fuse by switching on too many devices at once
    wdt_reset();
    delay(1000);
    gameReset();
}


void loop() {
    if (stage == live) {
        Mother.rs485PerformPoll();
        interpreter();
        resetCardStatusIfTimeout();
    }

    stageUpdate();
    // handleInputs(); 
    wdt_reset();
}





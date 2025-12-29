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
PCF8574 inputPCF;
STB_MOTHER Mother;
STB_MOTHER_IO MotherIO;

int stage = live;
//int stage = idle; //for debugging
// since stages are single binary bits and we still need to d some indexing
int stageIndex = 0;
// doing this so the first time it updates the brains oled without an exta setup line
int lastStage = -1;


/**
 * @brief Set the Stage Index object
*/
void setStageIndex() {
    for (int i=0; i<StageCount; i++) {
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
}


bool passwordInterpreter(char* password) {
    Mother.STB_.defaultOled.clear();
    for (int passNo=0; passNo < PasswordAmount; passNo++) {
        if (passwordMap[passNo] & stage) {
            if ( strlen(passwords[passNo]) == strlen(password) &&
                strncmp(passwords[passNo], password, strlen(passwords[passNo]) ) == 0
            ) {
            
                delay(500);
                // since there are only 2 stage with a single valid password
               
                stage = stage << 1;
                return true;
            }
        }
    }
    return false;
}


/**
 * @brief handles evalauation of codes and sends the result to the access module
 * @param cmdPtr 
*/
void handleResult(char *cmdPtr) {
    cmdPtr = strtok(NULL, KeywordsList::delimiter.c_str());

    // prepare return msg with correct or incorrect
    char msg[10] = "";
    char noString[3] = "";
    strcpy(msg, keypadCmd.c_str());
    strcat(msg, KeywordsList::delimiter.c_str());
    if (passwordInterpreter(cmdPtr) && (cmdPtr != NULL)) {
        sprintf(noString, "%d", KeypadCmds::correct);
        strcat(msg, noString);
    } else {
        sprintf(noString, "%d", KeypadCmds::wrong);
        strcat(msg, noString);
    }
  
    Mother.sendCmdToSlave(msg);
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


void oledUpdate() {
    char msg[32] = "";
    strcpy(msg, oledHeaderCmd.c_str());
    strcat(msg, KeywordsList::delimiter.c_str());
    strcat(msg, stageTexts[stageIndex]); 
    Mother.sendCmdToSlave(msg);
}


void stageActions() {
    wdt_reset();
 
    switch (stage) {
        case stages::solved:
        break;
    }
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

    Mother.setFlags(0, flagMapping[stageIndex]);
    delay(100);
    oledUpdate();
    stageActions();
}

void setup() {

    Mother.begin();
    // starts serial and default oled
    Mother.relayInit(relayPinArray, relayInitArray, relayAmount);
    MotherIO.ioInit(intputArray, sizeof(intputArray), outputArray, sizeof(outputArray));

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
    Mother.rs485PerformPoll();

    interpreter();
    stageUpdate();
    // handleInputs(); 
    wdt_reset();
}





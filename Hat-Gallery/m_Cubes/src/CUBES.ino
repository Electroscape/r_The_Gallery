/**
 * @file m_airlock.cpp
 * @author Martin Pek (martin.pek@web.de)
 * @brief controls a gate, lights and beamer.
 * inputs via RFID and Keypad
 * @version 0.1
 * @date 2022-09-09
 * 
 * @copyright Copyright (c) 2022
 * 
 *  TODO: 
 * - double post of clean airlock & decontamination
 * - enumerating brain types
 *  6. Wrong code entered on access module -> "Access denied" currently not implemented
 * add numbering of brains to header to make changes easiers
 *  Q: 
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

int stage = preStage;
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
    stage = setupStage;
    for (int relayNo=0; relayNo < relayAmount; relayNo++) {
        Mother.motherRelay.digitalWrite(relayNo, relayInitArray[relayNo]);
    }
}


bool passwordInterpreter(char* password) {
    Mother.STB_.defaultOled.clear();
    for (int passNo=0; passNo < PasswordAmount; passNo++) {
        if (passwordMap[passNo] & stage) {
            if ( (stage == startStage || strlen(passwords[passNo]) == strlen(password) ) &&
                strncmp(passwords[passNo], password, strlen(passwords[passNo]) ) == 0
            ) {
            
                delay(500);
                // since there are only 2 stage with a single valid password
                if (repeatDecontamination && stage == startStage) {
                    stage = decontamination;
                } else {
                    stage = stage << 1;
                }
                return true;
            }
        }
    }

    if (stage == airlockRequest) { stage = airlockFailed; }
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
        if (checkForKeypad()) { continue;}
        checkForRfid();
    }
}


void uvSequence() {
    wdt_disable();
    Mother.motherRelay.digitalWrite(uvLight, open);
    LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrBlack, 50);
    delay(100);
    int repCnt = 5;
    // if (repeatDecontamination) { repCnt = 2; }
    // are UV light supposed to be flashing or only LEDs? Assumed the latter 
    for (int rep=0; rep<repCnt; rep++) {
        for (int i=0; i<3; i++) {
            LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrBlue, 100);
            delay(50);
            LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrBlack, 50);
        }
        if (rep == repCnt -1) { break; }
        delay(2000);
    }
    delay(1500);
    wdt_enable(WDTO_8S);
    LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrGreen, 30);
    Mother.motherRelay.digitalWrite(uvLight, closed);
}


/**
 * @brief  a simple blinking for a given duration, should be a command on the brain soon
*/
void airLockBlink(unsigned long blinkDuration) {
    wdt_disable();
    unsigned long endTime = millis() + blinkDuration;
    LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrBlack, 100);
    while (millis() < endTime) {
        // was 100 
        LED_CMDS::setStripToClr(Mother, 1, LED_CMDS::clrYellow, 15, 2);
        delay(1500);
        LED_CMDS::setStripToClr(Mother, 1, LED_CMDS::clrBlack, 100, 2);
        delay(500);
    }
    wdt_enable(WDTO_8S);
}   




/**
 * @brief effects in case they code is entered wrong and airlock cleans again
*/
void airlockDenied() {
    bool reset = false;
    for (int i=0; i<5; i++) {
        LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrRed, 100);
        delay(200);
        LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrWhite, 1);
        delay(500);
        if (!reset) {
            reset = true;
            MotherIO.outputReset();
        }
        wdt_reset();
    }

    LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrRed, 30);

    char msg[32] = "";
    strcpy(msg, oledHeaderCmd.c_str());
    strcat(msg, KeywordsList::delimiter.c_str());
    strcat(msg, "Clean Airlock"); 
    Mother.sendCmdToSlave(msg);

    delay(3000);
    wdt_reset();

    repeatDecontamination = true;

}


void waitForGameStart() {
    wdt_disable();
    int inputTicks = 0;
    // waitin for the door to be opened
    while (inputTicks < 5) {
        if (MotherIO.getInputs() != (1 << door_reed)) {
            inputTicks++;
            delay(25);
        } else {
            inputTicks = 0;
        }
    }
    LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrWhite, 100);

    // and checking if the door is closed, being quicker here
    inputTicks = 0;
    while (inputTicks < 5) {
        if (MotherIO.getInputs() == (1 << door_reed)) {
            inputTicks++;
            delay(25);
        } else {
            inputTicks = 0;
        }
    }

    
    Mother.motherRelay.digitalWrite(door, closed);
    // need a fader function here
    LED_CMDS::fade2color(Mother,1,LED_CMDS::clrWhite,100,LED_CMDS::clrRed,30,3000,1);
    LED_CMDS::fade2color(Mother,1,LED_CMDS::clrWhite,100,LED_CMDS::clrRed,30,3000,2);
    LED_CMDS::fade2color(Mother,1,LED_CMDS::clrWhite,100,LED_CMDS::clrRed,30,3000,4);
    delay(3000);
    LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrRed, 30);
    
    MotherIO.setOuput(IOEvents::doorClosed);
    delay(50);
    wdt_enable(WDTO_8S);

    stage = preStage;
}
 

/**
 * @brief  its a big stage so its a seperate function, split could be when
*/
void setupRoom() {
    wdt_disable();
    // provide warning to gate operation
    Mother.motherRelay.digitalWrite(alarm, open);
    airLockBlink(gateWarningDelay);
    wdt_enable(WDTO_8S);

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
        case setupStage:
            setupRoom();
            waitForGameStart();
        break;

        // could be integrated to the setupStage and trashed
        case preStage:        
            wdt_reset();
            delay(2000);
            stage = startStage;
        break;

        case startStage:   
            LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrRed, 30);
        break;

        case intro: 
            wdt_disable();
            // Serial.println("running Into");
            // Mother.motherRelay.digitalWrite(beamerIntro, open);
            MotherIO.setOuput(IOEvents::welcomeVideo);
            delay(1000);
            MotherIO.outputReset();
            LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrBlack, 50);
            delay(39000); // Video Delay 
            wdt_enable(WDTO_8S);
            Mother.motherRelay.digitalWrite(beamerIntro, closed); // stop Beamer after Video
            //stage = sterilisation;// One time sterilisation
            stage = fumigation; // One time sterilisation
        break;
        // have to check if need some sort of synchronisation ... or have a bit of padding in the decontimnation video
        // Decontamination besteht aus Fumigation (weiß) - Sterilization(blau) - Biometric scan(grün)
        case decontamination: // in case of wrong code
            //delay(2950);?? woher kommt das?
            delay(1000);
            MotherIO.setOuput(uvEvent);
            delay(300); // Output Delay
            delay(50);
            MotherIO.outputReset();
            uvSequence();
            stage = airlockRequest;
        break;

        case fumigation: //Fumigation
            // starting with a bright light than green blinking
            wdt_disable();    
            LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrBlack, 100);
            //delay(500);
            MotherIO.setOuput(fumigationEvent); // fx26 dauer 14s 
            delay(100);  // Output Delay
            MotherIO.outputReset();
            LED_CMDS::fade2color(Mother,1,LED_CMDS::clrBlack,100,LED_CMDS::clrWhite,10,900,PWM::set1_2_3);
            delay(1000);
            LED_CMDS::blinking(Mother,1,LED_CMDS::clrBlack,LED_CMDS::clrWhite,50,200,100,10,PWM::set1_2_3);
            delay(2500);
            LED_CMDS::fade2color(Mother,1,LED_CMDS::clrBlack,100,LED_CMDS::clrWhite,30,900,PWM::set1_2_3);
            delay(1000);
            LED_CMDS::blinking(Mother,1,LED_CMDS::clrBlack,LED_CMDS::clrWhite,50,200,100,30,PWM::set1_2_3);
            delay(2500);
            LED_CMDS::fade2color(Mother,1,LED_CMDS::clrBlack,100,LED_CMDS::clrWhite,60,700,PWM::set1_2_3);
            delay(1000);
            LED_CMDS::blinking(Mother,1,LED_CMDS::clrBlack,LED_CMDS::clrWhite,50,200,100,60,PWM::set1_2_3);
            delay(2500); 
            LED_CMDS::fade2color(Mother,1,LED_CMDS::clrWhite,100,LED_CMDS::clrWhite,20,1500,PWM::set1_2_3);
            delay(1600);
            LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrWhite, 20);

            wdt_enable(WDTO_8S);
            stage = sterilization;
        break;

        
        case airlockRequest: break;
        case airlockOpening:
            MotherIO.setOuput(airlockOpeningEvent);
            delay(100);//Ouput Delay
            delay(3400); 
            MotherIO.outputReset();
            wdt_disable();
            LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrBlack, 100);
            Mother.motherRelay.digitalWrite(alarm, open);
            LED_CMDS::blinking(Mother,1,LED_CMDS::clrBlack,LED_CMDS::clrYellow,500,1500,100,15,PWM::set3);
            delay(gateWarningDelay);
            Mother.motherRelay.digitalWrite(gate_pwr, open);
            Mother.motherRelay.digitalWrite(gate_direction, gateUp);
            LED_CMDS::blinking(Mother,1,LED_CMDS::clrBlack,LED_CMDS::clrYellow,500,1500,100,15,PWM::set3);
            delay(gateDuration/2);
            Mother.motherRelay.digitalWrite(alarm, closed);
            LED_CMDS::blinking(Mother,1,LED_CMDS::clrBlack,LED_CMDS::clrYellow,500,1500,100,15,PWM::set3);
            delay(gateDuration/2);
            LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrBlack, 100);
            wdt_enable(WDTO_8S);
            stage = stage << 1;
        break;
        case airlockOpen:
            // depending on how the gates motor works we shut it off
            Mother.motherRelay.digitalWrite(gate_pwr, closed);
            // the gateup remains, but no power no change due to relay logic &
            // technically we could use runningLightDuration
            LED_CMDS::runningPWM(Mother, 1, LED_CMDS::clrYellow, 30, 2000, 4);
            wdt_disable();
            delay(runningLightDuration);
            LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrRed, 15);
            wdt_enable(WDTO_8S);
            stage = idle;
        break;
        case idle:
            delay(200);
            LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrRed, 15);
        break;
        case airlockFailed: 
            MotherIO.setOuput(wrongCode);
            airlockDenied();
            stage = startStage;
        break;

        case david_end_Stage:
            LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrGreen, 50);
            stage = idle;
        break;

        case rachel_end_stage:
            wdt_disable();
            /* delay(41000); // Video Rachel
            delay(34000); // Video Proceed to airlock start at second "remain calm" */
            LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrBlack, 50);
            delay(200);
            Mother.motherRelay.digitalWrite(alarm, open);
            LED_CMDS::blinking(Mother,1,LED_CMDS::clrBlack,LED_CMDS::clrYellow,950,50,100,100,PWM::set1_2_3);
            
            delay(10000); // Delay Countdown
            Mother.motherRelay.digitalWrite(alarm, closed);

            LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrBlack, 50);
            delay(1000);
            for (int i=0; i<4; i++) {
                LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrRed, 100);
                delay(200);
                LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrBlack, 100);
                delay(300);
                LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrYellow, 100);
                delay(100);
                LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrBlue, 100);
                delay(100);
                LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrBlack, 100);
                delay(300);
            }
            delay(500);
            LED_CMDS::blinking(Mother,1,LED_CMDS::clrBlack,LED_CMDS::clrRed,10,10,100,10,PWM::set2);
            delay(1000);
            for (int i=0; i<3; i++) {
                LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrRed, 60);
                delay(400);
                LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrBlack, 100);
                delay(300);
                LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrYellow, 60);
                delay(200);
                LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrBlue, 100);
                delay(200);
                LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrBlack, 100);
                delay(300);
            }
            delay(500);
            LED_CMDS::blinking(Mother,1,LED_CMDS::clrBlack,LED_CMDS::clrRed,10,10,100,10,PWM::set2);
            delay(1000);
            for (int i=0; i<3; i++) {
                LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrRed, 30);
                delay(650);
                LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrBlack, 100);
                delay(300);
                LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrYellow, 30);
                delay(325);
                LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrBlue, 100);
                delay(325);
                LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrBlack, 100);
                delay(300);
            }
            delay(500);
            LED_CMDS::blinking(Mother,1,LED_CMDS::clrBlack,LED_CMDS::clrRed,10,10,100,10,PWM::set2);
            delay(1000);
            for (int i=0; i<3; i++) {
                LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrRed, 30);
                delay(900);
                LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrBlack, 100);
                delay(300);
                LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrYellow, 30);
                delay(450);
                LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrBlue, 100);
                delay(450);
                LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrBlack, 100);
                delay(300);
            }
            LED_CMDS::setAllStripsToClr(Mother, 1, LED_CMDS::clrBlack, 1000);
            delay(7000);
            LED_CMDS::fade2color(Mother,1,LED_CMDS::clrBlack,100,LED_CMDS::clrWhite,100,12000,PWM::set1);
            delay(12000);
            wdt_enable(WDTO_8S);
            LED_CMDS::setStripToClr(Mother, 1, LED_CMDS::clrWhite, 100,0); // set strip to clr not yet with PWMset 24.03.23
            stage = idle;

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
    // if we go ailrockfailed, we have to keep the failed text on display 
    if (false && stage == airlockFailed) {
        wdt_reset();
    }
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


void handleInputs() {

    if (stage != idle) { return; }
    lastStage = idle;
    int result = MotherIO.getInputs();
        Serial.println(F("Wait for Input!"));
       // delay(2000);
    if (result > 0) {
        result -= result & (1 << door_reed);
        Serial.println(F("Input from Arbiter!"));
        Serial.println(result);
    }
    unsigned long startTime = millis();
    switch (result) {
        case 1 << 7: 
            stage = david_end_Stage;
        break;
        default: break;
    }
}


void setup() {

    Mother.begin();
    // starts serial and default oled
    Mother.relayInit(relayPinArray, relayInitArray, relayAmount);
    MotherIO.ioInit(intputArray, sizeof(intputArray), outputArray, sizeof(outputArray));

    // Serial.println("WDT endabled");
    wdt_enable(WDTO_8S);

    // technicall 2 but no need to poll the 2nd 
    Mother.rs485SetSlaveCount();

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
    handleInputs(); 
    wdt_reset();
}





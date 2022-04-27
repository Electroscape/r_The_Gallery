/**
*   2CP - TeamEscape - Engineering
*   Author Martin Pek & Abdullah Saei
*
*   - use namespace
*   - Modified Serial outputs
*   - Optimize initialization delay to smooth restarts.
*   - Locking after correct solution.
*
*/

/**************************************************************************/
// Setting Configurations
#include "header_st.h"

#include <stb_common.h>
#include <avr/wdt.h>

#include <stb_rfid.h>
#include <stb_led.h>
#include <stb_oled.h>

STB STB;

// defining necessary neopixel objects and variables
Adafruit_NeoPixel LED_Strips[STRIPE_CNT];

const long int clrRed = LED_Strips[0].Color(255, 0, 0);  //red
const long int clrGreen = LED_Strips[0].Color(0, 255, 0);  //green
const long int clrYellow = LED_Strips[0].Color(255, 255, 0);  //Yellow
const long int clrBlack = LED_Strips[0].Color(0, 0, 0);  //black

const byte RFID_SSPins[] = {
    RFID_1_SS_PIN,
    RFID_2_SS_PIN,
    RFID_3_SS_PIN,
    RFID_4_SS_PIN};

// very manual but ... its C its gonna be bitching when it doesn't know during compile time
const Adafruit_PN532 RFID_0(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[0]);
const Adafruit_PN532 RFID_1(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[1]);
const Adafruit_PN532 RFID_2(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[2]);
const Adafruit_PN532 RFID_3(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[3]);

Adafruit_PN532 RFID_READERS[4] = {
    RFID_0,
    RFID_1,
    RFID_2,
    RFID_3};  //Maximum number for reader supported by STB
char RFID_reads[4][RFID_SOLUTION_SIZE] = {
    "\0\0",
    "\0\0",
    "\0\0",
    "\0\0"};  //Empty reads for the beginning

#define RFID_DATABLOCK 1
uint8_t keya[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};


//==Variables==============================/
int cards_solution[RFID_AMOUNT] = {0};  //0 no card, 1 there is card, 2 correct card
int cards_present[RFID_AMOUNT] = {0};
bool gameLive = true;
bool printStats = true;

//=====Timer=============================/
unsigned long delayStart = 0;  // the time the delay started
bool delayRunning = false;     // true if still waiting for delay to finish

//==PCF8574==============================/
PCF8574 relay;

/*======================================
//===SETUP==============================
//====================================*/
void setup() {
    STB.begin();
    Serial.println("WDT endabled");
    Serial.println("MY NEW VERSION");
    Serial.println("MY NEW VERSION");
    Serial.println("MY NEW VERSION");
    wdt_enable(WDTO_8S);

    STB.i2cScanner();

    STB.dbg("Led init ...");
    if (STB_LED::ledInit(LED_Strips, ledCnts, ledPins, NEO_BRG)) {STB.dbgln(" successful!");};

    wdt_reset();

    for (int i = 0; i < RFID_AMOUNT; i++) {
        STB.dbg("RFID-Reader init: "); STB.dbgln(String(i));
        if (STB_RFID::RFIDInit(RFID_READERS[i])) {STB.dbgln("success!");};
        wdt_reset();
    }

    wdt_reset();

    STB.relayInit(relay, relayPinArray, relayInitArray, REL_AMOUNT);

    wdt_reset();
    delayStart = millis();  // start delay
    STB.printSetupEnd();
}

/*======================================
//===LOOP==============================
//====================================*/
void loop() {
    //send refresh signal every interval of time
    Update_serial();
    if (gameLive) {
        readRFIDs();
        updateLeds();
        if (isGameSolved()) {
            solveGame();
        }
    }
    wdt_reset();
    delay(150);
}

bool isGameSolved() {
    for (int i = 0; i<RFID_AMOUNT; i++) {
        if (cards_solution[i] == 0) { return false;}
    }
    return true;
}

void updateLeds() {

}

void solveGame() {
    delay(100);
    relay.digitalWrite(REL_ROOM_LI_PIN, LIGHT_OFF);
    relay.digitalWrite(REL_SCHW_LI_PIN, LIGHT_ON);
    delay(5000);
    wdt_reset();

    // Sometimes green LEDs miss the command then, instead of waiting
    // 3 secs to be updated, update every 1 sec.
    wdt_reset();
    delay(3000);
    relay.digitalWrite(REL_ROOM_LI_PIN, LIGHT_ON);

    //Block the game
    STB.dbgln("Waiting for new Game!");
    STB.dbgln("Restart in required!");
    wdt_disable();
    STB::printWithHeader("Game Complete", "SYS");
}


// RFID functions
/**
 * RFID framework read, check and update
 *
 * @param void
 * @return void
 */
void readRFIDs() {
    uint8_t data[16];
    String msg = "";
    bool cardsChanged = false;

    for (uint8_t reader_nr = 0; reader_nr < RFID_AMOUNT; reader_nr++) {

        if (STB_RFID::cardRead(RFID_READERS[reader_nr], data, 1)) {
            cards_present[reader_nr]++;

            // evaluate if the cards data matches a reader
            for (uint8_t solution_nr = 0; solution_nr < RFID_AMOUNT; solution_nr++) {
                if (strcmp(RFID_solutions[solution_nr], (char *) data)) {
                    msg += "C";
                    msg.concat(solution_nr + 1);
                    // check if the card is on the correct reader
                    if (solution_nr == reader_nr) {
                        cards_present[reader_nr]++;
                    } 
                } else if (reader_nr + 1 == RFID_AMOUNT) {
                    msg += "UK";
                }
            }
        // no card present
        } else {
            msg += "__";
        }

        msg += " ";

        // decide if updates to outputs is needed, by checking if a card got placed or removed
        if (cards_solution[reader_nr] != cards_present[reader_nr]) {
            cardsChanged = true;
            printStats = true;
            memcpy(cards_solution, cards_present, RFID_SOLUTION_SIZE);
        }
        
    }

    // to terminate the string
    msg += "\0";

    if (cardsChanged) { 
        STB.dbgln(msg); 
        STB::printWithHeader("Cards Changed", "SYS");
    }
}

/* 
    RFID card printouts
    "__" no card
    "XX"
    "Uk"
    "CX"
    STB::printWithHeader("!Correct", relayCode);
    STB::printWithHeader("!Wrong", relayCode);

    STB_LED::setStripToClr(LED_Strips[i], clrYellow);
    STB_LED::setStripToClr(LED_Strips[i], clrGreen);
    STB_LED::setStripToClr(LED_Strips[i], clrRed);
    STB_LED::setStripToClr(LED_Strips[i], clrBlack);
*/


/**
 * prints refesh message to the serial after delay in UpdateSignalAfterDelay
 *
 * @param void
 * @return void
 * @note used as heartbeat check
 */
void Update_serial() {
    // check if delay has timed out after UpdateSignalAfterDelay ms
    if ((millis() - delayStart) >= UpdateSignalAfterDelay) {
        delayStart = millis();
        printStats = true;
    }
}

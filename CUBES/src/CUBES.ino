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
#include "header_s.h"

// I2C Port Expander
#include <SPI.h>
#include <Wire.h>

#include "PCF8574.h"

// Watchdog timer
#include <avr/wdt.h>

// uncomment to use
#define DEBUGMODE 0

// == LEDS ================================================//

#define RFID_1_LED_PIN 9  // Per Konvention ist dies RFID-Port 1
#define RFID_2_LED_PIN 6  // Per Konvention ist dies RFID-Port 2
#define RFID_3_LED_PIN 5  // Per Konvention ist dies RFID-Port 3
#define RFID_4_LED_PIN 3  // Per Konvention ist dies RFID-Port 4
#define PWM_PINS \
    (int[]) { RFID_1_LED_PIN, RFID_2_LED_PIN, RFID_3_LED_PIN, RFID_4_LED_PIN }

// NeoPixel
#include <Adafruit_NeoPixel.h>   // Ueber Bibliotheksverwalter
                                 // NeoPixel
#define NEOPIXEL_NR_OF_PIXELS 1  // Anzahl der Pixel auf einem Strang (Test 1 Pixel)
#define STRIPE_CNT 3

Adafruit_NeoPixel LED_Strips[STRIPE_CNT];

// == PN532 imports and setup
#include <Adafruit_PN532.h>

// If using the breakout with SPI, define the pins for SPI communication.
#define PN532_SCK 13
#define PN532_MOSI 11
#define PN532_MISO 12

#define RFID_1_SS_PIN 8  // Per Konvention ist dies RFID-Port 1
#define RFID_2_SS_PIN 7  // Per Konvention ist dies RFID-Port 2
#define RFID_3_SS_PIN 4  // Per Konvention ist dies RFID-Port 3
#define RFID_4_SS_PIN 2  // Per Konvention ist dies RFID-Port 4

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

/*==OLED============================================================*/
#ifndef OLED_DISABLE
#include "SSD1306AsciiWire.h" /* https://github.com/greiman/SSD1306Ascii                            */
SSD1306AsciiWire oled;
#endif

//==Variables==============================/
int cards_solution[RFID_AMOUNT] = {0};  //0 no card, 1 there is card, 2 correct card
bool runOnce = false;
bool EndBlock = false;
bool printStats = true;

//=====Timer=============================/
unsigned long delayStart = 0;  // the time the delay started
bool delayRunning = false;     // true if still waiting for delay to finish

//==PCF8574==============================/
Expander_PCF8574 relay;

//==Serial Printing======================/
const int ctrlPin = A0;  // the control pin of max485 rs485 LOW read, HIGH write

/*======================================
//===SETUP==============================
//====================================*/
void setup() {
    brainSerialInit();
    Serial.println("WDT endabled");
    wdt_enable(WDTO_8S);
    Serial.println();
    Serial.println("LED: ... ");
    if (LED_init()) {
        Serial.println("LED: OK!");
    } else {
        Serial.println("LED: FAILED!");
    };

    wdt_reset();

    Serial.println();
    Serial.println("I2C: ... ");
    if (i2cScanner()) {
        Serial.println("I2C: OK!");
    } else {
        Serial.println("I2C: FAILED!");
    };

#ifndef OLED_DISABLE
    Serial.print(F("Oleds: ..."));
    if (oled_init()) {
        Serial.println(" light OLED ok...");
    }
#endif

    wdt_reset();

    Serial.println();
    Serial.println("RFID: ... ");
    if (RFID_init()) {
        Serial.println("RFID: OK!");
    } else {
        Serial.println("RFID: FAILED!");
    };

    wdt_reset();

    Serial.println();
    Serial.println("Relay: ... ");
    if (relay_init()) {
        Serial.println("Relay: OK!");
    } else {
        Serial.println("Relay: FAILED!");
    };

    wdt_reset();
    delayStart = millis();  // start delay
    printWithHeader("Setup Complete", "SYS");
}

/*======================================
//===LOOP==============================
//====================================*/
void loop() {
    //send refresh signal every interval of time
    Update_serial();

    if (!EndBlock) {
        wdt_reset();
        RFID_loop();
        Update_LEDs();
        wdt_reset();

        //Game Solved
        if (RFID_Status() && !runOnce) {
            Serial.println("GATE OPEN");
            Update_LEDs();
            //0.1 sec delay between correct msg and relay switch
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
            runOnce = true;
            wdt_reset();

            //Block the game
            Serial.println("Waiting for new Game!");
            EndBlock = true;
            Serial.println("Restart in required!");
            wdt_disable();
            printWithHeader("Game Complete", "SYS");
        }
    } else {
        Update_LEDs();
        delay(500);
    }
}

/**
 * Set LED to specific color
 *
 * @param i byte of LED index
 *        color_str string color name (red, green, white, gold, black)
 * @return void
 */
void NeoPixel_StripeOn(byte i, String color_str) {
    uint32_t color;

    if (color_str == "red") {
        color = LED_Strips[0].Color(255, 0, 0);  //red
    } else if (color_str == "green") {
        color = LED_Strips[0].Color(0, 255, 0);  //green
    } else if (color_str == "white") {
        color = LED_Strips[0].Color(255, 0, 0, 0);  //white
    } else if (color_str == "gold") {
        color = LED_Strips[0].Color(255, 70, 0);  //gold
    } else if (color_str == "yellow") {
        color = LED_Strips[0].Color(255, 255, 0);  //gold
    } else if (color_str == "black") {
        color = LED_Strips[0].Color(0, 0, 0);  //black
    } else {
        color = LED_Strips[0].Color(0, 0, 0);  //schwarz
    }

    LED_Strips[i].setPixelColor(0, color);
    LED_Strips[i].show();
}

/**
 * Switch LED off black
 *
 * @param i byte LED index
 * @return void
 */
void NeoPixel_StripeOff(byte i) {
    long int color_black = LED_Strips[0].Color(0, 0, 0);

    LED_Strips[i].setPixelColor(0, color_black);
    LED_Strips[i].show();
}

/**
 * Switch LED to GREEN
 *
 * @param i byte LED index
 * @return void
 */
void NeoPixel_StripeEndGame(byte i) {
    long int color_green = LED_Strips[0].Color(0, 255, 0);

    LED_Strips[i].setPixelColor(0, color_green);
    LED_Strips[i].show();
}

// RFID functions
/**
 * RFID framework read, check and update
 *
 * @param void
 * @return void
 */
void RFID_loop() {
    uint8_t success;
    uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};  // Buffer to store the returned UID
    uint8_t uidLength;
    uint8_t data[16];

    int cards_present[RFID_AMOUNT] = {0};  //compare with previous card stats to detect card changes

    for (uint8_t reader_nr = 0; reader_nr < RFID_AMOUNT; reader_nr++) {
        wdt_reset();
        //Serial.print("reader ");Serial.print(reader_nr);Serial.print(":");

        success = RFID_READERS[reader_nr].readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
        if (success) {
            //Serial.println(" Yes");

            if (uidLength != 4) {  //Card is not Mifare classic, discarding card
                Serial.println("False Card!");
                continue;
            }

            if (!read_PN532(reader_nr, data, uid, uidLength)) {
                Serial.println("read failed");
                continue;
            } else {
                //Valid card present
                cards_present[reader_nr] = 1;

                //Update data
                data[RFID_SOLUTION_SIZE - 1] = '\0';
                strncpy(RFID_reads[reader_nr], (char*)data, RFID_SOLUTION_SIZE);
            }

            if (!data_correct(reader_nr, data)) {
                //Serial.print(cards_present[reader_nr]);Serial.print(cards_solution[reader_nr]);
                //continue;
            } else {
                cards_present[reader_nr] = 2;
            }
        } else {
            //Serial.println(" No");
            //cards_present[reader_nr] = 0;
            //Serial.print(cards_present[reader_nr]);Serial.print(cards_solution[reader_nr]);
        }
        //Serial.print(cards_present[reader_nr]);Serial.println(cards_solution[reader_nr]);
        if (cards_solution[reader_nr] != cards_present[reader_nr]) {
            cards_solution[reader_nr] = cards_present[reader_nr];
            printWithHeader("Cards Changed", "SYS");
            runOnce = false;
            printStats = true;
        }
        //delay(1);
    }
}

/**
 * Checks and prints the status of the RFID
 *
 * @param void
 * @return void
 * @note It prints the header manually! better modify and constuct a string
 *       then send the string with printwithheader function
 */
bool RFID_Status() {
    if (!printStats)
        return false;

    printStats = false;
    // Struct status message:
    String msg = "";

    bool noZero = true;
    int sum = 0;
    size_t j;  //index for wrong solution card
    for (uint8_t i = 0; i < RFID_AMOUNT; i++) {
        sum += cards_solution[i];
        if (cards_solution[i] == 2) {
            msg += "C";
            msg.concat(i + 1);
        } else if (cards_solution[i] == 1) {
            bool found = false;
            for (j = 0; j < RFID_AMOUNT; j++) {
                if (strcmp(RFID_solutions[j], RFID_reads[i]) == 0) {
                    found = true;
                    break;
                }
            }
            if (found) {
                msg += "C";
                msg.concat(j + 1);
            } else {
                msg += "XX";
            }
        } else if (cards_solution[i] == 0) {
            noZero = false;
            msg += "__";
        } else {
            msg += "Uk";
        }
        msg += " ";
    }

    printWithHeader(msg, relayCode);
#ifndef OLED_DISABLE
    oled.clear();
    oled.println();
    oled.print("          ");
    oled.println(msg);
#endif

    if (sum == 2 * RFID_AMOUNT) {
#ifndef OLED_DISABLE
        oled.println("          Correct");
#endif
        printWithHeader("!Correct", relayCode);
        return true;
    } else if (noZero) {
#ifndef OLED_DISABLE
        oled.println("          Wrong");
#endif
        printWithHeader("!Wrong", relayCode);
    }
    return false;
}

void rainbow(u8 i) {
    uint32_t rainbowColors[] = {
        LED_Strips[i].Color(255, 0, 0), //red
        LED_Strips[i].Color(255, 165, 0), //orange
        LED_Strips[i].Color(255, 255, 0), //yellow
        LED_Strips[i].Color(0, 128, 0), //green
        LED_Strips[i].Color(0, 0, 255), //blue
        LED_Strips[i].Color(75, 0, 130), //purple
        LED_Strips[i].Color(238, 130, 238), //pink
        LED_Strips[i].Color(255, 255, 255) //white
    };
    wdt_reset();
    for(u32 color: rainbowColors) {
        LED_Strips[i].setPixelColor(0, color);
        LED_Strips[i].show();
        delay(1000);
        wdt_reset();
    }
}

#ifndef OLED_DISABLE
/**
 * Initialize Oled
 *
 * @param void
 * @return true (bool) on success
 */
bool oled_init() {
    // &SH1106_128x64 &Adafruit128x64
    Serial.print(F("Oled init\n"));
    oled.begin(&SH1106_128x64, OLED_ADD);
    oled.set400kHz();
    oled.setScroll(true);
    oledHomescreen();
    delay(1000);
    return true;
}

void oledHomescreen() {
    oled.clear();
    oled.setFont(Adafruit5x7);
    oled.print("\n\n\n");
    oled.setFont(Verdana12_bold);
    oled.println("  Type your code..");
}
#endif

/**
 * Updates the LEDs with cards present
 *
 * @param void
 * @return void
 */
void Update_LEDs() {
    int sum = 0;
    bool noZero = true;
    //  Serial.println("UPDATE LEDS");
    for (uint8_t i = 0; i < RFID_AMOUNT; i++) {
        sum += cards_solution[i];
        if (cards_solution[i] == 0) {
            noZero = false;
        }
    }

    if (sum == 2 * RFID_AMOUNT) {
        for (size_t i = 0; i < RFID_AMOUNT; i++) {
            NeoPixel_StripeOn(i, "green");
        }
    } else if (noZero) {
        for (size_t i = 0; i < RFID_AMOUNT; i++) {
            NeoPixel_StripeOn(i, "red");
        }
    } else {
        for (size_t i = 0; i < RFID_AMOUNT; i++) {
            if (cards_solution[i] == 0) {
                NeoPixel_StripeOn(i, "black");
            } else {
                NeoPixel_StripeOn(i, "yellow");
            }
        }
    }
}

/**
 * Reads all readers
 *
 * @param //TODO
 * @return true if success
 */
bool read_PN532(int reader_nr, uint8_t* data, uint8_t* uid, uint8_t uidLength) {
    bool success;

    // authentication may be shifted to another function if we need to expand
    success = RFID_READERS[reader_nr].mifareclassic_AuthenticateBlock(uid, uidLength, RFID_DATABLOCK, 0, keya);
    //dbg_println("Trying to authenticate block 4 with default KEYA value");
    delay(1);  //was 100!!
    if (!success) {
        //dbg_println("Authentication failed, card may already be authenticated");
        return false;
    }

    success = RFID_READERS[reader_nr].mifareclassic_ReadDataBlock(RFID_DATABLOCK, data);
    if (!success) {
        Serial.println("Reading failed, discarding card");
    }
    return success;
}

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

/**
 * Checks if the solution is correct
 *
 * @param // TODO
 * @return // TODO
 */
bool data_correct(int current_reader, uint8_t* data) {
    uint8_t result = -1;

    for (int reader_nr = 0; reader_nr < RFID_AMOUNT; reader_nr++) {
        for (int i = 0; i < RFID_SOLUTION_SIZE - 1; i++) {
            if (RFID_solutions[reader_nr][i] != data[i]) {
                // TODO: Unneeded code, should be cleaned!
                // We still check for the other solutions to show the color
                // but we display it being the wrong solution of the riddle
                if (reader_nr == current_reader) {
                    //Serial.print("Wrong Card"); Serial.println(current_reader);
                }
                continue;
            } else {
                //Serial.print(data[i]);
                if (i >= RFID_SOLUTION_SIZE - 2) {
                    // its a valid card but not placed on the right socket
                    //dbg_println("equal to result of reader");
                    //dbg_println(str(i));
                    result = reader_nr;
                }
            }
        }
        //Serial.println(" ");
    }
    if (result < 0) {
        Serial.print((char*)data);
        Serial.println(" = Undefined Card!!!");
        return false;
    }

    return result == current_reader;
}

//==FUNCTIONS=========================================//
/**
 * Initialise LEDs library
 *
 * @param i byte LED index
 * @return void
 */
void NeoPixel_init(byte i) {
    LED_Strips[i] = Adafruit_NeoPixel(NEOPIXEL_NR_OF_PIXELS, PWM_PINS[i], CLR_ORDER + NEO_KHZ800);
    LED_Strips[i].begin();
    // rainbow(i);
}

/**
 * Initialise LEDs and switch them off
 *
 * @param void
 * @return void
 * @note uses NeoPixel_init() function
 */
bool LED_init() {
    for (size_t i = 0; i < STRIPE_CNT; i++) {
        NeoPixel_init(i);
    }
    delay(100);
    for (size_t i = 0; i < STRIPE_CNT; i++) {
        NeoPixel_StripeOff(i);
    }
    return true;
}

/**
 * Initialise RFID
 *
 * @param void
 * @return true on success
 * @note When stuck WTD cause arduino to restart
 */
bool RFID_init() {
    bool success;
    for (int i = 0; i < RFID_AMOUNT; i++) {
        uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};  // Buffer to store the returned UID
        uint8_t uidLength;                      // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

        wdt_reset();

        Serial.print("initializing reader: ");
        Serial.println(i);
        RFID_READERS[i].begin();
        RFID_READERS[i].setPassiveActivationRetries(5);

        int retries = 0;
        while (true) {
            uint32_t versiondata = RFID_READERS[i].getFirmwareVersion();
            if (!versiondata) {
                Serial.println("Didn't find PN53x board");
                if (retries > 5) {
                    Serial.println("PN532 startup timed out, restarting");
                    softwareReset();
                }
            } else {
                Serial.print("Found chip PN5");
                Serial.println((versiondata >> 24) & 0xFF, HEX);
                Serial.print("Firmware ver. ");
                Serial.print((versiondata >> 16) & 0xFF, DEC);
                Serial.print('.');
                Serial.println((versiondata >> 8) & 0xFF, DEC);
                break;
            }
            retries++;
        }
        // configure board to read RFID tags
        RFID_READERS[i].SAMConfig();
        delay(1);  //was 20!!!
        success = RFID_READERS[i].readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
        //cards_solution[i]= int(success);
    }

    return success;
}

/**
 * Initialise Relays on I2C
 *
 * @param void
 * @return true when done
 */
bool relay_init() {
    Serial.println("initializing relay");
    relay.begin(RELAY_I2C_ADD);

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

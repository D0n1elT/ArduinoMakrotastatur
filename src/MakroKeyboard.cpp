#define KEY_LEFT_CTRL   0x80 // DEC: 128
#define KEY_LEFT_SHIFT  0x81 // DEC: 129
#define KEY_LEFT_ALT    0x82 // DEC: 130
#define KEY_LEFT_GUI    0x83 // DEC: 131
#define KEY_RIGHT_CTRL  0x84 // DEC: 132
#define KEY_RIGHT_SHIFT 0x85 // DEC: 133
#define KEY_RIGHT_ALT   0x86 // DEC: 134
#define KEY_RIGHT_GUI   0x87 // DEC: 135
#define KEY_RETURN      0xB0 // DEC: 176
#define KEY_ESC         0xB1 // DEC: 177
#define KEY_BACKSPACE   0xB2 // DEC: 178
#define KEY_TAB         0xB3 // DEC: 179
#define KEY_RIGHT_ARROW 0xD7 // DEC: 215
#define KEY_LEFT_ARROW  0xD8 // DEC: 216
#define KEY_DOWN_ARROW  0xD9 // DEC: 217
#define KEY_UP_ARROW    0xDA // DEC: 218
#define KEY_INSERT      0xD1
#define KEY_DELETE      0xD4
#define KEY_PAGE_UP     0xD3
#define KEY_PAGE_DOWN   0xD6
#define KEY_HOME        0xD2
#define KEY_END         0xD5
#define KEY_CAPS_LOCK   0xC1
#define KEY_F1          0xC2
#define KEY_F2          0xC3
#define KEY_F3          0xC4
#define KEY_F4          0xC5
#define KEY_F5          0xC6
#define KEY_F6          0xC7
#define KEY_F7          0xC8
#define KEY_F8          0xC9
#define KEY_F9          0xCA
#define KEY_F10         0xCB
#define KEY_F11         0xCC
#define KEY_F12         0xCD

// #############################################################
// This program is designed to be run on an Sparkfun Pro Micro
// to act as a programable macro keyboard.
// More info on https://teichm-sh.de/makro
// #############################################################

// The hardwarerevision you're preparing to upload to
#define HARDWAREREVISION 1
// Ver. 1.0 (1) is the very first revision. The board is unlabeled.
//  - Has no holes for MX Switches.
//  - Weird switch positions
//  - 07.11.2018
// Ver. 1.5 (2) board is labeled 13.01.2020
//  - Never manufactured one.
// Ver. 2.0 (3) labeled 18.01.2020
//  - Improved holes and switch positions
//  - Rotated Text 180° *Facepalm*

#define SOFTWAREREVISION "v1.1"
// Ver. "0.1" is the very first version.
//  - The concept of hardware/software -versions isn't implemented.
// Ver. "1.0"
//  - Hardware/software-versions implemented.
//  - Refactored a LOT of code.
// Ver. "v1.1"
//  - Reset EEPROM after every write...

#define EEPROMREAD
#define EEPROMWRITE

// Our used libraries.
#include <Arduino.h>
#include <Keyboard.h>
#include <EEPROM.h>
#include <errno.h>
#include <limits.h>

// Im too lazy for typing Serial.println every ducking time
// (And if we want to write the log to an SD Card for Example,
// we could replace this with actual functions to write to the SD Card)
#define log Serial.print
#define logln Serial.println

// ### Hardwarerevision 1 - 1.5 ##
  #if HARDWAREREVISION < 3 && HARDWAREREVISION > 0

    // We have 6 buttons on the board labeled from 1 to 6.
    #define MAX_BUTTONS 6
    boolean ledState[MAX_BUTTONS];
    boolean oldLedState[MAX_BUTTONS];
    byte ledPin[MAX_BUTTONS]    = {8,  7,  9,  6,  2, 3};
    byte buttonPin[MAX_BUTTONS] = {10, 16, 14, 15, 4, 5};

// #############################
// ### Hardwarerevision 1 - 1.5 ##
  #elif HARDWAREREVISION == 3

    // We have 6 buttons on the board labeled from 1 to 6.
    // This time in the right order.
    #define MAX_BUTTONS 6
    boolean ledState[MAX_BUTTONS];
    boolean oldLedState[MAX_BUTTONS];
    byte ledPin[MAX_BUTTONS]    = {4, 2,  6,  9,  7,  8};
    byte buttonPin[MAX_BUTTONS] = {5, 3, 15, 14, 16, 10};

  #endif
// #############################

int selectedButton = 0;

// The button which must be held down at boot for upload mode to begin.
// Indexing begins at 0.
#define uploadMode_bootbutton 0
// The button which when pressed inside upload mode reboots the keyboard.
// Indexing begins at 0.
#define uploadMode_rebootbutton 5
// Indicates if the upload mode is on.
boolean uploadMode;

// The EEPROM starting addresses for the commands of the buttons.
unsigned int buttonROMAddr[] =   {11,  180, 349, 518, 687, 856};
// The EEEPROM space determined for each button.
unsigned int buttonROMLength[] = {168, 168, 168, 168, 168, 168};

// str2int() can return following things
enum STR2INT_RETURNTYPES {
  STR2INT_SUCCESS,
  STR2INT_OVERFLOW,
  STR2INT_UNDERFLOW,
  STR2INT_INCONVERTIBLE
};

// A function I recklessly copied from the internet.
// You give it a pointer of an integer and a character
// buffer and the base of the number (10 DEC, 2 BIN, 8 OCT, 16 HEX)
STR2INT_RETURNTYPES str2int(int*, const char*, int);

// Prints the given returntype and returns true if successful
boolean str2int_printresult(STR2INT_RETURNTYPES);

// ASCII value for CMND_STRING
#define CMND_STRING_ASCII 'A'
// CMND_STRING but in a shortened 3 letter form
#define CMND_STRING_SHORT "str"
// CMND_STRING but written out
#define CMND_STRING_FULL "String"

// ASCII value for CMND_DELAY
#define CMND_DELAY_ASCII 'B'
// CMND_DELAY but in a shortened 3 letter form
#define CMND_DELAY_SHORT "dly"
// CMND_DELAY but written out
#define CMND_DELAY_FULL "Delay"

// ASCII value for CMND_MODKEYRELEASE
#define CMND_MODKEYRELEASE_ASCII 'C'
// CMND_MODKEYRELEASE but in a shortened 3 letter form
#define CMND_MODKEYRELEASE_SHORT "mkr"
// CMND_MODKEYRELEASE but written out
#define CMND_MODKEYRELEASE_FULL "Modkey-Release"

// ASCII value for CMND_MODKEYPRESS
#define CMND_MODKEYPRESS_ASCII 'D'
// CMND_MODKEYPRESS but in a shortened 3 letter form
#define CMND_MODKEYPRESS_SHORT "mkp"
// CMND_MODKEYPRESS but written out
#define CMND_MODKEYPRESS_FULL "Modkey-Press"

// A enum for each command.
enum CMNDTYPES {
  CMND_STRING = CMND_STRING_ASCII,
  CMND_DELAY = CMND_DELAY_ASCII,
  CMND_MODKEYPRESS = CMND_MODKEYPRESS_ASCII,
  CMND_MODKEYRELEASE = CMND_MODKEYRELEASE_ASCII,
  CMND_NOCOMMAND = -1
};

boolean initUploadMode();     // initializes the upload mode, in which we program the button actions.
void playNoSerialAnim();      // plays when no serial connection could be established at boot after 30 seconds.
void playIntroAnim();         // plays at boot.
void calcUploadMode();        // constantly outputs EEPROM data on selected button.
void(* resetFunc) (void) = 0; // declare reset function @ address 0.
boolean checkMagicPackets();  // check if EEPROM was cleared.
void serialGetCommand();      // get and parse the inputs and process them into commands.
boolean processButton (int btnID, boolean dryRun); // wrapper around execCommandsEEPROM().
String getCmdTypeAsString (CMNDTYPES input); // just delivers commands as a String.
CMNDTYPES getCmdType (const String input); // gives you a CMNDTYPES enum for a String.
boolean execCommandsEEPROM(int startingAddr, int btnLength, boolean dryMode); // reads EEPROM, searches for commands and executes them.
boolean storeCommandIntoEEPROM(CMNDTYPES CMND_ID, int adr, int len, char* buf, unsigned int *newOffset); // writes a command into EEPROM
void printButtonEEPROMSpace(int selectedButton); // prints EEPROM for this button

// Print debug messages?
//#define DEBUG

void setup() {
  // Helpful if we want to program the controller.
  // Without the delay, it would fail to program sometimes.
  delay(1500);

  // Initialize all LEDs and buttons.
  for (int i = 0; i < MAX_BUTTONS; i++) {
    pinMode(ledPin[i], OUTPUT);
    digitalWrite(ledPin[i], LOW);
    pinMode(buttonPin[i], INPUT_PULLUP);
  }

  // If button <uploadMode_bootbutton> is pressed -> go into upload mode. (for programing the buttons)
  if (!digitalRead(buttonPin[uploadMode_bootbutton])) {
    Serial.begin(115200);
    initUploadMode();
    return;
  } else {
    // Play the everything is normal animation.
    for (int i = 0; i < MAX_BUTTONS * 2; i++) {
      digitalWrite(ledPin[i % MAX_BUTTONS], !digitalRead(ledPin[i % MAX_BUTTONS]));
      delay(40);
    }
    Keyboard.begin();
  }

  /*
   * // Test if we get a Serial connection in 30 seconds.
   * long then = millis();
   * while (!Serial) {
   *  digitalWrite(ledPin[5], !digitalRead(ledPin[5]));
   *  delay(100);
   *  if (millis() - then >= 30000) {
   *   // Sadly we didn't get a Serial connection.. So we play the no Serial animation.
   *   playNoSerialAnim();
   *   digitalWrite(ledPin[5], LOW);
   *   return;
   *  }
   * }
   * digitalWrite(ledPin[5], LOW);
   */

  // We check if the EEPROM was written once.
  if (!checkMagicPackets()) {
    playNoSerialAnim();
    return;
  }

  // And last but not least we play the intro animation.
  playIntroAnim();
}

// Just prints the upload mode header.
void printUploadModeHeader(){
  logln(F("--------------------------------------------------------------"));
  logln(F("| Welcome to the manual uploadmode of this macro keyboard.   |"));
  logln(F("| Did you know, you can use a nice GUI to program too?       |"));
  logln(F("| More information on teichm-sh.de/makro                     |"));
  logln(F("--------------------------------------------------------------"));
  logln(F("--------------Technical information--------------"));
  log(F("Softwareversion: "));
  logln(SOFTWAREREVISION);
  log(F("Hardwarerevision: "));
  logln(HARDWAREREVISION);
  log(F("Keyboard was programed before: "));
  logln(checkMagicPackets() ? "true" : "false");
  logln(F("------------------------------------------------ "));
}

void printHelp(){
  logln(F("First you need to specify your desired button to program."));
  log(F("To specify a button you want to program, execute following command: 'btnX'\r\nWhere X is the desired button! Please note: 0 <= X <= "));
  logln(MAX_BUTTONS - 1);

  logln(F("Then there are 7 commands you can use after specifing which button you want to program:"));
  logln(F("'btn<x>': explained above."));
  logln(F("'str<...>': a string the keyboard should type."));
  logln(F("'dly<0 <= x <= 65536>': delay further keyboard actions for x milliseconds."));
  logln(F("'mkp<x>': press (and hold) a keyboard modifier key."));
  logln(F("'mkr<x>': release a keyboard modifier key."));
  logln(F("'rst': reboot/reset the keyboard."));
  logln(F("'help': print this message."));
}

/*
 *  initUploadMode() initializes the upload mode.
 *  and returns true if successfull.
 */
boolean initUploadMode() {
  logln(F("Initialising Uploadmode..."));

  // Play the upload mode starting animation.
  for (int i = 0; i < MAX_BUTTONS * 12; i++) {
    digitalWrite(ledPin[i % MAX_BUTTONS], !digitalRead(ledPin[i % MAX_BUTTONS]));
    delay(70);
  }

  long then = millis();
  while (!Serial) {
    digitalWrite(ledPin[5], !digitalRead(ledPin[5]));
    delay(100);
    // Test if we get a Serial connection in 30 seconds.
    if (millis() - then >= 30000) {
      // We didn't get a Serial connection.. So we play the no Serial animation.
      // And because we're in upload mode we CAN'T go further on.
      uploadMode = false;
      playNoSerialAnim();
      digitalWrite(ledPin[5], LOW);
      return false;
    }
  }
  digitalWrite(ledPin[5], LOW);

  // If we DON'T find the magic packets, ERASE ALL EEPROM CONTENT STARTING AT THE SMALLEST VALUE IN buttonROMAddr[]
  if (!checkMagicPackets()) {
    // Find the smallest addr of the buttonROMAddr array.
    unsigned int smallestROMAddr = buttonROMAddr[0];
    for (unsigned int i : buttonROMAddr) {
      if (i < smallestROMAddr)
        smallestROMAddr = i;
    }

    long tempTimer = 0;
    // Erase all EEPROM button commands. But don't erase reserved bytes.
    // And we're assuming that the reserved bytes are on the very start of the EEPROM.
    for (unsigned int i = smallestROMAddr; i < EEPROM.length(); i++) {
      // 'erase' EEPROM with ones
      #ifdef EEPROMWRITE
       EEPROM.put(i, 255);
      #endif
      // We could be faster but I thougt a nice animation
      // indicating that all your data are gone would be cool.
      delay(2);

      if (millis() - tempTimer >= 90) {
        tempTimer = millis();
        // Blink every 90ms...
        digitalWrite(ledPin[2], !digitalRead(ledPin[2]));
      }
    }
    digitalWrite(ledPin[2], LOW); // To be sure the LED is off now.
  }
  uploadMode = true;

  // Time to show our guts.
  printUploadModeHeader();
  printHelp();

  // Return true because init. upload mode was successfull.
  return true;
}

// Serial buffer size
#define BUFLEN 170
// Serial buffer
char buffer[BUFLEN];
CMNDTYPES serial_cmnd = CMND_NOCOMMAND;
unsigned int serial_cmnd_len = 0;
unsigned int serial_cmnd_offset = 0;

boolean serialPutCommandsToEEPROM(int buttonID){
  // Can't be saved to EEPROM if the user input ist larger than the free space.
  int adr = buttonROMAddr[buttonID] + serial_cmnd_offset;
  if (adr + serial_cmnd_len >= 1024 || serial_cmnd_len + serial_cmnd_offset > buttonROMLength[buttonID]) {
    log("btnOffset: ");
    log(buttonROMAddr[buttonID]);
    log("\tSerial_cmnd_offset: ");
    log(serial_cmnd_offset);
    log("\tadr: ");
    log(adr);
    log("\tlen: ");
    logln(serial_cmnd_len);
    logln(F("Message wouldn't fit into EEPROM!"));
    return false;
  }

  log(serial_cmnd); log(" "); log(adr); log(" "); log(serial_cmnd_len); log(" "); logln(serial_cmnd_offset);
  if (storeCommandIntoEEPROM(serial_cmnd, adr, serial_cmnd_len, buffer, &serial_cmnd_offset)) {
    logln(F("storeCommandIntoEEPROM() successful"));

    log(F("Erasing EEPROM space after command: "));
    log(adr+serial_cmnd_len);
    log(F(" to "));
    logln(buttonROMLength[buttonID]+buttonROMAddr[buttonID]-1);
    for (unsigned int i = adr+serial_cmnd_len; i < buttonROMAddr[buttonID] + buttonROMLength[buttonID]; i++) {
      // 'Reset' EEPROM
      #ifdef EEPROMWRITE
        EEPROM.put(i, 255); // '255'
      #endif
    }

    printButtonEEPROMSpace(buttonID);
  } else {
    logln(F("storeCommandIntoEEPROM() NOT successful"));
    return false;
  }

  return true;
}

// Sending: CMND_FULLSTR; CMND_IDSTR; CMND_SHORTSTR; CMND_ID
// Receive: BTN_ID; CMND_IDSTR; VALUE
unsigned long timer_serialGetCommands;
int buttonID = -1;
boolean gotSomething = false;
boolean gotCMND_IDSTR = false;
boolean gotBTN_ID = false;

boolean serialReceiveCommands() {
  // Reset timer
  timer_serialGetCommands = millis();

  gotCMND_IDSTR = false;
  gotBTN_ID = false;
  gotSomething = false;
  serial_cmnd_offset = 0;
  serial_cmnd_len = 0;
  serial_cmnd = CMND_NOCOMMAND;

  // Wait for a command but not longer than 10.000ms for one.
  while (millis() - timer_serialGetCommands < 10000) {
    if (Serial.available() > 0) {
      // Reset timer
      timer_serialGetCommands = millis();

      // Parsing...
      if (!gotBTN_ID) {
        int oldButtonID = buttonID;
        buttonID = Serial.parseInt(SKIP_ALL, ';');
        // TODO: USE isButtonValid() method
        if (buttonID >= 0 && buttonID <= MAX_BUTTONS) {
          gotBTN_ID = true;
          if (oldButtonID != buttonID) {
            serial_cmnd_offset = 0;
          }
        } else {
          logln(F("ERROR!"));
          logln(F("BTN-ID is invalid!"));
          return false;
        }
      } else if (!gotCMND_IDSTR) {
        String cmnd_idstr;
        cmnd_idstr = Serial.readStringUntil(';');
        if (cmnd_idstr.c_str()[0] == ';') {
          // Previous parseInt() won't remove it's ';' out of the stream.
          cmnd_idstr = Serial.readStringUntil(';');
        }
        serial_cmnd = getCmdType(cmnd_idstr);
        if (serial_cmnd != CMND_NOCOMMAND) {
          gotCMND_IDSTR = true;
        } else {
          logln(F("ERROR!"));
          logln(F("CMND-IDSTR is invalid!"));
          return false;
        }
      } else if (gotBTN_ID && gotCMND_IDSTR) {
        switch (serial_cmnd) {
          case CMND_STRING: {
            String msg = Serial.readStringUntil('\n');
            #ifdef DEBUG
              log("String value: "); logln(msg);
            #endif

            serial_cmnd_len = msg.length();
            msg.toCharArray(buffer, BUFLEN);
            buffer[++serial_cmnd_len] = 0;

            #ifdef DEBUG
              log("Length: "); logln(serial_cmnd_len);
              for (unsigned int i = 0; i < serial_cmnd_len; i++) {
                log("buffer["); log(i);
                log("]: "); logln(buffer[i]);
              }
            #endif

            break;
          }
          case CMND_DELAY:
          case CMND_MODKEYRELEASE:
          case CMND_MODKEYPRESS: {
            int value = Serial.parseInt();
            itoa(value, buffer, 10);
            break;
          }
          case CMND_NOCOMMAND:
          default: {
            logln(F("ERROR!"));
            logln(F("Command was not recognized!"));
            logln(F("Please send commands in the following format:"));
            logln(F("<BTN_ID>;<CMND_IDSTR>;<VALUE>"));
            return false;
          }
        }

        gotCMND_IDSTR = false;
        gotBTN_ID = false;

        #ifdef DEBUG
          log(F("serialPutCommandsToEEPROM(")); log(buttonID); logln(")");
        #endif

        if (serialPutCommandsToEEPROM(buttonID)) {
          #ifdef DEBUG
            logln("Successful serialPutCommandsToEEPROM().");
          #endif
          gotSomething = true;
        } else {
          #ifdef DEBUG
            logln("Unsuccessful serialPutCommandsToEEPROM().");
          #endif
          return false;
        }
      }
    } else {
      // Safety delay.
      delay(20);
    }
  }
  return gotSomething;
}

void loop() {
  if (uploadMode) {
    if (!digitalRead(buttonPin[uploadMode_rebootbutton])) {
      // Reboot Keyboard
      resetFunc();
    }
    calcUploadMode();
    return;
  } else if (Serial && Serial.available() > 0) {
    delay(250);
    String msg = Serial.readStringUntil('\n');
    msg.toUpperCase();
    if (msg.startsWith("GETEEPROM")) {
      for (unsigned int i = 0; i < EEPROM.length(); i++) {
        log(i);
        log(";");
        logln(String((int)EEPROM[i]));
      }
    } else if (msg.startsWith("GETBUTTONS")) {
      logln(F("# Format: BUTTONID; CMND_IDSTR; VALUE"));
      for (unsigned int i = 0; i < MAX_BUTTONS; i++) {
        // Run in dryMode so no commands will be executed.
        selectedButton = i;
        processButton(i, true);
      }
    } else if (msg.startsWith("GETCMNDS")) {
      logln(F("# Format: CMND_FULLSTR; CMND_IDSTR; CMND_SHORTSTR; CMND_ID"));
      log(CMND_STRING_FULL);        log(';'); log(getCmdTypeAsString(CMND_STRING));        log(';'); log(CMND_STRING_SHORT);        log(';'); logln(CMND_STRING);
      log(CMND_DELAY_FULL);         log(';'); log(getCmdTypeAsString(CMND_DELAY));         log(';'); log(CMND_DELAY_SHORT);         log(';'); logln(CMND_DELAY);
      log(CMND_MODKEYPRESS_FULL);   log(';'); log(getCmdTypeAsString(CMND_MODKEYPRESS));   log(';'); log(CMND_MODKEYPRESS_SHORT);   log(';'); logln(CMND_MODKEYPRESS);
      log(CMND_MODKEYRELEASE_FULL); log(';'); log(getCmdTypeAsString(CMND_MODKEYRELEASE)); log(';'); log(CMND_MODKEYRELEASE_SHORT); log(';'); logln(CMND_MODKEYRELEASE);
    } else if (msg.startsWith("SETBUTTONS")) {
      if (serialReceiveCommands()) {
        #ifdef DEBUG
          logln("Successful serialReceiveCommands().");
        #endif
      } else {
        #ifdef DEBUG
          logln("Unsuccessful serialReceiveCommands()!");
        #endif
      }
    } else if (msg.startsWith("DELETEEEPROM")) {
      for (unsigned int i = 0; i < EEPROM.length(); i++){
        EEPROM.write(i, 255);
        digitalWrite(ledPin[i%6], !digitalRead(ledPin[i%6]));
      }
    } else if (msg.startsWith("UPLOADMODE")) {
      initUploadMode();
    } else if (msg.startsWith("RST")) {
      logln("Resetting...");
      delay(100);
      resetFunc();
    } else {
      logln(F("You won't get far doing this. You need to be in uploadmode in order to program this keyboard manually."));
      log(F("To get into uploadmode, hold down button "));
      log(uploadMode_bootbutton + 1);
      logln(F(" at boot or type 'uploadmode' now."));
      //logln(F("You won't get far doing this. Please consider downloading the programing software at teichm-sh.de/makro"));
    }
  }

  // Process button input for normal mode
  for (int i = 0; i < MAX_BUTTONS; i++) {
    ledState[i] = !digitalRead(buttonPin[i]);
    if (oldLedState[i] != ledState[i]) {
      if (ledState[i]) {
        // Button just pressed.
        digitalWrite(ledPin[i], ledState[i]);
        /*boolean successful = */
        processButton(i, false);
      } else {
        // Button just released. Playing fade out anim.
        for (int r = 0; r < 90; r++) {
          digitalWrite(ledPin[i], HIGH);
          delayMicroseconds(1000 - r * 10);
          digitalWrite(ledPin[i], LOW);
          delayMicroseconds(300 + r * 10);
        }
      }
      /* // DEBUG Text
      if (Serial) {
        log("BUTTON ");
        log(i);
        log("\t");
        log(ledState[i] ? "HIGH" : "LOW");
        log("\tLED Output Pin:\t");
        log(ledPin[i]);
        log("\tBUTTON Input Pin:\t");
        logln(buttonPin[i]);
      } */
      delay(20);
    }
    oldLedState[i] = ledState[i];
  }
}

void playNoSerialAnim() {
  for (int i = MAX_BUTTONS - 1; i >= 0; i--) {
    for (int u = MAX_BUTTONS - 1; u >= 0; u--) {
      if (u != i) digitalWrite(ledPin[u], HIGH);
    }
    delayMicroseconds(200);
    for (int g = 0; g < MAX_BUTTONS; g++) {
      digitalWrite(ledPin[g], LOW);
    }
    delay(100);
    digitalWrite(ledPin[i], HIGH);
    delay(60);
    digitalWrite(ledPin[i], LOW);
  }
  for (int i = 0; i < MAX_BUTTONS; i++) {
    for (int u = 0; u < MAX_BUTTONS; u++) {
      if (u != i) digitalWrite(ledPin[u], HIGH);
    }
    delayMicroseconds(200);
    for (int g = 0; g < MAX_BUTTONS; g++) {
      digitalWrite(ledPin[g], LOW);
    }
    delay(100);
    digitalWrite(ledPin[i], HIGH);
    delay(60);
    digitalWrite(ledPin[i], LOW);
  }
}

void printHexDumpEEPROM(int innerBounds, int outerBounds) {
  #ifdef EEPROMREAD
    log(F("---------------------------------EEPROM-Content--"));
    log(innerBounds);
    log("-to-");
    log(outerBounds);
    logln(F("--------------------------------------"));

    int counter = 0;
    for (int i = innerBounds; i <= outerBounds; i++) {
      byte c;
      #ifdef EEPROMREAD
        c = EEPROM.read(i);
      #endif
      if (c == 0) {
        log("00 ");
      } else {
        log(c, HEX);
        log(" ");
      }

      counter++;
      if (counter == 16 || i == outerBounds) {
        if (counter != 16) {
          byte val = 16 - counter;
          while (val != 0) {
            log("   ");
            val--;
          } 
        }
        log("   |   ");
        for (int j = i - counter + 1; j <= i; j++) {
          byte c;
          #ifdef EEPROMREAD
            c = EEPROM.read(j);
          #endif
          if (c >= 32 && c <= 126) {
            log(String((char)c));
            log(" ");
          } else {
            log(". ");
          }
        }
        if (counter != 16) {
          byte val = 16 - counter;
          while (val != 0) {
            log("  ");
            val--;
          } 
        }
        log("   ");
        log(i - counter + 1);
        log(" ");
        log(i);
        logln();
        counter = 0;
      }
    }
    logln(F("-------------------------------------------------------------------------------------------------"));
  #endif
}

void playIntroAnim() {
  int noSerialAnimCounter = 5;
  for (int i = 0; i < MAX_BUTTONS * 3; i++) {
    for (int g = 0; g < 7; g++) {
      digitalWrite(ledPin[noSerialAnimCounter], HIGH);
      if (i >= 12) delay(5  - (i % 6));
      else delay(6);
      digitalWrite(ledPin[noSerialAnimCounter], LOW);
      if (i >= 12) delay(10 + (i % 6));
      else delay(10);
    }
    switch (noSerialAnimCounter) {
      case 5:
        noSerialAnimCounter = 4;
        break;
      case 4:
        noSerialAnimCounter = 2;
        break;
      case 2:
        noSerialAnimCounter = 0;
        break;
      case 0:
        noSerialAnimCounter = 1;
        break;
      case 1:
        noSerialAnimCounter = 3;
        break;
      case 3:
        noSerialAnimCounter = 5;
        break;
    }
    for (int n = 0; n < 6; n++) {
      if (!digitalRead(buttonPin[n])) {
        return;
      }
    }
  }
}

// boolean forceEEPROMReset = false;
boolean buttonSelected = false;
boolean helpPrinted = false;
long uploadModeTimer = millis();

unsigned int btnOffset = INT_MAX;
unsigned int newOffset = 0;

// Prints EEPROM values for a specific button
void printButtonEEPROMSpace(int selectedButton){
  // Debug messages for the current button selected only
  int adr = buttonROMLength[selectedButton] + buttonROMAddr[selectedButton];

  #ifndef DEBUG
    printHexDumpEEPROM(buttonROMAddr[selectedButton], adr - 1);
  #else
    logln(F("--------------------------------------------------------------"));
    for (int i = buttonROMAddr[selectedButton]; i < adr; i++) {
      /*// if button 2 is pressed then set the cmnd space with '255'
      if ((!digitalRead(buttonPin[2]) || forceEEPROMReset)) {
        // 'Reset' EEPROM
        #ifdef EEPROMWRITE
          EEPROM.put(i, 255); // '255'
        #endif
        delay(30);
        if (millis() - tempTimer >= 90) {
          tempTimer = millis();
          digitalWrite(ledPin[2], !digitalRead(ledPin[2]));
        }
        newOffset = 0;
        forceEEPROMReset = true;
      }*/
      log("EEPROM[");
      log(i);
      log("]:\t");
      byte c;
      #ifdef EEPROMREAD
        c = EEPROM.read(i);
      #endif
      if (c == 0) {
        logln("NULL");
      } else if (c == 255) {
        logln("UNBESCHRIEBEN");
      } else if (c >= 32 && c <= 126) {
        logln(String((char)c));
      } else {
        log("[");
        log(c);
        logln("]");
      }
    }
    logln("----------------");
    digitalWrite(ledPin[2], LOW);

    /*if (forceEEPROMReset) {
      log("EEPROM WAS CLEARED FROM: '");
      log(buttonROMAddr[selectedButton]);
      log( "'\t-'");
      log(adr-1);
      logln("'");
      forceEEPROMReset = false;
    }*/
  #endif
}

void calcUploadMode() {
  if (uploadMode) {
    uploadModeTimer = millis(); // LOL DEBUG TODO FIXME
    if (millis() - uploadModeTimer >= 5000) {
      for (int i = 0; i < MAX_BUTTONS; i++) {
        if (i == 2 || i == 4)continue;
        digitalWrite(ledPin[i], !digitalRead(ledPin[i]));
      }
      uploadModeTimer = millis();

      if (buttonSelected) {
        printButtonEEPROMSpace(selectedButton);
      } else if (!helpPrinted) {
        printHelp();
        logln("<debug> Help Printed...");
        helpPrinted = true;
      }
    }

    if (Serial) {
      serialGetCommand();
    } else {
      // We lost connection!
      // We play an animation. 500 * 10 = 5000ms
      for (int u = 0; u < 100; u++) {
        for (int i = 1; i < MAX_BUTTONS; i++) {
          digitalWrite(ledPin[i], !digitalRead(ledPin[i]));
          delay(10);
        }
      }
      // We need to reset the device.
      resetFunc();  //call reset
    }
  }
}

#define MAGIC_PACKET_LOW  68
#define MAGIC_PACKET_HIGH 84

boolean checkMagicPackets() {
  // MAGIC_PACKET_LOW && MAGIC_PACKET_HIGH our magic packet
  boolean isROMWritten;
  #ifdef EEPROMREAD
    isROMWritten = (EEPROM.read(0) == MAGIC_PACKET_LOW && EEPROM.read(1) == MAGIC_PACKET_HIGH);
  #endif
  if (!isROMWritten) {
    if (Serial) {
      logln();
      logln(F("-------------ERROR!-------------\n"
          "EEPROM MAGIC PACKET DOESN'T MATCH!\n"
          "Please type in 'uploadmode' or hold button "));
      log(uploadMode_bootbutton + 1);
      logln(F(" on bootup! to get into upload mode!"));
      logln(F("-------------ERROR!-------------"));
    }
    for (byte k = 0; k < 100; k++) {
      for (byte i = 0; i < MAX_BUTTONS; i++) {
        digitalWrite(ledPin[i], !digitalRead(ledPin[i]));
      }
      delay(50);
    }
    return false;
  }
  return true;
}

void exec_cmnd_string(String string) {
  log(F("Executing String Command: '"));
  log(string);
  logln("'");
  Keyboard.print(string);
  delay(10);
}

void exec_cmnd_modkeyPress(uint16_t keycode) {
  log(F("Executing Mod Key Press Command: '"));
  log(keycode);
  logln("'");
  Keyboard.press(keycode);
  delay(10);
}

void exec_cmnd_modkeyRelease(uint16_t keycode) {
  log(F("Executing Mod Key Release Command: '"));
  log(keycode);
  logln("'");
  Keyboard.release(keycode);
  delay(10);
}

void exec_cmnd_delay(uint16_t timeMs) {
  log(F("Executing Delay Command: '"));
  log(timeMs);
  logln("'");
  delay(timeMs);
}

byte noCommandCounter = 0;
int adressOfCmndID = -1;
boolean searchingForNull = false;
boolean searchingViaByteCounter = false;
CMNDTYPES cmndCurrentlyInvesting = CMND_NOCOMMAND;

// Define to print debug messages when executing commands
//#define EXEC_DEBUG

// reads EEPROM, searches for commands and executes them.
// or it can run in dryMode and return a list of commands
boolean execCommandsEEPROM(int startingAddr, int btnLength, boolean dryMode) {
  if (!checkMagicPackets())return false;

  byte buflen = 0;
  char characterBuffer[200];
  int8_t byteCounter = -1;

  for (int i = startingAddr; i < startingAddr + btnLength; i++) {
    #ifdef EXEC_DEBUG
      log("Command at Adr: ");
      log(adressOfCmndID);
      log(" Searching NULL?: ");
      log(searchingForNull ? "TRUE" : "FALSE");
      log(" SearchingViaByteCounter?: ");
      log(searchingViaByteCounter ? "TRUE" : "FALSE");
      log("\tByteCounter at: ");
      log(byteCounter);
    #endif

    byte c;
    #ifdef EEPROMREAD
      c = EEPROM.read(i);
    #endif
    #ifdef EXEC_DEBUG
      log("\tEEPROM[");
      log(i);
      log("]: ");
      
      if (c == 0) {
        logln("NULL");
      } else if (c == 255) {
        logln("UNBESCHRIEBEN");
      } else if (c >= 32 && c <= 126) {
        logln(String((char)c));
      } else {
        log("[");
        log(c);
        logln("]");
      }
    #endif

    byteCounter -= 1;
    if (byteCounter < 0)
      byteCounter = -1;

    if (c == CMND_STRING_ASCII && !searchingForNull) { //CMND_STRING
      searchingForNull = true;
      adressOfCmndID = i;
      cmndCurrentlyInvesting = CMND_STRING;
      #ifdef EXEC_DEBUG
        logln("CHANGING TO STRING");
      #endif
      buflen = -1;
      noCommandCounter = 0;
    } else if (c == CMND_DELAY_ASCII && !searchingForNull) { //CMND_DELAY
      searchingForNull = false;
      searchingViaByteCounter = true;
      byteCounter = 2;
      adressOfCmndID = i;
      cmndCurrentlyInvesting = CMND_DELAY;
      #ifdef EXEC_DEBUG
        logln("CHANGING TO DELAY");
      #endif
      buflen = -1;
      noCommandCounter = 0;
    } else if (c == CMND_MODKEYPRESS_ASCII && !searchingForNull) { //CMND_MODKEYPRESS
      searchingForNull = false;
      searchingViaByteCounter = true;
      byteCounter = 2;
      adressOfCmndID = i;
      cmndCurrentlyInvesting = CMND_MODKEYPRESS;
      #ifdef EXEC_DEBUG
        logln("CHANGING TO MKPRESS");
      #endif
      buflen = -1;
      noCommandCounter = 0;
    } else if (c == CMND_MODKEYRELEASE_ASCII && !searchingForNull) { //CMND_MODKEYRELEASE
      searchingForNull = false;
      searchingViaByteCounter = true;
      byteCounter = 2;
      adressOfCmndID = i;
      cmndCurrentlyInvesting = CMND_MODKEYRELEASE;
      #ifdef EXEC_DEBUG
        logln("CHANGING TO MKRELEASE");
      #endif
      buflen = -1;
      noCommandCounter = 0;
    } else if ((byteCounter == 0 && searchingViaByteCounter) || (c == 0 && searchingForNull)) {
      searchingForNull = false;
      searchingViaByteCounter = false;
      byteCounter = -1;
      noCommandCounter = 0;

      #ifdef EXEC_DEBUG
        log("CMND: ");
        log(getCmdTypeAsString(cmndCurrentlyInvesting));
        log("\tAt: ");
        log(adressOfCmndID);
        log("\tto: ");
        logln(i);
      #endif

      characterBuffer[buflen] = c;

      switch (cmndCurrentlyInvesting) {
        case CMND_STRING: {
            if (dryMode) {
              String res = String(selectedButton) + ";" + getCmdTypeAsString(cmndCurrentlyInvesting) + ";" + characterBuffer;
              logln(res);
            } else {
              log("Detected String Command. Sending to Computer: '");
              log(characterBuffer);
              logln("'");
              exec_cmnd_string(characterBuffer);
            }
            break;
          }
        case CMND_DELAY: {
            uint8_t d1 = (uint8_t)characterBuffer[1]; //LOW BYTE
            uint8_t d2 = (uint8_t)characterBuffer[0]; //HIGH BYTE
            uint16_t wd = ((uint16_t)d2 << 8) | d1;
            // CONVERT 2 * 8bit to 1 * 16bit

            if (dryMode) {
              String res = String(selectedButton) + ";" + getCmdTypeAsString(cmndCurrentlyInvesting) + ";" + String(wd);
              logln(res);
            } else {
              log("Detected Delay Command. Sending nothing. Waiting for: '");
              log(String(wd));
              logln("'");
              exec_cmnd_delay(wd);
            }
            break;
          }
        case CMND_MODKEYRELEASE: {
            uint8_t d2 = (uint8_t)characterBuffer[0]; //HIGH BYTE
            uint8_t d1 = (uint8_t)characterBuffer[1]; //LOW BYTE
            uint16_t wd = ((uint16_t)d2 << 8) | d1;
            // CONVERT 2 * 8bit to 1 * 16bit

            if (dryMode) {
              String res = String(selectedButton) + ";" + getCmdTypeAsString(cmndCurrentlyInvesting) + ";" + String(wd);
              logln(res);
            } else {
              log("Detected ModKeyRelease Command. Sending to Computer: '");
              log(String(wd));
              logln("'");
              exec_cmnd_modkeyRelease(wd);
            }
            break;
          }
        case CMND_MODKEYPRESS: {
            uint8_t d2 = (uint8_t)characterBuffer[0]; //HIGH BYTE
            uint8_t d1 = (uint8_t)characterBuffer[1]; //LOW BYTE
            uint16_t wd = ((uint16_t)d2 << 8) | d1;
            // CONVERT 2 * 8bit to 1 * 16bit

            if (dryMode) {
              String res = String(selectedButton) + ";" + getCmdTypeAsString(cmndCurrentlyInvesting) + ";" + String(wd);
              logln(res);
            } else {
              log("Detected ModKeyPress Command. Sending to Computer: '");
              log(String(wd));
              logln("'");
              exec_cmnd_modkeyPress(wd);
            }
            break;
          }
          case CMND_NOCOMMAND:
          default: {
            log(getCmdTypeAsString(cmndCurrentlyInvesting));
            logln(" - what the fuck? Thats not a normal Command!");
            return false;
          }
      }
      #ifdef EXEC_DEBUG
        logln("After execution of command.");
      #endif
      buflen = -1;
      adressOfCmndID = 0;
    } else if (searchingForNull || searchingViaByteCounter) {
      characterBuffer[buflen] = c;
      #ifdef EXEC_DEBUG
        log("Add to character buffer. Size: ");
        log(buflen);
        log(" = ");
        logln(c);
      #endif
      noCommandCounter = 0;
    } else {
      if (noCommandCounter >= 10) {
        #ifdef EXEC_DEBUG
          logln("We don't get any commands anymore.");
          logln("We're going to stop here.");
        #endif
        noCommandCounter = 0;
        return true;
      }
      noCommandCounter++;
    }
    buflen++;
  }
  return true;
}

// Returns a String for the CMNDTYPE
String getCmdTypeAsString (CMNDTYPES input) {
  switch (input) {
    case CMND_STRING:
      return "CMND_STRING";
      break;
    case CMND_DELAY:
      return "CMND_DELAY";
      break;
    case CMND_MODKEYRELEASE:
      return "CMND_MODKEYRELEASE";
      break;
    case CMND_MODKEYPRESS:
      return "CMND_MODKEYPRESS";
      break;
    case CMND_NOCOMMAND:
      return "CMND_NOCOMMAND";
      break;
    default:
      return "INVALID";
      break;
  }
}

// Returns a CMNDTYPE for a String
CMNDTYPES getCmdType (const String input) {
  if (input == "CMND_STRING") {
    return CMND_STRING;
  } else if (input == "CMND_DELAY") {
      return CMND_DELAY;
  } else if (input == "CMND_MODKEYRELEASE") {
    return CMND_MODKEYRELEASE;
  } else if (input == "CMND_MODKEYPRESS") {
    return CMND_MODKEYPRESS;
  } else {
    return CMND_NOCOMMAND;
  }
}

// Just a wrapper function for execCommandsEEPROM()
boolean processButton (int btnID, boolean dryMode) {
  return execCommandsEEPROM(buttonROMAddr[btnID], buttonROMLength[btnID], dryMode);
}

boolean figuringOutWhichButton = true;
boolean figuringOutWhichCommand = false;
CMNDTYPES thisCommandid = CMND_NOCOMMAND;

/*
 * isButtonValid() checks if global selectedButton is valid
 * returns true if valid and false if invalid.
 */
boolean isButtonValid() {
  if (selectedButton >= 0 && selectedButton < MAX_BUTTONS) {
    figuringOutWhichButton = false;
    figuringOutWhichCommand = true;
    btnOffset = buttonROMAddr[selectedButton];
    log(F("You selected button: "));
    logln(selectedButton);
    #ifdef DEBUG
      log("\twith a EEPROM offset of: ");
      logln(btnOffset);
    #endif
    logln("Please specify a command now. Help: 'help'");
    newOffset = 0;
    thisCommandid = CMND_NOCOMMAND;

    /*
     *  // Reset the EEPROM 
     *  long tempTimer = 0;
     *  for (int i = buttonROMAddr[selectedButton]; i < buttonROMAddr[selectedButton] + buttonROMLength[selectedButton]; i++) {
     *    EEPROM.put(i, 255);
     *    delay(2);
     *    if (millis() - tempTimer >= 90) {
     *      tempTimer = millis();
     *      digitalWrite(ledPin[2], !digitalRead(ledPin[2]));
     *    }
     *  }
     *  digitalWrite(ledPin[2], LOW);
     */

    return true;
  } else {
    logln(F("Youre button selection is invalid!\r\nHelp: 'help'"));
    selectedButton = -1;
    newOffset = 0;
    btnOffset = buttonROMAddr[0];
    thisCommandid = CMND_NOCOMMAND;
    figuringOutWhichButton = true;
    figuringOutWhichCommand = false;
    return false;
  }
}


int len = 0;
void serialGetCommand() {
  char buf[BUFLEN];

  // A boolean for meassuring \n\r or \r or \n
  boolean oldSerialState = false;

  while (Serial.available() > 0 || len > 0) {
    // If serial com program sends single characters instead of whole words.
    unsigned long timer = millis();
    while (len > 0 && Serial.available() <= 0) {
      delay(200);
      if (millis() - timer > 2000) {
        logln(F("Serial buffer resets."));
        len = 0;
        return;
      }
    }

    char c = Serial.read();

    // When we get a "\r\n" with every new line...
    if (oldSerialState && (c == '\n' || c == '\r')) {
      break;
    }

    if ((c == '\n') || (c == '\r')) {
      oldSerialState = true;
      logln(); // Print 'missing' '\n'...

      if (figuringOutWhichButton) {
        if (figuringOutWhichCommand) {
          logln(F("Something gone wrong!\r\nErrorcode: 101"));
          len = 0;
          figuringOutWhichButton = false;
          figuringOutWhichCommand = false;
          return;
        }
        if (strncmp(buf, "btn", 3) == 0) {
          // selectedButton = (int)buf[3] - 48;
          STR2INT_RETURNTYPES result = str2int(&selectedButton, buf + 3, 10);

          if (str2int_printresult(result) && isButtonValid()) {
            buttonSelected = true;
          } else {
            buttonSelected = false;
          }

          // len = 0 && buffer cleared
          len = 0;
          return;
        } else if (strncmp(buf, "help", 4) == 0) {
          printHelp();
          return;
        } else {
          // Print help because no valid input was given.
          logln("No valid input! Please have a look at this:");
          printHelp();
          if (figuringOutWhichButton) {
            logln(F("Waiting for you to specify a button. Help: 'help'"));
          } else if (figuringOutWhichCommand) {
            logln(F("Waiting for you to specify a command. Help: 'help'"));
          }
        }
      } else if (figuringOutWhichCommand && len > 2) {
        thisCommandid = CMND_NOCOMMAND;

        if (strncmp(buf, "btn", 3) == 0) {
          // Now we know buf is at minimum 3 bytes long.
          selectedButton = -1;
          STR2INT_RETURNTYPES result = str2int(&selectedButton, buf + 3, 10);

          if (str2int_printresult(result) && isButtonValid()){
            buttonSelected = true;
          } else {
            buttonSelected = false;
          }
          len = 0;
          break;
        } else if (strncmp(buf, CMND_STRING_SHORT, 3) == 0) {
          thisCommandid = CMND_STRING;
          figuringOutWhichCommand = false;
        } else if (strncmp(buf, CMND_DELAY_SHORT, 3) == 0) {
          thisCommandid = CMND_DELAY;
          figuringOutWhichCommand = false;
        } else if (strncmp(buf, CMND_MODKEYPRESS_SHORT, 3) == 0) {
          thisCommandid = CMND_MODKEYPRESS;
          figuringOutWhichCommand = false;
        } else if (strncmp(buf, CMND_MODKEYRELEASE_SHORT, 3) == 0) {
          thisCommandid = CMND_MODKEYRELEASE;
          figuringOutWhichCommand = false;
        } else if (strncmp(buf, "rst", 3) == 0) {
          resetFunc();
        } else if (strncmp(buf, "help", 4) == 0) {
          printHelp();
          break;
        } else {
          logln("Invalid input! Please have a look at this:");
          printHelp();
          if (figuringOutWhichButton) {
            logln(F("Waiting for you to specify a button. Help: 'help'"));
          } else if (figuringOutWhichCommand) {
            logln(F("Waiting for you to specify a command. Help: 'help'"));
          }
          break;
        }
        log(F("You selected command: '"));
        log(getCmdTypeAsString(thisCommandid));
        log(F("' and the total command's length is: "));
        logln(len + 1);
      } else {
        len = 0;
        figuringOutWhichCommand = true;
        logln("Invalid input! Please have a look at this:");
        printHelp();
        if (figuringOutWhichButton) {
          logln(F("Waiting for you to specify a button. Help: 'help'"));
        } else if (figuringOutWhichCommand) {
          logln(F("Waiting for you to specify a command. Help: 'help'"));
        }
        return;
      }

      if (len > 2) {
        if (figuringOutWhichCommand || figuringOutWhichButton) break;

        // Because we have a NULL terminator
        len += 1;

        // We want to remove the command prefix (like "dly" and which is hopefully always 3 chars long)
        // FIXME: Support bigger commands
        len -= 3;
        strncpy(buf, buf + 3, len);

        // Can't be saved to EEPROM if the user input ist larger than the free space.
        unsigned int adr = btnOffset + newOffset;
        if (adr + len >= 1024 || len + newOffset > buttonROMLength[selectedButton]) {
          #ifdef DEBUG
            log("btnOffset: ");
            log(btnOffset);
            log("\tnewOffset: ");
            log(newOffset);
            log("\tadr: ");
            log(adr);
            log("\tlen: ");
            logln(len);
          #endif
          logln(F("\tThis command wouldn't fit into EEPROM!"));
          break;
        }

        if (storeCommandIntoEEPROM(thisCommandid, adr, len, buf, &newOffset)) {
          printButtonEEPROMSpace(selectedButton);
        }

        #ifdef DEBUG
          log("btnOffset: ");
          log(btnOffset);
          log("\tnewOffset: ");
          log(newOffset);
          log("\tadr: ");
          log(adr);
          log("\tlen: ");
          logln(len);
        #endif

        len = 0;
        #ifdef EEPROMWRITE
          EEPROM.write(0, MAGIC_PACKET_LOW);
          EEPROM.write(1, MAGIC_PACKET_HIGH);
        #endif
        figuringOutWhichCommand = true;
      }
    } else {
      oldSerialState = false;

      // Backspace
      if (c == 0x08 && len > 0 ) {
        len--;
        buf[len] = 0; // append null terminator
        log(String(c));
      } else if (len < BUFLEN) {
        // Prevent buffer overflow
        buf[len] = c;
        if (c == '\r' || c == '\n'){
          // Newline with carriage return.
          log('\r'); log('\n');
        } else log(String(c));
        len++;
        buf[len] = 0; // append null terminator
      }
    }
  }
}

/*
 * storeCommandIntoEEPROM()
 */
boolean storeCommandIntoEEPROM(CMNDTYPES CMND_ID, int adr, int len, char* buf, unsigned int *newOffset) {
  //log("CMND_ID: "); logln(CMND_ID);
  //log("CMND_ID_STR: "); logln(getCmdTypeAsString(CMND_ID));
  #ifdef EEPROMWRITE
    if (!checkMagicPackets()) {
      logln("Writing EEPROM magic packets");
      EEPROM.write(0, MAGIC_PACKET_LOW);
      EEPROM.write(1, MAGIC_PACKET_HIGH);
    }
  #endif

  switch (CMND_ID) {
    case CMND_STRING: {
        log("Writing String: '");
        #ifdef EEPROMWRITE
          EEPROM.write(adr, CMND_STRING_ASCII);
        #endif
        for (int i = adr; i < adr + len; i++) {
          // i + 1 because we skip the first adr because thats the cmndid
          #ifdef EEPROMWRITE
            EEPROM.write(i + 1, buf[i - adr]);
          #endif
          log(buf[i - adr]);
        }
        log("' to '");
        log(adr + 1);
        logln("'");
        // newOffset = oldOffset + new cmnd len + commandid
        *newOffset = *newOffset + len + 1;
        break;
    }
    case CMND_MODKEYPRESS: {
        int keycode;
        STR2INT_RETURNTYPES res = str2int(&keycode, buf, 10);
        if (str2int_printresult(res)) {
          uint8_t code_hi = ((keycode >> 8) & 0xff);
          uint8_t code_lo = ((keycode >> 0) & 0xff);
          #ifdef EEPROMWRITE
            EEPROM.write(adr, CMND_MODKEYPRESS_ASCII);
            EEPROM.write(adr + 1, code_hi);
            EEPROM.write(adr + 2, code_lo);
          #endif
          log("Writing ModKeyPress: '");
          log(code_hi);
          log("' to '");
          log(adr + 1);
          log("' and '");
          log(code_lo);
          log("' to '");
          log(adr + 2);
          logln("'");
          // newOffset = oldOffset + 2*keycode + commandid
          *newOffset = *newOffset + 2 + 1;
        } else {
          return false;
        }
        break;
    }
    case CMND_MODKEYRELEASE: {
        int keycode;
        STR2INT_RETURNTYPES res = str2int(&keycode, buf, 10);
        if (str2int_printresult(res)) {
          uint8_t code_hi = ((keycode >> 8) & 0xff);
          uint8_t code_lo = ((keycode >> 0) & 0xff);
          #ifdef EEPROMWRITE
            EEPROM.write(adr, CMND_MODKEYRELEASE_ASCII);
            EEPROM.write(adr + 1, code_hi);
            EEPROM.write(adr + 2, code_lo);
          #endif
          log("Writing ModKeyRelease: '");
          log(code_hi);
          log("' to '");
          log(adr + 1);
          log("' and '");
          log(code_lo);
          log("' to '");
          log(adr + 2);
          logln("'");
          // newOffset = oldOffset + 2*keycode + commandid
          *newOffset = *newOffset + 2 + 1;
        } else {
          return false;
        }
        break;
    }
    case CMND_DELAY: {
        int timeMs;
        STR2INT_RETURNTYPES res = str2int(&timeMs, buf, 10);
        if (str2int_printresult(res)) {
          uint8_t hi = ((timeMs >> 8) & 0xff);
          uint8_t lo = ((timeMs >> 0) & 0xff);
          #ifdef EEPROMWRITE
            EEPROM.write(adr, CMND_DELAY_ASCII);
            EEPROM.write(adr + 1, hi);
            EEPROM.write(adr + 2, lo);
          #endif
          log("Writing Delay: '");
          log(hi);
          log("' to '");
          log(adr + 1);
          log("' and '");
          log(lo);
          log("' to '");
          log(adr + 2);
          logln("'");
          // newOffset = oldOffset + hi + lo + commandid
          *newOffset = *newOffset + 2 + 1;
        } else {
          return false;
        }
        break;
    }
    case CMND_NOCOMMAND:
      // Lets drop to default case
    default: {
        log("Error! Unknown command!: ");
        logln(CMND_ID);
        return false;
    }
  }
  return true;
}

// Prints the given returntype and returns true if successfull
boolean str2int_printresult(STR2INT_RETURNTYPES result) {
  if (result != STR2INT_SUCCESS) {
    log("CONVERTING INPUT FAILED! Errorcode: ");
    if (result == STR2INT_INCONVERTIBLE) {
      logln("102 INCONVERTIBLE");
    } else {
      logln("103 NUMBER TOO BIG OR TOO SMALL");
    }
    return false;
  }
  return true;
}

/* Convert string s to int out.

   @param[out] out The converted int. Cannot be NULL.

   @param[in] s Input string to be converted.

       The format is the same as strtol,
       except that the following are inconvertible:

       - empty string
       - leading whitespace
       - any trailing characters that are not part of the number

       Cannot be NULL.

   @param[in] base Base to interpret string in. Same range as strtol (2 to 36).

   @return Indicates if the operation succeeded, or why it failed.
*/
STR2INT_RETURNTYPES str2int(int *out, const char *s, int base) {
  char *end;
  if (s[0] == '\0' || isspace(s[0]))
    return STR2INT_INCONVERTIBLE;
  errno = 0;
  long l = strtol(s, &end, base);
  /* Both checks are needed because INT_MAX == LONG_MAX is possible. */
  if (l > INT_MAX || (errno == ERANGE && l == LONG_MAX))
    return STR2INT_OVERFLOW;
  if (l < INT_MIN || (errno == ERANGE && l == LONG_MIN))
    return STR2INT_UNDERFLOW;
  if (*end != '\0')
    return STR2INT_INCONVERTIBLE;
  *out = l;
  return STR2INT_SUCCESS;
}

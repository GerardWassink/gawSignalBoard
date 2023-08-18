/* ------------------------------------------------------------------------- *
 * Name   : SignalBoard
 * Author : Gerard Wassink
 * Date   : February 02, 2022
 * Purpose: Connect thru wifi, setup a small webserver to signal messages
 * Versions:
 *   0.1  : Initial version
 *   0.2  : Built in snooze timer which will repeat every minute
 *              untill no signals left anymore
 *   0.3  : Changed button UI
 *   0.4  : Textual changes
 *          Introducing resetBoardPin
 *   0.5  : Text into array for easy replacement (NLS)
 *   0.6  : Textual changes
 *   0.7  : Temperature sensor built in
 *   0.8  : Permanent screen built in
 *   0.9  : Small textual changes
 *          Deleted commented out code for temperature reading
 *          Code cleanup
 *   
 * ------------------------------------------------------------------------- */
#define progVersion "0.9"                      // Program version definition
/* ------------------------------------------------------------------------- *
 *             GNU LICENSE CONDITIONS
 * ------------------------------------------------------------------------- *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * ------------------------------------------------------------------------- *
 *       Copyright (C) February 2022 Gerard Wassink
 * ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- *
 *       Include libraries for peripherals & Arduino stuff
 * ------------------------------------------------------------------------- */
#include "arduino_secrets.h"

#include <SPI.h>
#include <WiFiNINA.h>

#include <Wire.h>                           // I2C comms library
#include <LiquidCrystal_I2C.h>              // LCD library

/* ------------------------------------------------------------------------- *
 *       Switch debugging on / off (compiler directives)
 *       prevent from bloating with the Serial routines when debug is done
 * ------------------------------------------------------------------------- */
#define DEBUG 0

#if DEBUG == 1
  #define debugstart(x) Serial.begin(x)
  #define debug(x) Serial.print(x)
  #define debugln(x) Serial.println(x)
#else
  #define debugstart(x)
  #define debug(x)
  #define debugln(x)
#endif


/* ------------------------------------------------------------------------- *
 *       Other defines
 * ------------------------------------------------------------------------- */
#define BEEPER 10                           // pin for beeper (low active)
#define resetBoardPin 9                     // pin to reset messages

#define LANGUAGE NL                         // language for texts (NL or EN)

/* ------------------------------------------------------------------------- *
 * how long before beeping again when signals are still active 
 * ------------------------------------------------------------------------- */
#define snoozeTimer 60000


/* ------------------------------------------------------------------------- *
 *       Create objects with addres(ses) for the LCD screen(s)
 * ------------------------------------------------------------------------- */
LiquidCrystal_I2C lcd1(0x24,20,4);          // Initialize display 1


/* ------------------------------------------------------------------------- *
 *       Initial switch values
 * ------------------------------------------------------------------------- */
bool CHOICE1 = false;
bool CHOICE2 = false;
bool CHOICE3 = false;
bool CHOICE4 = false;

/* ------------------------------------------------------------------------- *
 *       Define global variables
 * ------------------------------------------------------------------------- */
float temp1 = 0.0;                          // Temperatures read from sensor
  

/* ------------------------------------------------------------------------- *
 *       Variables for snooze function
 * ------------------------------------------------------------------------- */
unsigned long snoozePreviousMillis;         // Log time
unsigned long currentMillis;                // Log time
int lastNum = 0;                            // # of times to beep on snooze


/* ------------------------------------------------------------------------- *
 *       Wifi secrets (to be stored in a .h file)
 * ------------------------------------------------------------------------- */
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;                  // your network SSID (name)
char pass[] = SECRET_PASS;                  // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                           // your network key index number (needed only for WEP)

String myIP;

int status = WL_IDLE_STATUS;
WiFiServer server(80);


/* ------------------------------------------------------------------------- *
 *       Choose language (NL or EN for now, or add your own)
 * ------------------------------------------------------------------------- */
String choiceText[] = 
{
#if LANGUAGE == NL
  /* 
   * Nederlands
   */
  F("      Hallo...      "),
  F("    Pomp draait!    "),
  F("  We kunnen eten!   "),
  F("---=== HELP!  ===---")
#elif LANGUAGE == EN
  /* 
   * English
   */
  F("      Hello...      "),
  F("    Pump running    "),
  F("  Dinner is ready!  "),
  F("---=== HELP!  ===---")
#else
  /* 
   * Default
   */
  FAULTY STATEMENT, NO LANGUAGE CHOSEN
#endif
};

String buttonText[] = 
{
#if LANGUAGE == NL
  /* 
   * Nederlands
   */
  F("Hallo"),
  F("Pomp"),
  F("Eten"),
  F("HELP!")
#elif LANGUAGE == EN
  /* 
   * English
   */
  F("Hello"),
  F("Pump"),
  F("Dinner"),
  F("HELP!")
#else
  /* 
   * Default
   */
  FAULTY STATEMENT, NO LANGUAGE CHOSEN
#endif
};


/* ------------------------------------------------------------------------- *
 * display the Wifi status in the display                  printWifiStatus()
 * ------------------------------------------------------------------------- */
void printWifiStatus() {
  // print the SSID of the network you're attached to:
  debug("SSID: ");
  debugln(WiFi.SSID());
  LCD_display(lcd1, 1, 0, "SSID: ");
  LCD_display(lcd1, 1, 6, WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  debug("IP Address: ");
  debugln(ip);

  myIP = "";
  myIP.concat(ip[0]); myIP.concat(".");
  myIP.concat(ip[1]); myIP.concat(".");
  myIP.concat(ip[2]); myIP.concat(".");
  myIP.concat(ip[3]);

  LCD_display(lcd1, 2, 0, "IP: ");
  LCD_display(lcd1, 2, 4, myIP);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  debug("signal strength (RSSI):");
  debug(rssi);
  debugln(" dBm");
  LCD_display(lcd1, 3, 0, "signal: ");
  LCD_display(lcd1, 3, 8, String(rssi));
  LCD_display(lcd1, 3,13, "(dBm)");

  // print where to go in a browser:
  debug("To see this page in action, open a browser to http://");
  debugln(ip);
}


/* ------------------------------------------------------------------------- *
 * Sound the beeper for the specified number of times         soundBeeper()
 * ------------------------------------------------------------------------- */
void soundBeeper(int num) {
  for (int i=num; i>0; i--) {
    digitalWrite(BEEPER, HIGH);
    delay(100);
    digitalWrite(BEEPER, LOW);
    delay(100);
  }
  /* 
   * Initialize snooze timer
   */
  snoozePreviousMillis = currentMillis;
}


/* ------------------------------------------------------------------------- *
 *       Routine to swtich display off                           screenOff()
 * ------------------------------------------------------------------------- */
void screenOff(LiquidCrystal_I2C screen) {
  screen.noBacklight();
}
  
/* ------------------------------------------------------------------------- *
 *       Routine to clear the display                         clearDisplay()
 * ------------------------------------------------------------------------- */
void clearDisplay(int fromLine, int toLine) {
  for (int i=fromLine; i<=toLine; i++) {
    LCD_display(lcd1, i, 0, F("                    "));
  }
}

/* ------------------------------------------------------------------------- *
 *       Routine to swtich display on                             screenOn()
 * ------------------------------------------------------------------------- */
void screenOn(LiquidCrystal_I2C screen) {
  screen.backlight();
}
  

/* ------------------------------------------------------------------------- *
 *       Routine to display stuff on the display of choice     LCD_display()
 * ------------------------------------------------------------------------- */
void LCD_display(LiquidCrystal_I2C screen, int row, int col, String text) {
    screen.setCursor(col, row);
    screen.print(text);
}


/* ------------------------------------------------------------------------- *
 *       One time setup                                              setup()
 * ------------------------------------------------------------------------- */
void setup() {
  pinMode(BEEPER, OUTPUT);
  pinMode(resetBoardPin, INPUT_PULLUP);
  
  /* 
   * Initialize several objects
   */                                               
  lcd1.init();                              // Initialize LCD Screen 1
  lcd1.backlight();                         // Backlights on by default
  
  debugstart(9600);                       // initialize serial in case of debugging

  LCD_display(lcd1, 0, 0, F("Signal Board vs. "));
  LCD_display(lcd1, 0,17, progVersion);
  
  /* 
   * check for the WiFi module 
   */
  if (WiFi.status() == WL_NO_MODULE) {
    debugln("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }
  LCD_display(lcd1, 1, 0, F("Wifi board present  "));
  soundBeeper(1);
  
  /* 
   * check firmware version
   */
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    debugln("Please upgrade the firmware");
    LCD_display(lcd1, 2, 0, F("Upgrade the firmware"));
  } else {
    LCD_display(lcd1, 2, 0, F("Firmware okay       "));
  }
  soundBeeper(1);
  
  
  /* 
   * attempt to connect to WiFi network
   */
  while (status != WL_CONNECTED) {
    debug("Attempting to connect to Network named: ");
    debugln(ssid);                    // print the network name (SSID);
    LCD_display(lcd1, 3, 0, F("Connecting to Wifi.."));
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    
    delay(10000);                     // wait for connection:

  }
  LCD_display(lcd1, 3, 0, F("Connected to Wifi   "));
  soundBeeper(1);
  delay(1000);                        // chance to read bfore screen blank
  
  clearDisplay(1,3);
  
  /* 
   * Start the webserver
   */
  server.begin();                           // start the web server on port 80

  /* 
   * Show status on display
   */
  printWifiStatus();                        // you're connected now, so print out the status
  delay(5000);                              // chance to read before screen blank
  
  /* 
   * Clear display
   */
  for (int i=0; i<4; i++) {
    LCD_display(lcd1, i, 0, F("                    "));
  }
  
  /* 
   * Display off, ready for business
   */
  lcd1.init();                              // Initialize LCD Screen 1
  screenOff(lcd1);                          // Backlights off

}


/* ------------------------------------------------------------------------- *
 * display Wifi status and temperature in the display     printGeneralInfo()
 * ------------------------------------------------------------------------- */
void printGeneralInfo() {
  screenOn(lcd1);
  // Show the name and version
  LCD_display(lcd1, 0, 0, F("Signal Board vs. "));
  LCD_display(lcd1, 0,17, progVersion);
  
  // print the SSID of the network you're attached to:
  LCD_display(lcd1, 1, 0, "Wifi: ");
  LCD_display(lcd1, 1, 6, WiFi.SSID());

  // Print our IP address
  LCD_display(lcd1, 2, 0, "IP  : ");
  LCD_display(lcd1, 2, 6, myIP);

  // Delay for 5 second, then clear screen again
  delay(5000);
  clearDisplay(0,3);
}


/* ------------------------------------------------------------------------- *
 *       Loop thru this section                                       loop()
 * ------------------------------------------------------------------------- */
void loop() {
  currentMillis = millis();
  
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    debugln("new client");                  // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        debug(c);                           // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          /*
           * if the current line is blank, you got two newline characters in a row.
           * that's the end of the client HTTP request, so send a response:
          */
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            /* 
             *  Establish the styles with CSS: 
             */
            client.print("<style>");
            client.print(".container {margin: 0 auto; text-align: center; margin-top: 100px;}");
            client.print("button {color: white; width: 100px; height: 100px;");
            client.print("border-radius: 50%; margin: 20px; border: none; font-size: 20px; outline: none; transition: all 0.2s;}");
            client.print(".red{background-color: rgb(196, 39, 39);}");
            client.print(".green{background-color: rgb(39, 121, 39);}");
            client.print(".blue {background-color: rgb(5, 87, 180);}");
            client.print(".purple {background-color: rgb(196, 39, 180);}");
            client.print(".off{background-color: grey;}");
            client.print("button:hover{cursor: pointer; opacity: 0.7;}");
            client.print("</style>");
            
            /* 
             * here follows the content of the HTTP response: 
             */
            client.print("<div class='container'>");
            
            client.print("<a href='./'><h1>Signal Board ");
            client.print(String(progVersion));
            client.print("</h1></a>");

            client.print("<h5>A simple messaging board for use over wifi / LAN</h5>");
            
            if (CHOICE1) {
              client.print("<button class='red' type='submit' onmousedown='location.href=\"/RL\"'>");
              client.print(buttonText[0]);
              client.print("</button>");
            } else {
              client.print("<button class='off' type='submit' onmousedown='location.href=\"/RH\"'>");
              client.print(buttonText[0]);
              client.print("</button>");
            }
            
            if (CHOICE2) {
              client.print("<button class='green' type='submit' onmousedown='location.href=\"/GL\"'>");
              client.print(buttonText[1]);
              client.print("</button>");
            } else {
              client.print("<button class='off' type='submit' onmousedown='location.href=\"/GH\"'>");
              client.print(buttonText[1]);
              client.print("</button>");
            }
            
            if (CHOICE3) {
              client.print("<button class='blue' type='submit' onmousedown='location.href=\"/BL\"'>");
              client.print(buttonText[2]);
              client.print("</button>");
            } else {
              client.print("<button class='off' type='submit' onmousedown='location.href=\"/BH\"'>");
              client.print(buttonText[2]);
              client.print("</button>");
            }
            
            if (CHOICE4) {
              client.print("<button class='purple' type='submit' onmousedown='location.href=\"/PL\"'>");
              client.print(buttonText[3]);
              client.print("</button>");
            } else {
              client.print("<button class='off' type='submit' onmousedown='location.href=\"/PH\"'>");
              client.print(buttonText[3]);
              client.print("</button>");
            }

            client.print("</div>");

            /* 
             * The HTTP response ends with another blank line:
             */
            client.println();
            
            /* 
             * break out of the while loop
             */
            break;

          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }


        /* 
         * Check to see what client request we received
         */
        if (currentLine.endsWith("GET /RH")) {
          LCD_display(lcd1, 0, 0, choiceText[0]);
          lastNum = 1;
          soundBeeper(lastNum);
          CHOICE1 = true;
        }
        if (currentLine.endsWith("GET /RL")) {
          LCD_display(lcd1, 0, 0, F("                    "));
          CHOICE1 = false;
        }
        
        if (currentLine.endsWith("GET /GH")) {
          LCD_display(lcd1, 1, 0, choiceText[1]);
          lastNum = 2;
          soundBeeper(lastNum);
          CHOICE2 = true;
        }
        if (currentLine.endsWith("GET /GL")) {
          LCD_display(lcd1, 1, 0, F("                    "));
          CHOICE2 = false;
        }
        
        if (currentLine.endsWith("GET /BH")) {
          LCD_display(lcd1, 2, 0, choiceText[2]);
          lastNum = 3;
          soundBeeper(lastNum);
          CHOICE3 = true;
        }
        if (currentLine.endsWith("GET /BL")) {
          LCD_display(lcd1, 2, 0, F("                    "));
          CHOICE3 = false;
        }
        
        if (currentLine.endsWith("GET /PH")) {
          LCD_display(lcd1, 3, 0, choiceText[3]);
          lastNum = 7;
          soundBeeper(lastNum);
          CHOICE4 = true;
        }
        if (currentLine.endsWith("GET /PL")) {
          LCD_display(lcd1, 3, 0, F("                    "));
          CHOICE4 = false;
        }
      }
    }

    /* 
     * close the connection
     */
    client.stop();
    debugln("client disconnected");
    
  }
  
  /* 
   * Switch display on when any button pressed, else off
   */
  bool boardStatus = (CHOICE1 + CHOICE2 + CHOICE3 + CHOICE4);
  if (boardStatus > 0) {
    screenOn(lcd1);

  /* 
   * after snoozetimer beep last signal if not switched off
   */
    if (currentMillis - snoozePreviousMillis > snoozeTimer) {
      snoozePreviousMillis = currentMillis;
      soundBeeper(lastNum);
    }
  } else {
    screenOff(lcd1);
  }

  /* 
   * check reset board pin
   */
  bool resetBoard = digitalRead(resetBoardPin);
  if (resetBoard == LOW) {
    if (boardStatus > 0) {
      clearDisplay(0,3);
      CHOICE1 = false;
      CHOICE2 = false;
      CHOICE3 = false;
      CHOICE4 = false;
    } else {
      printGeneralInfo();
    }
  }
}



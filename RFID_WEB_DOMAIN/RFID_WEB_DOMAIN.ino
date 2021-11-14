/*
  Web client

  This sketch connects to a website (http://www.google.com)
  using an Arduino Wiznet Ethernet shield.

  Circuit:
   Ethernet shield attached to pins 10, 11, 12, 13

  created 18 Dec 2009
  by David A. Mellis
  modified 9 Apr 2012
  by Tom Igoe, based on work by Adrian McEwen

*/
/*
 * --------------------------------------------------------------------------------------------------------------------
 * Example sketch/program showing how to read new NUID from a PICC to serial.
 * --------------------------------------------------------------------------------------------------------------------
 * This is a MFRC522 library example; for further details and other examples see: https://github.com/miguelbalboa/rfid
 * 
 * Example sketch/program showing how to the read data from a PICC (that is: a RFID Tag or Card) using a MFRC522 based RFID
 * Reader on the Arduino SPI interface.
 * 
 * When the Arduino and the MFRC522 module are connected (see the pin layout below), load this sketch into Arduino IDE
 * then verify/compile and upload it. To see the output: use Tools, Serial Monitor of the IDE (hit Ctrl+Shft+M). When
 * you present a PICC (that is: a RFID Tag or Card) at reading distance of the MFRC522 Reader/PCD, the serial output
 * will show the type, and the NUID if a new card has been detected. Note: you may see "Timeout in communication" messages
 * when removing the PICC from reading distance too early.
 * 
 * @license Released into the public domain.
 * 
 * Typical pin layout used:
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 */

/* SimpleHttpExample
// (c) Copyright 2010-2012 MCQN Ltd.
// Released under Apache License, version 2.0
//
// Simple example to show how to use the HttpClient library
// Get's the web page given at http://<kHostname><kPath> and
// outputs the content to the serial port
*/
 
#include <SPI.h>
#include <Ethernet.h>
#include <MFRC522.h>
#include <ArduinoUniqueID.h>
#include <HttpClient.h>

int loop_counter = 0;
String file_loc = "";
String req = ""; //Request

unsigned long time_open;

//Uno
//#define SS_PIN 9
//#define RST_PIN 8
//Mega
#define SS_PIN 53
#define RST_PIN 49
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key;

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
//IPAddress server(74,125,232,128);  // numeric IP for Google (no DNS)
//char server[] = "mydomain.com";    // name address for Google (using DNS)
//String serverName = "mydomain.com";

String serverName = "www.danbalazs.hu"; // name address for Google (using DNS)

// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 0, 177);
IPAddress myDns(192, 168, 0, 1);

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;

// Variables to measure the speed
unsigned long byteCount = 0;
bool printWebData = true;  // set to false for better speed measurement

byte nuidPICC[4];
String http_response  = "";
int    response_start = 0;
int    response_end   = 0;

char UniqueIDString[(UniqueIDsize * 2) + 1];

String kPath = "";
const int kNetworkTimeout = 30 * 1000;
// Number of milliseconds to wait if no data is available before trying again
const int kNetworkDelay = 1000;

void setup() {

  // You can use Ethernet.init(pin) to configure the CS pin
  Ethernet.init(10);  // Most Arduino shields
  //Ethernet.init(5);   // MKR ETH shield
  //Ethernet.init(0);   // Teensy 2.0
  //Ethernet.init(20);  // Teensy++ 2.0
  //Ethernet.init(15);  // ESP8266 with Adafruit Featherwing Ethernet
  //Ethernet.init(33);  // ESP32 with Adafruit Featherwing Ethernet

  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  Serial.print("Arduino SN: ");
  ///////////////////////////////////////////////////////////////////////Arduino SN///////////////////////////////////////////////////////////////////////
  UniqueIDdump(Serial);  // Prints "UniqueID: 55 35 35 33 31 30 0D 1A 25"

  byte index = 0;
  for (size_t i = 0; i < UniqueIDsize; i++)
  {
    UniqueIDString[index++] = "0123456789ABCDEF"[UniqueID[i] >> 4];
    UniqueIDString[index++] = "0123456789ABCDEF"[UniqueID[i]  & 0x0F];
  }
  UniqueIDString[index++] = 0; // Null Terminator
  Serial.print("UniqueIDString: "); // Prints: 5535353331300D1A25
  Serial.println(UniqueIDString);
  ///////////////////////////////////////////////////////////////////////Arduino SN///////////////////////////////////////////////////////////////////////
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  //pinMode(4, OUTPUT);             //kék
  pinMode(5, OUTPUT);             //zöld
  pinMode(6, OUTPUT);             //piros

  Serial.println(F("This code scan the MIFARE Classsic NUID."));

  // start the Ethernet connection:
  Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
      while (true) {
        delay(1); // do nothing, no point running without Ethernet hardware
      }
    }
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip, myDns);
  } else {
    Serial.print("  DHCP assigned IP ");
    Serial.println(Ethernet.localIP());
    Serial.print("  DNS assigned IP ");
    Serial.println(Ethernet.dnsServerIP());
  }
  // give the Ethernet shield a second to initialize:
  delay(1000);
  Serial.print("connecting to ");
  Serial.print(serverName);
  Serial.println("...");

}

String makeURL(String loc, String arduino_id, String uid){
  String output = "";
  output = loc;
  output += "?";
  if(uid != ""){
    output += "rfid_id=";
    output += uid;
    output += "&";
  }
  output += "arduino_id=";
  output += arduino_id;
  return output;
}

void getDatas(EthernetClient c, String kHostname, String kPath) {
  HttpClient http(c);
  String http_response = "";
  int err = 0;
  err = http.get(kHostname.c_str(), kPath.c_str());
    if (err == 0)
  {
    Serial.println("startedRequest ok");

    err = http.responseStatusCode();
    if (err >= 0)
    {
      Serial.print("Got status code: ");
      Serial.println(err);

      // Usually you'd check that the response code is 200 or a
      // similar "success" code (200-299) before carrying on,
      // but we'll print out whatever response we get

      err = http.skipResponseHeaders();
      if (err >= 0)
      {
        int bodyLen = http.contentLength();
        Serial.print("Content length is: ");
        Serial.println(bodyLen);
        Serial.println();
        Serial.println("Body returned follows:");

        // Now we've got to the body, so we can print it out
        unsigned long timeoutStart = millis();
        char c;
        // Whilst we haven't timed out & haven't reached the end of the body

        while ( (http.connected() || http.available()) &&
                ((millis() - timeoutStart) < kNetworkTimeout) )
        {
          if (http.available())
          {
            c = http.read();
            http_response += c;
            // Print out this character
            Serial.print(c);

            bodyLen--;
            // We read something, reset the timeout counter
            timeoutStart = millis();
          }
          else
          {
            // We haven't got any data, so let's pause to allow some to
            // arrive
            delay(kNetworkDelay);
          }
        }
      }
      else
      {
        Serial.print("Failed to skip response headers: ");
        Serial.println(err);
      }
    }
    else
    {
      Serial.print("Getting response failed: ");
      Serial.println(err);
    }
  }
  else
  {
    Serial.print("Connect failed: ");
    Serial.println(err);
  }
  //Serial.print("Response: ");
  Serial.println(http_response);
  // Serial.println(pathString);
  http.stop();
  
    response_start = http_response.indexOf("<data>") + 6;
    response_end = http_response.indexOf("</data>");
    http_response = http_response.substring(response_start, response_end);
    Serial.print("Website response : ");
    Serial.println(http_response);
    Serial.println();

    if (http_response == "true") {openDoor();}
    if (http_response == "false") {wrongCard();}
  
}

void loop() {
  //Serial.println(loop_counter);
  loop_counter++;
  if (loop_counter > 300) {
    kPath = makeURL("/PHPproject/arduino/arduino_postman/open_door.php", UniqueIDString, "");
    getDatas(client, serverName, kPath);
    loop_counter = 0;
  }

  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been read
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  // Store NUID into nuidPICC array
  for (byte i = 0; i < 4; i++) {
    nuidPICC[i] = rfid.uid.uidByte[i];
  }
  String uid = getDec(rfid.uid.uidByte, rfid.uid.size);
  Serial.println(uid);

  kPath = makeURL("/PHPproject/arduino/arduino_postman/rfid_test.php", UniqueIDString, uid);
  getDatas(client, serverName, kPath);


  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}

void openDoor() {
  Serial.print("OPEN DOOR");
  digitalWrite(5, HIGH);
  delay(5000);
  digitalWrite(5, LOW);

}

void wrongCard() {
  Serial.print("NOT OPEN DOOR");

  digitalWrite(6, HIGH);
  delay(2000);
  digitalWrite(6, LOW);
}

String getDec(byte *buffer, byte bufferSize) {
  String output = "";
  for (byte i = 0; i < bufferSize; i++) {
    //Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    //Serial.print(buffer[i], DEC);
    output = output + String(buffer[i], DEC);
  }
  return output;
}

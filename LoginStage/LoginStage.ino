/*
  gadget-cache-loginstage

  A gadget cache that is part of a multi stage night-cache.

  The ESP powers up and the OLED displays a progress bar.
  After that a WifiSymbol appears.
  Next you have to connect your phone to the wifihotspot the ESP just started sharing.
  You will get an automatic Captive Portal notification.
  This will directly point you the the loginpage (On some phones this does´nt work.
  In that case enter the URL 192.168.1.1 into your browser).
  There you have to enter your username and password.
  If successfully entered the Oled display will show another progress par.
  In the background it will send the batterylevel, the username, and the
  calculated time remaining till you have to swap batterys to IFTT.com.
  This service than sends me a notification with all that information via Telegram.

  modified 9 April 2018
  by chhofi

  Find on GitHub
  https://github.com/chhofi/gadget-cache-loginstage
*/


// --------------- Libraries Start

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

// --------------- Libraries End


// --------------- OLED Start

#include <Wire.h>
#include "SH1106Brzo.h"
#include <brzo_i2c.h>
#include "images.h"
SH1106Brzo  display(0x3c, D5, D6); //connected via I2C

// --------------- OLED End

// --------------- PINOUT Start

int inputLed = D4;  //Photodiode connected on pin D4
int outputTrans = D0; //Transistor for powerering the SIM800 module

// --------------- PINOUT End


// --------------- Start SIM800 Start

// Select your modem:
#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h>


const char apn[]  = "pinternet.interkom.de"; // Your GPRS credentials

const char user_gsm[] = "";                     // Leave empty, if missing user or pass
const char pass_gsm[] = "";

#include <SoftwareSerial.h>
SoftwareSerial SerialAT(D2, D1); // RX, TX

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);


const char server[] = "maker.ifttt.com";
const int port = 80;

// --------------- SIM800 End


// --------------- WIFI Settings Start

String proband;                //name of the player
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
ESP8266WebServer webServer(80);
int loginVersuche = 0;
String anchars = "abcdefghijklmnopqrstuvwxyz0123456789", username = "admin", loginPassword = "admin"; //username will be compared with 'user' and loginPassword with 'pass' when login is done
const String loginPage = "<!DOCTYPE html><html><body style='background-color:black;'><head><title>ReptiloIdent</title></head><body><div id=\"login\"> <form action='/login' method='POST'> <center> <h1><font face='courier new'><font color=#00C000>REGISTRATION REPTO</font></h1><p><input type='text' name='user' placeholder='Identification'></p><p><input type='password' name='pass' placeholder='Unique Passphrase'></p><br><button type='submit' name='submit'>login</button></center><br><br><marquee behavior='alternate' direction='left'scrolldelay='5'scrollamount='5'><font face='courier new'size='4'color='#00C000'>A=1</font</body></html></form></body></html>";
const String loginOk = ""
                       "<!DOCTYPE html><html><body style='background-color:black;'><head><title>Identifikationverifikation</title></head><body>"
                       "<h1><font face='courier new'><font color=#00C000>Identifikation erfolgreich</font></h1><p><font face='courier new'><font color=#00C000>Bitte folgen Sie den weiteren Identifikationsschritten</font></p></body></html>";

// --------------- End WIFI Settings--------------


int send_starttime = 0; // for calculating how long the power of the device will last

ADC_MODE(ADC_VCC); //Enable measure voltage mode


// --------------- Initalistation Start ---------------

void setup() {

  pinMode(inputLed, INPUT);
  pinMode(outputTrans, OUTPUT);
  digitalWrite(outputTrans, LOW);


  // --------------- OLED Start
  // Initialising the UI will init the display too.
  display.init();
  //display.flipScreenVertically();
  //display.flipScreenHorizontally();

  display.setFont(ArialMT_Plain_10);
  // --------------- OLED End


  Serial.begin(115200); // For debugging

  ledactivate(); // Start webServer when photodiode gets hit by light

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("Repto_Authentification");

  // if DNSServer is started with "*" for domain name, it will reply with
  // provided IP to all DNS request
  dnsServer.start(DNS_PORT, "*", apIP);

  webServer.on("/login", handleLogin);


  // replay to all requests with same HTML
  webServer.onNotFound([]() {
    webServer.send(200, "text/html", loginPage);
  });
  webServer.begin();

  //Show progress bar
  drawProgressBarRange();
  delay(1000);           //Wait for 1 second

  //Draw wifi Logo
  drawWifi();
}

// --------------- Initalistation End ---------------

void ledactivate() {
  while (digitalRead(inputLed) == 1) {          //Check if photodiode got activted
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 0, "Licht-");
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 20, "Authentifizierung");
    display.display();
    if ((millis() / 1000) % 5 == 0) //Write the passed operating time to the EEPROM every 5 seconds
    { updaterestworkingtime();
    }
    delay(1000);
  }
}


void drawProgressBarRange() {  
  display.setFont(ArialMT_Plain_24);
  for (int i = 0; i <= 100; i++)
  {
    display.clear();
    int progress = i;
    // draw the progress bar
    display.drawProgressBar(0, 45, 120, 10, progress);
    // draw the percentage as String
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 15, String(progress) + "%");
    display.display();
    delay(30);
  }
}


void drawProgressBarRange(int startpercent, int endpercent) {
  display.setFont(ArialMT_Plain_24);
  for (int i = startpercent; i <= endpercent; i++)
  {
    display.clear();
    int progress = i;
    // draw the progress bar
    display.drawProgressBar(0, 45, 120, 10, progress);
    // draw the percentage as String
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 15, String(progress) + "%");
    display.display();

    delay(10);

  }
}

void drawWifi() {
  // see http://blog.squix.org/2015/05/esp8266-nodemcu-how-to-create-xbm.html
  // on how to create xbm files
  display.clear();
  display.drawXbm(34, 14, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);

  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_16);
  display.drawString(64, 47, "http://192.168.1.1");

  display.display();
}


int restworkingtime() { //return in seconds

  int leftTime = 108000 - (readprom().toInt()); //The battery has a capacity of 1500mAh x 3 = 4500mAh you can calculate the operating time with online tools. The result in my case was an operating time of 30 hours = 30h *60min *60sec => 108000 seconds
  return leftTime;

}

int updaterestworkingtime() {
  if (send_starttime  == 0) { // After the first initalisation take the runtime and add it to the falue of the EEPROM
    int timeNow = millis() / 1000;
    int newTime = readprom().toInt() + timeNow;


    EEPROM.begin(512);      // REST the EEPROM
    // write a 0 to all 512 bytes of the EEPROM
    for (int i = 0; i < 512; i++)
    EEPROM.write(i, 0);
    EEPROM.end();


    writetoprom(String(newTime)); // Write the old + the current operation time to the EEPROM

    send_starttime  = timeNow;


  }

  else {
    int timeNow = millis() / 1000;
    int newTime = readprom().toInt() + (timeNow - send_starttime);

    EEPROM.begin(512);
    // write a 0 to all 512 bytes of the EEPROM
    for (int i = 0; i < 512; i++)
    EEPROM.write(i, 0);
    EEPROM.end();

    writetoprom(String(newTime));

    send_starttime  = timeNow;

  }
}

void writetoprom(String content) {
  EEPROM.begin(512);

  //Serial.println("writing eeprom ssid:");
  for (int i = 0; i < content.length(); ++i)
  {
    EEPROM.write(i, content[i]);
    //Serial.print("Wrote: ");
    // Serial.println(content[i]);
  }
  EEPROM.end();
}

String readprom() {
  EEPROM.begin(512);

  //Serial.println("Reading EEPROM ssid");
  String esid;
  for (int i = 0; i < 100; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  //Serial.print("SSID: ");
  //Serial.println(esid);
  return esid;
  
  EEPROM.end();
}

void sendgsm() {
  digitalWrite(outputTrans, HIGH); // Activate the transistor to power the SIM800 Module
  delay(10);
  updaterestworkingtime();

  String messagecontent = "/trigger/geocache/with/key/your_secret_key?value1="; // IFTT Webhooks key
  messagecontent = messagecontent + proband;
  messagecontent = messagecontent + "&value2=";
  messagecontent = messagecontent + String(readbattervoltage());
  messagecontent = messagecontent + "&value3=";
  messagecontent = messagecontent + String(restworkingtime() / 60); // in minutes


  Serial.println(messagecontent);

  // Set GSM module baud rate
  SerialAT.begin(115200);
  delay(1500);// for save operating set to 3000

  drawProgressBarRange(0, 30);   //Progressbar 30 percent

  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "1MIN Wartezeit ..."); // Translation: 1min waiting time
  display.display();

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  Serial.println(F("Initializing modem..."));
  modem.init();

  Serial.print(F("Waiting for network..."));
  if (!modem.waitForNetwork()) {
    Serial.println(" fail");
    drawSuccess();
    delay(10000);
    return;
  }
  Serial.println(" OK");

  drawProgressBarRange(30, 60); //progressbar 60 percent
  
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "1MIN Wartezeit ...");
  display.display();

  Serial.print(F("Connecting to "));
  Serial.print(apn);
  if (!modem.gprsConnect(apn, user_gsm, pass_gsm)) {
    Serial.println(" fail");
    drawSuccess();
    delay(10000);
    return;
  }
  Serial.println(" OK");

  drawProgressBarRange(60, 80); //progressbar 80 percent


  Serial.print(F("Connecting to "));
  Serial.print(server);
  if (!client.connect(server, port)) {
    Serial.println(" fail");
    drawSuccess();
    delay(10000);
    return;
  }
  Serial.println(" OK");

  // Make a HTTP GET request:
  client.print(String("POST ") + messagecontent + " HTTP/1.0\r\n");
  client.print(String("Host: ") + server + "\r\n");
  client.print("Connection: close\r\n\r\n");

  drawProgressBarRange(89, 90); //progressbar 90 percent

  client.stop();
  Serial.println("Server disconnected");

  modem.gprsDisconnect();
  Serial.println("GPRS disconnected");

  drawProgressBarRange(90, 100);  //progressbar 100 percent

  digitalWrite(outputTrans, LOW); 
  updaterestworkingtime();

}

void drawSuccess() {
  int startzeit = millis();
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_24);
  display.drawString(64, 22, "A=1");
  display.display();

  delay(3000);

  while (1 == 1) {
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 0, "Willkommen");

    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 20, "Proband");

    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 40, proband);
    display.display();

    delay(2000);

    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 0, "Authentifizierung");

    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 20, "wird vorbereitet");

    delay(2000);

    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 0, "Begebe dich");

    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 20, "jetzt");

    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 40, "schnell zu");
    display.display();

    delay(2000);

    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_24);
    display.drawString(0, 0, "N 49*");

    display.setFont(ArialMT_Plain_24);
    display.drawString(0, 30, "28.039");
    display.display();

    delay(6000);

    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_24);
    display.drawString(0, 0, "N 28*");

    display.setFont(ArialMT_Plain_24);
    display.drawString(0, 30, "17.220");
    display.display();
    updaterestworkingtime();
    delay(6000);
  }

}

int generatepassphrase(char charBuf[]) {         // Generates a passphrase with the letters of the username 
  int sum = 0;
  for (int i = 0; i < sizeof(charBuf); i++) {   

    sum = sum + (charBuf[i], HEX);              // Takes the HEX value of the letter and sum them up

  }

  return sum * 10 + 234455;               // Some math so that the code is not so easy to hack

}

void handleLogin() {  // --------------- If the "OK" button on the website was pressed this function gets called

  String  msg = "<center><br>";
  proband = webServer.arg("user");

  char charBuf[50];
  proband.toCharArray(charBuf, 50);

  if (proband == "reseteverything") { //reset EEPROM // --------------- If you want to reset the operating time write "reseteverything" int the "username" field on the webpage and klick "OK"
    EEPROM.begin(512);
    // write a 0 to all 512 bytes of the EEPROM
    for (int i = 0; i < 512; i++)
    EEPROM.write(i, 0);
    EEPROM.end();
  }

  if (webServer.arg("pass") == String(generatepassphrase(charBuf))) { //check if login details are good
    webServer.send(200, "text/html", loginOk);
    sendgsm();
    drawSuccess();

  }
  else {

    if (loginVersuche < 5) {
      msg += ".           Falsche Identifikation ";
      msg += "Sie muessen es erneut versuchen            .";
      loginVersuche = loginVersuche++;
    }

    else if (loginVersuche < 10) {
      msg += "Tipp: ID= Kalender Registrierung";
      msg += "Tipp: PW= Online bereits bekannt";

    }

    else {
      msg += "Lösung: admin";
      msg += "Lösung: admin";
    }
    String content = loginPage;
    content +=  msg + "</center>";
    webServer.send(200, "text/html", content); //merge loginPage and msg and send it

  }

}

float readbattervoltage() { // reading the battery voltage
  float voltage = 0.00f;
  float temp = 0.00f;
  int i = 0;

  while (i < 5) { // 5 measurements and than calculating the average
    temp = temp + ESP.getVcc();
    delay(10);
    i ++;
  }
  voltage = temp / 5;

  return (voltage / 1024) + 0.66; // in vol + adding the offset measured with a multimeter
}

void loop() {

  dnsServer.processNextRequest();
  webServer.handleClient();

}

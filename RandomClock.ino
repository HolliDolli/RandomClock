/*
  RandomClock

  Display a Clock with an ws2812(b)
  
  Achtung: Random(min,max) : max is exclusiv!
  
  Upload-PW: admin

  Projekt liegt auf Github, also GitDesk laden !!
*/

//What we need:
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
#include <ESP8266WiFi.h>
#include <coredecls.h> // optional settimeofday_cb() callback to check on server
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <time.h>
#include <Adafruit_NeoPixel.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
 
 
#include "d:\mysecrets.h"

//EEPROM Position and Length data
#define LENheader 5
#define LENssid 32 // SSID hat maximal 32 Stellen
#define LENpass 64 // Password hat maximal 64 Stellen, also genau 63, aber egal.
#define POSheader 0
#define POSssid POSheader+LENheader
#define POSpass POSssid+LENssid

#define MY_NTP_SERVER "de.pool.ntp.org"           
#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"   

// jomjol nimmt D7, Neopixel Doku ESPHome empfiehlt GPIO3
#define PIN        D6 // Which pin on the Arduino is connected to the NeoPixels?  // On Trinket or Gemma, suggest changing this to 1
#define NUMPIXELS 27 // How many NeoPixels are attached to the Arduino? 


//Some Initstuff
LiquidCrystal_PCF8574 lcd(0x27);  // set the LCD address to 0x27 for a 16 chars and 2 line display


// When setting up the NeoPixel library, we tell it how many pixels,
// and which pin to use to send signals. Note that for older NeoPixel
// strips you might need to change the third parameter -- see the
// strandtest example for more information on possible values.
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

bool testWifi(void);
void launchWeb(void);
void setupAP(void);
 
//--------Establishing Local server at port 80 whenever required
ESP8266WebServer server(80);
 
//some Vars we need
int show = -1;
int lastHH = -1;
int lastMM = -1;
time_t now;                         // this is the epoch
tm tm;                              // the structure tm holds time information in a more convient way
boolean timeIsSet = false;

int i = 0;
int statusCode;
String st;
String content;

String EEssid;
String EEpass;
String EEheader = "HGURC";


// 2 custom characters, brauchen wir nicht, lassen wir aber als Gedaechtnisstuetze drin.
/*
byte dotOff[] = { 0b00000, 0b01110, 0b10001, 0b10001,
                  0b10001, 0b01110, 0b00000, 0b00000 };
byte dotOn[] = { 0b00000, 0b01110, 0b11111, 0b11111,
                 0b11111, 0b01110, 0b00000, 0b00000 };
*/

//Functions used for saving WiFi credentials and to connect to it which you do not need to change 
bool testWifi(void)
{
  int c = 0;
  Serial.println("Waiting for WiFi to connect");
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED)
    {
      return true;
    }
    Serial.print("*");
    delay(500);
    c++;
  }
  Serial.println("");
  Serial.println("Connection timed out, opening AP or Hotspot");
  return false;
}
 
void launchWeb()
{
  Serial.println("");
  if (WiFi.status() == WL_CONNECTED)
    Serial.println("WiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  createWebServer();
  // Start the server
  server.begin();
  Serial.println("Server started");
}
 
void setupAP(void)
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan completed");
  if (n == 0)
    Serial.println("No WiFi Networks found");
  else
  {
    Serial.print(n);
    Serial.println(" Networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }
  Serial.println("");
  st = "<ol>";
  for (int i = 0; i < n; ++i)
  {
    // Print SSID and RSSI for each network found
    st += "<li>";
    st += i;
    st += " - ";
    st += WiFi.SSID(i);
    st += " (";
    st += WiFi.RSSI(i);
 
    st += ")";
    st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
    st += "</li>";
  }
  st += "</ol>";
  delay(100);
  WiFi.softAP("RandomClock", "");
  Serial.println("Initializing_Wifi_accesspoint");
  launchWeb();
  Serial.println("over");
}
  
void createWebServer()
{
 {
    server.on("/", []() {
 
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html>RandomClock ESP8266 WiFi Connectivity Setup ";
      content += "<form action=\"/scan\" method=\"POST\"><input type=\"submit\" value=\"scan\"></form>";
      content += ipStr;
      content += "<p>";
      content += st;
      content += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32><input name='pass' length=64><input type='submit'></form>";
      content += "</html>";
      server.send(200, "text/html", content);
    });
    server.on("/scan", []() {
      //setupAP();
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
 
      content = "<!DOCTYPE HTML>\r\n<html>go back";
      server.send(200, "text/html", content);
    });
 
    server.on("/setting", []() {
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      if (qsid.length() > 0 && qpass.length() > 0) {
        Serial.println("clearing eeprom");
        for (int i = 0; i < 101; ++i) {
          EEPROM.write(i, 0);
        }
        Serial.println(qsid);
        Serial.println("");
        Serial.println(qpass);
        Serial.println("");
 
        Serial.println("writing eeprom header:");
        for (int i = 0; i < LENheader; ++i)
        {
          EEPROM.write(POSheader+i, EEheader[i]);
          Serial.print("Wrote: ");
          Serial.println(EEheader[i]);
        }
        Serial.println("writing eeprom ssid:");
        if (qsid.length() == 1){
          qsid = WiFi.SSID(static_cast<int>(qsid[0]));
        }
        for (int i = 0; i < qsid.length(); ++i)
        {
          EEPROM.write(POSssid+i, qsid[i]);
          Serial.print("Wrote: ");
          Serial.println(qsid[i]);
        }
        Serial.println("writing eeprom pass:");
        for (int i = 0; i < qpass.length(); ++i)
        {
          EEPROM.write(POSpass + i, qpass[i]);
          Serial.print("Wrote: ");
          Serial.println(qpass[i]);
        }
        EEPROM.commit();
 
        content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
        statusCode = 200;
        ESP.reset();
      } else {
        content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
        Serial.println("Sending 404");
      }
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "application/json", content);
 
    });
  } 
} 
 
void wifiReconnect(){
  // Beim Reconnect holen wir nicht nochmal die Config-Daten und da gibt es auchen keinen AP. Basta. Kein rumtragen!
    Serial.println ("Reconnecting");
    //xlcd.home();
    //xlcd.clear();
    //xlcd.print("Reinit WiFi");
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(EEssid.c_str(), EEpass.c_str());
    while ( WiFi.status() != WL_CONNECTED ) {
      Serial.print ( "." );
      delay ( 500 );
    }
    Serial.println(WiFi.localIP());
    //xlcd.print(" done");
    //xlcd.home();
    //xlcd.clear();
    //xlcd.print("Init NTP");
    configTime(MY_TZ, MY_NTP_SERVER);
    //xlcd.print(" done");

}



void time_is_set(bool from_sntp /* <= this optional parameter can be used with ESP8266 Core 3.0.0*/) {
  Serial.print(F("time was sent! from_sntp="));
  Serial.print(from_sntp);
  timeIsSet = from_sntp;
}

void getDefaultConfig(){
  Serial.println("using default config");
  EEssid = ssid;  
  EEpass = password;
}

void getEEConfig() {
  String EEID;
  
  Serial.println("checking EEPROM for ID");
  
  for (int i = 0; i < LENheader; i++){
    EEID += char(EEPROM.read(POSheader+i));
  }
  
  Serial.print("ID found:");
  Serial.println(EEID);
  if (EEID != EEheader) {  //Haben wir den Header HGURC gefunden?
    getDefaultConfig();
  } else {
    Serial.println("ID ok, reading data");
    EEssid = "";
    for (int i = 0; i<LENssid; i++){ 
      EEssid += char(EEPROM.read(POSssid+i));
    }
    Serial.println(EEssid);
    EEpass = "";
    for (int i = 0; i<LENpass; i++){ 
      EEpass += char(EEPROM.read(POSpass+i));
    }
    Serial.println(EEpass);
  }
}


// the setup function runs once when you press reset or power the board
void setup() {
  int error;
  Serial.begin(115200);
  Serial.println("Setting Callback");
  settimeofday_cb(time_is_set); // optional: callback if time was sent, funktioniert leider nicht. :-(
  Serial.println("LCD...");
  // wait on Serial to be available on Leonardo
  while (!Serial)
    ;
  Serial.println("Probing for PCF8574 on address 0x27...");
  Wire.begin();
  Wire.beginTransmission(0x27);
  error = Wire.endTransmission();
  Serial.print("Error: ");
  Serial.print(error);
  if (error == 0) {
    Serial.println(": LCD found.");
    show = 0;
    //xlcd.begin(16, 2);  // initialize the lcd
//    //xlcd.createChar(1, dotOff);
//    //xlcd.createChar(2, dotOn);
    //xlcd.setBacklight(255);
  } else {
    Serial.println(": LCD not found.");
  }  // 
  
  EEPROM.begin(512); //Initializing EEPROM
  delay(10);
  getEEConfig();
  
  
  //xlcd.home();
  //xlcd.clear();
  //xlcd.print("Init WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(EEssid.c_str(), EEpass.c_str());
  if (testWifi() ) {
    Serial.println("WiFi success");
  } else {
    launchWeb(); //den verstehe ich nicht, der kommt am ende von setupAP nochmal. hmmmm?
    setupAP();
    while (true){
      Serial.println("waiting");
      while ((WiFi.status() != WL_CONNECTED))
      {
        Serial.print(".");
        delay(100);
        server.handleClient();
      }
    }
  } 
  Serial.println(WiFi.localIP());
  //xlcd.print(" done");
  //xlcd.home();
  //xlcd.clear();
  //xlcd.print("Init NTP");
  configTime(MY_TZ, MY_NTP_SERVER);
  //xlcd.print(" done");
  //xlcd.home();
  //xlcd.clear();
  //xlcd.print("Init Neopixel");
  Serial.print("NP-");
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)  
  Serial.print("NP+");
  pixels.show(); // alle Pixel aus.
  pixels.setBrightness(10); // Helligkeit maximal 255
  for (int i = 0 ; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(255, 255, 255));
  }
  pixels.show(); // alle Pixel aus.
  delay(1000);
  //xlcd.print(" done");
  pinMode(LED_BUILTIN, OUTPUT); //Damit die LED SOS funken kann. :-)
  
  

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}



void LCDCMD() {
    if (show == 0) {
    //xlcd.setBacklight(255);
    //xlcd.home();
    //xlcd.clear();
    //xlcd.print("Hello LCD");
    delay(1000);
    //xlcd.setBacklight(0);
    delay(400);
    //xlcd.setBacklight(255);
  } else if (show == 1) {
    //xlcd.clear();
    //xlcd.print("Cursor On");
    //xlcd.cursor();
  } else if (show == 2) {
    //xlcd.clear();
    //xlcd.print("Cursor Blink");
    //xlcd.blink();
  } else if (show == 3) {
    //xlcd.clear();
    //xlcd.print("Cursor OFF");
    //xlcd.noBlink();
    //xlcd.noCursor();
  } else if (show == 4) {
    //xlcd.clear();
    //xlcd.print("Display Off");
    //xlcd.noDisplay();
  } else if (show == 5) {
    //xlcd.clear();
    //xlcd.print("Display On");
    //xlcd.display();
  } else if (show == 7) {
    //xlcd.clear();
    //xlcd.setCursor(0, 0);
    //xlcd.print("*** first line.");
    //xlcd.setCursor(0, 1);
    //xlcd.print("*** second line.");
  } else if (show == 8) {
    //xlcd.scrollDisplayLeft();
  } else if (show == 9) {
    //xlcd.scrollDisplayLeft();
  } else if (show == 10) {
    //xlcd.scrollDisplayLeft();
  } else if (show == 11) {
    //xlcd.scrollDisplayRight();
  } else if (show == 12) {
    //xlcd.clear();
    //xlcd.print("write-");
  } else if (show == 13) {
    //xlcd.clear();
    //xlcd.print("custom 1:<\01>");
    //xlcd.setCursor(0, 1);
    //xlcd.print("custom 2:<\02>");
  } else {
    //xlcd.print(show - 13);
  }  // if
  delay(1400);
  show = (show + 1) % 16;
}


// the loop function runs over and over again forever
void loop() {
  boolean done; 
  int target;
  int ind;
  int z1;
  boolean ledH2[9]; 
  boolean ledM1[6];
  boolean ledM2[9];
  
  ArduinoOTA.handle();


  time(&now);                       // read the current time
  localtime_r(&now, &tm);           // update the structure tm with the current time
// Falls der update nicht geklappt hat bauen wir alle Verbindungen neu auf.
  if ((!timeIsSet) or ((lastHH == 6) and (lastMM == 0)) or ((lastHH == 18) and (lastMM == 0))) { //reconnect wenn nicht erreichbar, oder um 6 Uhr.
    wifiReconnect();  
    time(&now);                       // read the current time
    localtime_r(&now, &tm);           // update the structure tm with the current time
  }  
  
// Hier warten wir bis die neue Minute angebrochen ist.
  while ( (lastHH == tm.tm_hour) and (lastMM == tm.tm_min) ) {
    Serial.print ( "-" );
    delay(500);
    time(&now);                       // read the current time
    localtime_r(&now, &tm);           // update the structure tm with the current time
  }  

  lastHH = tm.tm_hour;
  lastMM = tm.tm_min;
  
//Ausgabe der aktuellen Zeit auf Serial und LCD  
  Serial.println();
  Serial.print(tm.tm_hour);
  Serial.print(":");
  Serial.print(tm.tm_min);
  Serial.print(":");
  Serial.println(tm.tm_sec);
  //xlcd.home();
  //xlcd.clear();
  //xlcd.print(tm.tm_hour);
  //xlcd.print(":");
  //xlcd.print(tm.tm_min);
  //xlcd.print(":");
  //xlcd.print(tm.tm_sec);

// Ermitteln der n zufälligen LEDs für die 1 Stelle der Stunde
  boolean ledH1[] = {false, false, false};
  target = lastHH / 10;  
  for (int i = 0; i < target; i++) {
    done = false;
    while (!done) {    
      ind = random(0, 3);
      if (!ledH1[ind]) {
        ledH1[ind] = true;    
        done = true;      
      }    
    }  
  }
// Ermitteln der n zufälligen LEDs für die 2 Stelle der Stunde
  done = false;
  z1 = lastHH /10;
  target = lastHH - (z1 * 10);
  switch (target) {
  case 6 ... 9:  
    for (int i=0; i<9; i++){
      ledH2[i] = true;      
    }              
    for (int i = 9; i > target; i=i-1){
      done = false;
      while (!done) {    
        ind = random(0, 9);
        if (ledH2[ind]) {
          ledH2[ind] = false;    
          done = true;      
        }    
      }  
    }
    break;
    
  default:  
    for (int i=0; i<9; i++){
      ledH2[i] = false;      
    }              
    for (int i = 0; i < target; i=i+1){
      done = false;
      while (!done) {    
        ind = random(0, 9);
        if (!ledH2[ind]) {
          ledH2[ind] = true;    
          done = true;      
        }    
      }  
    }
    break;
  }
   
// Ermitteln der n zufälligen LEDs für die 1 Stelle der Minute
  done = false;
  target = lastMM / 10;
  switch (target) {
  case 3 ... 5:  
    for (int i=0; i<6; i++){
      ledM1[i] = true;      
    }              
    for (int i = 6; i > target; i=i-1){
      done = false;
      while (!done) {    
        ind = random(0, 6);
        if (ledM1[ind]) {
          ledM1[ind] = false;    
          done = true;      
        }    
      }  
    }
    break;
    
  default:  
    for (int i=0; i<6; i++){
      ledM1[i] = false;      
    }              
    for (int i = 0; i < target; i=i+1){
      done = false;
      while (!done) {    
        ind = random(0, 6);
        if (!ledM1[ind]) {
          ledM1[ind] = true;    
          done = true;      
        }    
      }  
    }
    break;
  }

// Ermitteln der n zufälligen LEDs für die 2 Stelle der Minute
  done = false;
  z1 = lastMM /10;
  target = lastMM - (z1 * 10);
  switch (target) {
  case 6 ... 9:  
    for (int i=0; i<9; i++){
      ledM2[i] = true;      
    }              
    for (int i = 9; i > target; i=i-1){
      done = false;
      while (!done) {    
        ind = random(0, 9);
        if (ledM2[ind]) {
          ledM2[ind] = false;    
          done = true;      
        }    
      }  
    }
    break;
    
  default:  
    for (int i=0; i<9; i++){
      ledM2[i] = false;      
    }              
    for (int i = 0; i < target; i=i+1){
      done = false;
      while (!done) {    
        ind = random(0, 9);
        if (!ledM2[ind]) {
          ledM2[ind] = true;    
          done = true;      
        }    
      }  
    }
    break;
  }
  pixels.clear(); // Set all pixel colors to 'off'
  pixels.setBrightness(10); // Helligkeit maximal 255
  int pix = 0;
  for (int i=0; i<3; i++){
    Serial.print(ledH1[i]);  
    if (ledH1[i]) {    
      pixels.setPixelColor(pix, pixels.Color(51, 255, 153));
    } else {
      pixels.setPixelColor(pix, pixels.Color(0, 0, 0));
    }      
    pix++;
  }
  Serial.println(pix);
  for (int i=0; i<9; i++){
    Serial.print(ledH2[i]);  
    if (ledH2[i]) {    
      pixels.setPixelColor(pix, pixels.Color(255, 255, 51));
    } else {
      pixels.setPixelColor(pix, pixels.Color(0, 0, 0));
    }      
    pix++;
  }
  Serial.println(pix);
  for (int i=0; i<6; i++){
    Serial.print(ledM1[i]);  
    if (ledM1[i]) {    
      pixels.setPixelColor(pix, pixels.Color(255, 51, 255));
    } else {
      pixels.setPixelColor(pix, pixels.Color(0, 0, 0));
    }      
    pix++;
  }
  Serial.println(pix);
  for (int i=0; i<9; i++){
    Serial.print(ledM2[i]);  
    if (ledM2[i]) {    
      pixels.setPixelColor(pix, pixels.Color(0, 153, 153));
    } else {
      pixels.setPixelColor(pix, pixels.Color(0, 0, 0));
    }      
    pix++;
  }
  Serial.println(pix);
  pixels.show();   // Send the updated pixel colors to the hardware.
//Den Rest der angebrochenen Minute legen wir uns schlafen.
  while (tm.tm_sec != 59) {
    ArduinoOTA.handle();
    time(&now);                       // read the current time
    localtime_r(&now, &tm);           // update the structure tm with the current time
    delay(500); 
  }
}

#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <time.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Orbitron_Medium_20.h"
#include "ani.h"
#include "env.h"

// Define colors for display
#define TFT_GREY 0x5AEB
#define TFT_LIGHTBLUE 0x01E9
#define TFT_DARKRED 0xA041
#define TFT_BLUE 0x5D9B

TFT_eSPI tft = TFT_eSPI();
StaticJsonDocument<1000> doc;

// Display settings
const int pwmFreq = 5000;
const int pwmResolution = 8;
const int pwmLedChannelTFT = 0;

// Time settings
const char* ntpServer = "pool.ntp.org";
const int gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;
struct tm timeinfo;
// Store date objects
char dateFormat[80];
char secondFormat[80];
char timeFormat[80];

// Open Weather API
const String town = "Oslo";
const String country = "NO";
const String endpoint = "http://api.openweathermap.org/data/2.5/weather?q="+town+","+country+"&units=metric&APPID=";

// Data
String payload = "";  // whole json 
String tmp = "";      // temperature
String hum = "";      // humidity

int backlight[5] = {10,30,60,120,220};  // Brightness options
byte b=1;                               // Brighthess level

void printLocalTime() {
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  strftime(dateFormat,80,"%d %B %Y", &timeinfo);
  strftime(timeFormat,80,"%H:%M", &timeinfo);
  strftime(secondFormat,80,"%S", &timeinfo);
  
  Serial.println(&timeinfo, "%A, %d %B %Y %H:%M:%S");
}

void setup(void) {
  pinMode(0,INPUT_PULLUP);
  pinMode(35,INPUT);

  // Initialize screen
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);
  tft.setTextSize(1);

  ledcSetup(pwmLedChannelTFT, pwmFreq, pwmResolution);
  ledcAttachPin(TFT_BL, pwmLedChannelTFT);
  ledcWrite(pwmLedChannelTFT, backlight[b]);

  Serial.begin(115200);

  // Initialize WiFi
  tft.print("Connecting to ");
  tft.println(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    tft.print(".");
  }
  
  tft.println("");
  tft.println("WiFi connected.");
  tft.println("IP address: ");
  tft.println(WiFi.localIP());
  delay(3000);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

  // Screen setup
  tft.setTextColor(TFT_WHITE,TFT_BLACK);
  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true);

  // IP address
  tft.setCursor(2, 232, 1);
  tft.println(WiFi.localIP());

  // Bright label
  tft.setCursor(80, 204, 1);
  tft.println("BRIGHT:");

  // Seconds label
  tft.setCursor(80, 152, 2);
  tft.println("SEC:");

  // Temperature label
  tft.setTextColor(TFT_WHITE,TFT_LIGHTBLUE);
  tft.setCursor(4, 152, 2);
  tft.println("TEMP:");

  // Humidity label
  tft.setCursor(4, 192, 2);
  tft.println("HUM: ");

  // Town
  tft.setTextColor(TFT_WHITE,TFT_BLACK);
  tft.setFreeFont(&Orbitron_Medium_20);
  tft.setCursor(6, 82);
  tft.println(town);

  // Vertical line
  tft.fillRect(68,152,1,74,TFT_GREY);

  // Brightness status
  for(int i=0;i<b+1;i++) {
    tft.fillRect(78+(i*7),216,3,10,TFT_BLUE);
  }
}

int counter = 0;        // Count time after last data update
int period = 2000;      // Period for data request
int btnLeft = 0;        // Invert display colors
int btnRight = 0;       // Brightness controll
int frame = 0;          // Frame for animation
String currentDate;
String currentSec;
String currentTime;
bool inverted = 1;      // Display invert state

void loop() {
  // Animation
  tft.pushImage(0, 88,  135, 65, ani[frame]);
  frame++;
  if(frame>=10) {
    frame=0;
  }

  // Brightness control
  if(digitalRead(35)==0) {
    if(btnRight==0) {
      btnRight=1;
      tft.fillRect(78,216,44,12,TFT_BLACK);
      b++;
      
      if(b>=5) {
        b=0;
      }
  
      for(int i=0;i<b+1;i++) {
        tft.fillRect(78+(i*7),216,3,10,TFT_BLUE);
        ledcWrite(pwmLedChannelTFT, backlight[b]);
      }
    }
  } else {
    btnRight=0;
  }

  // Invert screen color
  if(digitalRead(0)==0) {
    if(btnLeft==0) {
      btnLeft=1;
      inverted=!inverted;
      tft.invertDisplay(inverted);
    }
  } else {
    btnLeft=0;
  }

  // Data request every period
  if(counter==0) {
    getData();
    counter++;
  }
    
  if(counter>period) {
    counter=0;
  }

  // Temperature value
  tft.setFreeFont(&Orbitron_Medium_20);
  tft.setCursor(2, 187);
  tft.println(tmp.substring(0,2));

  // Humidity value
  tft.setCursor(2, 227);
  tft.println(hum+"%");

  printLocalTime();

  // Display date
  if (currentDate != dateFormat) {
    currentDate = dateFormat;
    tft.setTextColor(TFT_ORANGE,TFT_BLACK);
    tft.setTextFont(2);
    tft.setCursor(6, 44);
    tft.println(dateFormat);
  }

  // Display seconds
  if (currentSec != secondFormat) {
    currentSec = secondFormat;
    tft.setTextColor(TFT_WHITE,TFT_BLACK);
    tft.fillRect(78,170,48,28,TFT_DARKRED);
    tft.setFreeFont(&Orbitron_Light_24);
    tft.setCursor(81, 192);
    tft.println(secondFormat);
  }

  // Display time
  if (currentTime != timeFormat) {
    currentTime = timeFormat;
    tft.setFreeFont(&Orbitron_Light_32);
    tft.fillRect(3,8,120,30,TFT_BLACK);
    tft.setCursor(5, 34);
    tft.println(timeFormat);
  }
  
  delay(80);
}

void getData() {
  tft.fillRect(1,170,64,20,TFT_BLACK);
  tft.fillRect(1,210,64,20,TFT_BLACK);
  if (WiFi.status() == WL_CONNECTED) { //Check the current connection status
 
    HTTPClient http;
 
    http.begin(endpoint + key); //Specify the URL
    int httpCode = http.GET();  //Make the request
 
    if (httpCode > 0) {
      payload = http.getString();
      Serial.println(payload);
    } else {
      Serial.println("Error on HTTP request");
    }
 
    http.end(); //Free the resources
  }
  char inp[1000];
  payload.toCharArray(inp,1000);
  deserializeJson(doc,inp);
  
  String tmp2 = doc["main"]["temp"];
  String hum2 = doc["main"]["humidity"];
  tmp=tmp2;
  hum=hum2;
 }

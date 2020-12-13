
//
#include "ani.h"
#include <SPI.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson.git
#include <NTPClient.h>           //https://github.com/taranais/NTPClient

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

#define TFT_GREY 0x5AEB
#define lightblue 0x01E9
#define darkred 0xA041
#define blue 0x5D9B
#include "Orbitron_Medium_20.h"
#include <WiFi.h>

#include <WiFiUdp.h>
#include <HTTPClient.h>

const int pwmFreq = 5000;
const int pwmResolution = 8;
const int pwmLedChannelTFT = 0;


const char* ssid     = "IGKx20";       ///EDIIIT
const char* password = "1804672019"; //EDI8IT
String town="Paris";              //EDDIT
String Country="FR";                //EDDIT
const String endpoint = "http://api.openweathermap.org/data/2.5/weather?q="+town+","+Country+"&units=metric&APPID=";
const String key = "d0d0bf1bb7xxxx2e5dce67c95f4fd0800"; /*EDDITTTTTTTTTTTTTTTTTTTTTTTT                      */

String payload=""; //whole json 
 String tmp="" ; //temperatur
  String hum="" ; //humidity
  

StaticJsonDocument<1000> doc;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

int backlight[5] = {10,30,60,120,220};
byte b=1;

void setup(void) {
   pinMode(0,INPUT_PULLUP);
   pinMode(35,INPUT);
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);  tft.setTextSize(1);

  ledcSetup(pwmLedChannelTFT, pwmFreq, pwmResolution);
  ledcAttachPin(TFT_BL, pwmLedChannelTFT);
  ledcWrite(pwmLedChannelTFT, backlight[b]);

  Serial.begin(115200);
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
  tft.setTextColor(TFT_WHITE,TFT_BLACK);  tft.setTextSize(1);
    tft.fillScreen(TFT_BLACK);
    tft.setSwapBytes(true);
  

        
          tft.setCursor(2, 232, 1);
          tft.println(WiFi.localIP());
           tft.setCursor(80, 204, 1);
           tft.println("BRIGHT:");

           

          
          tft.setCursor(80, 152, 2);
          tft.println("SEC:");
          tft.setTextColor(TFT_WHITE,lightblue);
           tft.setCursor(4, 152, 2);
          tft.println("TEMP:");

          tft.setCursor(4, 192, 2);
          tft.println("HUM: ");
          tft.setTextColor(TFT_WHITE,TFT_BLACK);

            tft.setFreeFont(&Orbitron_Medium_20);
            tft.setCursor(6, 82);
           tft.println(town);

           tft.fillRect(68,152,1,74,TFT_GREY);

           for(int i=0;i<b+1;i++)
           tft.fillRect(78+(i*7),216,3,10,blue);

// Initialize a NTPClient to get time
  timeClient.begin(); 
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(3600);   /*EDDITTTTTTTTTTTTTTTTTTTTTTTT                      */
  getData();
  delay(500);
}
int i=0;
String tt="";
int count=0;
bool inv=1;
int press1=0; 
int press2=0;////

int frame=0;
String curSeconds="";

void loop() {

  
  

  tft.pushImage(0, 88,  135, 65, ani[frame]);
   frame++;
   if(frame>=10)
   frame=0;
  
 

   if(digitalRead(35)==0){
   if(press2==0)
   {press2=1;
   tft.fillRect(78,216,44,12,TFT_BLACK);
 
   b++;
   if(b>=5)
   b=0;

   for(int i=0;i<b+1;i++)
   tft.fillRect(78+(i*7),216,3,10,blue);
   ledcWrite(pwmLedChannelTFT, backlight[b]);}
   }else press2=0;

   if(digitalRead(0)==0){
   if(press1==0)
   {press1=1;
   inv=!inv;
   tft.invertDisplay(inv);}
   }else press1=0;
   
   if(count==0)
   getData();
   count++;
   if(count>2000)
   count=0;
   
    tft.setFreeFont(&Orbitron_Medium_20);
    tft.setCursor(2, 187);
         tft.println(tmp.substring(0,3));

         tft.setCursor(2, 227);
         tft.println(hum+"%");

           tft.setTextColor(TFT_ORANGE,TFT_BLACK);
           tft.setTextFont(2);
           tft.setCursor(6, 44);
           tft.println(dayStamp);
           tft.setTextColor(TFT_WHITE,TFT_BLACK);

          while(!timeClient.update()) {
          timeClient.forceUpdate();
  }
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  formattedDate = timeClient.getFormattedDate();
  Serial.println(formattedDate);

 
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
 
 
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
         
         if(curSeconds!=timeStamp.substring(6,8)){
         tft.fillRect(78,170,48,28,darkred);
         tft.setFreeFont(&Orbitron_Light_24);
         tft.setCursor(81, 192);
         tft.println(timeStamp.substring(6,8));
         curSeconds=timeStamp.substring(6,8);
         }

         tft.setFreeFont(&Orbitron_Light_32);
         String current=timeStamp.substring(0,5);
         if(current!=tt)
         {
          tft.fillRect(3,8,120,30,TFT_BLACK);
          tft.setCursor(5, 34);
          tft.println(timeStamp.substring(0,5));
          tt=timeStamp.substring(0,5);
         }
  
  delay(80);
}


void getData()
{
    tft.fillRect(1,170,64,20,TFT_BLACK);
    tft.fillRect(1,210,64,20,TFT_BLACK);
   if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status
 
    HTTPClient http;
 
    http.begin(endpoint + key); //Specify the URL
    int httpCode = http.GET();  //Make the request
 
    if (httpCode > 0) { //Check for the returning code
 
         payload = http.getString();
       // Serial.println(httpCode);
        Serial.println(payload);
        
      }
 
    else {
      Serial.println("Error on HTTP request");
    }
 
    http.end(); //Free the resources
  }
 char inp[1000];
 payload.toCharArray(inp,1000);
 deserializeJson(doc,inp);
  
  String tmp2 = doc["main"]["temp"];
  String hum2 = doc["main"]["humidity"];
  String town2 = doc["name"];
  tmp=tmp2;
  hum=hum2;
  
   Serial.println("Temperature"+String(tmp));
   Serial.println("Humidity"+hum);
   Serial.println(town);
   
 }
         

     

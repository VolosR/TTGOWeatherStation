#include "ani.h"
#include <SPI.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson.git
#include <NTPClient.h>           //https://github.com/taranais/NTPClient
#include "SPIFFS.h" 
#include <TJpg_Decoder.h>
#include <SimpleMap.h>  

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

#define ICON_WIDTH 40
#define ICON_HEIGHT 40
#define ICON_POS_X (tft.width() - ICON_WIDTH)

const char* ssid     = "XXXXX";       ///EDIIIT
const char* password = "XXXXX"; //EDI8IT
String town="Dnipro";              //EDDIT
String Country="UA";                //EDDIT
const String endpoint = "http://api.openweathermap.org/data/2.5/weather?q="+town+","+Country+"&units=metric&APPID=";
const String key = "29772fe7718efgh5dfga8f744d0cce9"; /*EDDITTTTTTTTTTTTTTTTTTTTTTTT                      */

String payload=""; //whole json 
 String tmp="" ; //temperatur
  String hum="" ; //humidity
  String pressure="" ; //humidity
  String weather="";
  

StaticJsonDocument<1000> doc;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

int backlight[5] = {10,30,60,120,220};
byte b=4;

SimpleMap<String, String>* myWeatherMap;

void SPIFFSInit(){
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS initialisation failed!");
    while (1) yield(); // Stay here twiddling thumbs waiting
  }
  Serial.println("\r\nInitialisation done.");
}

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
  if ( y >= tft.height() ) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

void setup(void) {
   pinMode(0,INPUT_PULLUP);
   pinMode(35,INPUT);
   
  SPIFFSInit();
  
  tft.init();

  myWeatherMap = new SimpleMap<String, String>([](String& a, String& b) -> int {
        if (a == b) return 0;

        if (a > b) return 1;

        /*if (a < b) */ return -1;
    });

  myWeatherMap->put("clear sky", "/01d.jpg");
  myWeatherMap->put("few clouds", "/02d.jpg");
  myWeatherMap->put("scattered clouds", "/03d.jpg");
  myWeatherMap->put("broken clouds", "/04d.jpg");
  myWeatherMap->put("shower rain", "/09d.jpg");
  myWeatherMap->put("rain", "/10d.jpg");
  myWeatherMap->put("thunderstorm", "/11d.jpg");
  myWeatherMap->put("snow", "/13d.jpg");
  myWeatherMap->put("mist", "/50d.jpg");
  
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
  

      
           tft.setCursor(80, 194, 1);
           tft.println("BRIGHT:");

           

          
          tft.setCursor(80, 142, 2);
          tft.println("SEC:");
          tft.setTextColor(TFT_WHITE,lightblue);
           tft.setCursor(4, 142, 2);
          tft.println("TEMP:");

          tft.setCursor(4, 182, 2);
          tft.println("HUM: ");


          tft.setCursor(4, 223, 2);
          tft.println("PRESSURE: ");
          
          tft.setTextColor(TFT_WHITE,TFT_BLACK);

            tft.setFreeFont(&Orbitron_Medium_20);
            tft.setCursor(6, 65);
           tft.println(town);

           tft.fillRect(68,142,1,74,TFT_GREY);

           for(int i=0;i<b+1;i++)
           tft.fillRect(85+(i*7),206,3,10,blue);

// Initialize a NTPClient to get time
  timeClient.begin(); 
  
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(3*3600);   /*EDDITTTTTTTTTTTTTTTTTTTTTTTT                      */
  getData();
  delay(500);

  TJpgDec.setJpgScale(1);
  TJpgDec.setCallback(tft_output);
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

  tft.pushImage(0, 76,  135, 65, ani[frame]);
   frame++;
   if(frame>=10)
   frame=0;
  
 

   if(digitalRead(35)==0){
   if(press2==0)
   {press2=1;
   tft.fillRect(85,206,44,12,TFT_BLACK);
 
   b++;
   if(b>=5)
   b=0;

   for(int i=0;i<b+1;i++)
   tft.fillRect(85+(i*7),206,3,10,blue);
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
    tft.setCursor(2, 177);
         tft.println(tmp.substring(0,4));

         tft.setCursor(2, 217);
         tft.println(hum+"%");

           tft.setTextColor(TFT_ORANGE,TFT_BLACK);
           tft.setTextFont(2);
           tft.setCursor(6, 30);
           tft.println(dayStamp);
           tft.setTextColor(TFT_WHITE,TFT_BLACK);

         tft.setTextFont(2);
         tft.setCursor(85, 223);
         tft.println(pressure);

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
 dayStamp.replace("-", ".");
 
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
         
         if(curSeconds!=timeStamp.substring(6,8)){
         tft.fillRect(78,160,48,28,darkred);
         tft.setFreeFont(&Orbitron_Light_24);
         tft.setCursor(81, 182);
         tft.println(timeStamp.substring(6,8));
         curSeconds=timeStamp.substring(6,8);
         }

         tft.setFreeFont(&Orbitron_Light_32);
         String current=timeStamp.substring(0,5);
         if(current!=tt)
         {
          tft.fillRect(3,0,120,30,TFT_BLACK);
          tft.setCursor(5, 26);
          tft.println(timeStamp.substring(0,5));
          tt=timeStamp.substring(0,5);
         }

         
  TJpgDec.drawFsJpg(ICON_POS_X, 35, myWeatherMap->get(weather));
  
  delay(80);
}


void getData()
{
    tft.fillRect(1,160,64,20,TFT_BLACK);
    tft.fillRect(1,200,64,20,TFT_BLACK);
    tft.fillRect(85, 223,64,20,TFT_BLACK);
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
  String pressure2 = doc["main"]["pressure"];
  String town2 = doc["name"];
  String weather2 = doc["weather"][0]["description"];
  tmp=tmp2;
  hum=hum2;
  weather=weather2;
  pressure=pressure2;
  
   Serial.println("Temperature"+String(tmp));
   Serial.println("Humidity"+hum);
   Serial.println("Pressure"+pressure);
   Serial.println(town);
   
 }

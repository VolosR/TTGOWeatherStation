#include <TFT_eSPI.h>        // Hardware-specific: user must select board in library
#include <ArduinoJson.h>     // https://github.com/bblanchon/ArduinoJson.git
#include <ESP32Time.h>       // replace #include <NTPClient.h>  // https://github.com/taranais/NTPClient
#include <WiFi.h>
#include <HTTPClient.h>

#include "animation.h"
#include "icons.h"
#include "Orbitron_Bold_14.h"
#include "Orbitron_Medium_20.h"

#define TFT_GREY 0x5AEB
#define TFT_LIGHTBLUE 0x01E9
#define TFT_DARKRED 0xA041
#define TFT_BLUE 0x5D9B
#define PIN_BUTTON1 0
#define PIN_BUTTON2 35

typedef struct {
	const char* town;
	const char* country;
} Towns;

// User data - EDIT
const char* ssid = "IGKx20";
const char* password = "1804672019";
const String key = "d0d0bf1bb7xxxx2e5dce67c95f4fd0800";

// Select your towns
const Towns towns[] PROGMEM = {
    {"Ribeirão Preto", "BR"},
    {"São Paulo", "BR"},
    {"Paris", "FR"},
    {"Madrid", "ES"},
    {"New York", "US"},
    {"Brasília", "BR"},
    {"Cuiabá", "BR"},
    {"Manaus", "BR"},
    {"Camaçari", "BR"},
    {"Tokyo", "JP"},
    {"Miami", "US"},
    {"Malibu", "US"},
    {"Anchorage", "US"},
    {"Vancouver", "CA"},
    {"Ottawa", "CA"},
    {"Zürich", "CH"},
    {"London", "GB"},
    {"Greenland", "GL"},
};

// Initialization
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);
ESP32Time rtc(0);  // GMT by default

const int pwmFreq = 5000;
const int pwmResolution = 8;
const int pwmLedChannelTFT = 0;
const int backlight[5] = {10, 30, 60, 120, 220};
const String endpoint = "http://api.openweathermap.org/data/2.5/weather?units=metric&APPID=" + key + "&q=";

// Variables for loop operation
String curDate = "";
String curTime = "";
String curSeconds = "";
String curTemperature = "";
String curHumidity = "";
byte curBright = 1;
int curTimezone = 0;
int curTown = 0;
int loopCount = 0;
int press1 = 0;
int press2 = 0;
int frame = 0;
int x_spr = 0;
int dir_spr = 1;
String ipinfo_io;
char* footer;
char* footer_pos;
char* footer_end;
char  footer_30;

void setup(void) {
    Serial.begin(115200);
    pinMode(0,INPUT_PULLUP);
    pinMode(35,INPUT);

    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);

    spr.setColorDepth(16);
    spr.createSprite(50, 50);
    spr.setSwapBytes(true);

    ledcSetup(pwmLedChannelTFT, pwmFreq, pwmResolution);
    ledcAttachPin(TFT_BL, pwmLedChannelTFT);
    ledcWrite(pwmLedChannelTFT, backlight[curBright]);

    // Wi-Fi connection

    tft.println(String("* ") + ssid + " *\n");
    tft.print("Connecting");
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      tft.print(".");
    }

    tft.println("\n\nWiFi connected!");
    tft.println("\nIP address:");
    tft.println(WiFi.localIP());
    delay(3000);

    // Layout

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.fillScreen(TFT_BLACK);
    tft.setSwapBytes(true);

    tft.setCursor(2, 232, 1);
    tft.println(WiFi.localIP());

    tft.setCursor(95, 204, 1);
    tft.println("BRIGHT");

    tft.setCursor(95, 152, 2);
    tft.println("SEC");

    tft.setTextColor(TFT_WHITE, TFT_LIGHTBLUE);

    tft.setCursor(4, 152, 2);
    tft.println("TEMP         ");

    tft.setCursor(4, 192, 2);
    tft.println("HUM          ");

    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.fillRect(90, 152, 1, 74, TFT_GREY);

    tft.setCursor(6, 82);
    tft.setFreeFont(&Orbitron_Medium_20);
    if (tft.textWidth(towns[curTown].town) > tft.width() - 12)
        tft.setFreeFont(&Orbitron_Bold_14);
    tft.println(towns[curTown].town);

    for(int i = 0; i <= curBright; i++)
        tft.fillRect(93 + (i * 7), 216, 3, 10, TFT_BLUE);

    getLocalInfo();
    getData();
}

void loop() {
    tft.pushImage(0, 88,  135, 65, animation[frame++]);
    if (frame == frames) frame = 0;

    spr.pushSprite(x_spr, 95, 0x94b2);
    x_spr += dir_spr;
    if (x_spr == 85) dir_spr = -1;
    else if (x_spr == 0) dir_spr = 1;

    if (digitalRead(PIN_BUTTON2) == 0){
        if (press2 == 0) {
            press2 = 1;
            tft.fillRect(93, 216, 44, 12, TFT_BLACK);
            if(++curBright == 5) curBright = 0;
            for(int i = 0; i <= curBright; i++)
                tft.fillRect(93 + (i * 7), 216, 3, 10, TFT_BLUE);
            ledcWrite(pwmLedChannelTFT, backlight[curBright]);
        }
    }
    else
        press2 = 0;

    if (digitalRead(PIN_BUTTON1) == 0) {
        if (press1 == 0) {
            press1 = 1;
            if (++curTown == sizeof(towns) / sizeof(Towns))
                curTown = 0;
            tft.fillRect(0, 60, 135, 27, TFT_BLACK);
            tft.setCursor(6, 82);
            tft.setFreeFont(&Orbitron_Medium_20);
            if (tft.textWidth(towns[curTown].town) > tft.width() - 12)
                tft.setFreeFont(&Orbitron_Bold_14);
            tft.println(towns[curTown].town);
            loopCount = 3000;
        }
    }
    else
        press1 = 0;

    if (++loopCount > 3000) {  /// about 5 minutes
        while (!getData())
            delay(100);
        loopCount = 0;
    }

    tft.setCursor(2, 232, 1);
    if (++footer_pos == footer_end)
        footer_pos = footer;
    footer_30 = *(footer_pos + 30);
    *(footer_pos + 30) = '\0';
    tft.println(footer_pos);
    Serial.println(footer_pos);
    *(footer_pos + 30) = footer_30;

    String _date = rtc.getTime("%a, %d %b %Y ");
    String _time = rtc.getTime("%R");
    String _seconds = rtc.getTime("%S");

    if (curDate != _date) {
        curDate = _date;
        tft.fillRect(0, 44, 135, 18, TFT_BLACK);
        tft.setTextColor(TFT_ORANGE, TFT_BLACK);
        tft.setTextFont(2);
        tft.setCursor(6, 44);
        tft.println(curDate);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
    }

    if (curTime != _time) {
        curTime = _time;
        tft.setFreeFont(&Orbitron_Light_32);
        tft.fillRect(0, 0, 135, 40, TFT_BLACK);
        tft.setCursor(5, 40);
        tft.print(curTime);
        tft.setTextColor(TFT_ORANGE, TFT_BLACK);
        tft.setCursor(110, 0);
        tft.setTextFont(2);
        int _timezone = int(curTimezone / 3600);
        tft.print(_timezone < 0 ? (_timezone > -10 ? "-0" : "-") : (_timezone < 10 ? "+0" : "+"));
        tft.println(abs(_timezone));
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
    }

    if (curSeconds != _seconds) {
        curSeconds = _seconds;
        tft.fillRect(93, 170, 48, 28, TFT_DARKRED);
        tft.setFreeFont(&Orbitron_Medium_20);
        tft.setCursor(96, 192);
        tft.println(curSeconds);
    }

    delay(200);
}

bool getData() {
    String icon = "";
    if (WiFi.status() == WL_CONNECTED) {  // Check the current connection status
        HTTPClient http;
        int httpCode;

        http.begin(endpoint + towns[curTown].town + "," + towns[curTown].country);
        const char *headerKeys[] = {"Date"};
        const size_t headerKeysCount = sizeof(headerKeys) / sizeof(headerKeys[0]);
        http.collectHeaders(headerKeys, headerKeysCount);
        httpCode = http.GET();  // Make the request

        if (httpCode == 200) {  // Check for the returning code
            String payload = http.getString();
            StaticJsonDocument<1000> doc;
            deserializeJson(doc, payload.c_str());

            String _lon = doc["coord"]["lon"];
            String _lat = doc["coord"]["lat"];
            String _description = doc["weather"][0]["description"];
            String _icon = doc["weather"][0]["icon"];
            String _temp = doc["main"]["temp"];
            String _feels_like = doc["main"]["feels_like"];
            String _temp_min = doc["main"]["temp_min"];
            String _temp_max = doc["main"]["temp_max"];
            String _pressure = doc["main"]["pressure"];
            String _humidity = doc["main"]["humidity"];
            String _visibility = doc["visibility"];
            String _wind_speed = doc["wind"]["speed"];
            String _wind_degree = doc["wind"]["deg"];
            String _clouds = doc["clouds"]["all"];
            String _dt_capture = doc["dt"];
            String _sunrise = doc["sys"]["sunrise"];
            String _sunset = doc["sys"]["sunset"];
            String _timezone = doc["timezone"];
            String _name = doc["name"];

            curTimezone = atoi(_timezone.c_str());
            setRtcTime(http.header("Date"));

            icon = _icon;

            if (curTemperature != _temp) {
                curTemperature = _temp;
                tft.setFreeFont(&Orbitron_Medium_20);
                tft.fillRect(0, 173, 89, 14, TFT_BLACK);
                tft.setCursor(2, 187);
                _temp = curTemperature.substring(0, 4);
                if (_temp.length() == 4 and _temp[3] == '.') {
                    if (_temp[0] == '-' and _temp[1] == '1')
                        _temp = curTemperature.substring(0, 5);
                    else
                        _temp = curTemperature.substring(0, 3);
                }
                tft.println(_temp + "°C");

                spr.pushImage(0, 0, 50, 50, icons[icons_map(_icon)]);
            }

            if (curHumidity != _humidity) {
                curHumidity = _humidity;
                tft.setFreeFont(&Orbitron_Medium_20);
                tft.fillRect(0, 213, 89, 14, TFT_BLACK);
                tft.setCursor(2, 227);
                tft.println(curHumidity + "%");
            }

            char buffer[6];
            time_t _time;

            _name = "   " + _name;

            _time = atoi(_dt_capture.c_str()) + curTimezone;
            strftime(buffer, 6, "%H:%M", gmtime(&_time));
            _dt_capture = buffer;

            _time = atoi(_sunrise.c_str()) + curTimezone;
            strftime(buffer, 6, "%H:%M", gmtime(&_time));
            _sunrise = buffer;

            _time = atoi(_sunset.c_str()) + curTimezone;
            strftime(buffer, 6, "%H:%M", gmtime(&_time));
            _sunset = buffer;

            updateFooter(_name + " weather: " + _description +
                         _name + " feels like: " + _feels_like + "°C" +
                         _name + " pressure: " + _pressure + "hPa" +
                         _name + " location: " + _lon + "," + _lat +
                         _name + " visibility: " + (atoi(_visibility.c_str()) / 1000.0) + "km" +
                         _name + " wind speed: " + _wind_speed + "m/s" +
                         _name + " wind degree: " + _wind_degree + "°" +
                         _name + " clouds: " + _clouds + "%" +
                         _name + " capture at: " + _dt_capture +
                         _name + " sunrise: " + _sunrise +
                         _name + " sunset: " + _sunset +
                         "   Weather from https://openweathermap.org/");

        }
        else {
            Serial.println("Error on HTTP request [" + String(httpCode) + "]");
            return false;
        }

        http.end(); //Free the resources
    }
    else {
        Serial.println("No internet connection!");
        return false;
    }

    Serial.println("_____________________________________________________________");
    Serial.println("Town: [" + String(curTown) + "] " + towns[curTown].town + ", " + towns[curTown].country);
    Serial.println("Date and time: " + rtc.getDateTime());
    Serial.println("Temperature: " + curTemperature);
    Serial.println("Humidity: " + curHumidity);
    Serial.println("Icon: " + icon);
    return true;
}

int numberOfMonth(const char* month) {
    String m3 = String((char) tolower(*month)) + (char) tolower(month[1]) + (char) tolower(month[2]);
    if (m3 == "jan") return 1;
    if (m3 == "fer") return 2;
    if (m3 == "mar") return 3;
    if (m3 == "apr") return 4;
    if (m3 == "may") return 5;
    if (m3 == "jun") return 6;
    if (m3 == "jul") return 7;
    if (m3 == "aug") return 8;
    if (m3 == "sep") return 9;
    if (m3 == "oct") return 10;
    if (m3 == "nov") return 11;
    if (m3 == "dec") return 12;
    return 0;
}

void setRtcTime(String date_string){
    int day, month, year, hours, minutes, seconds;
    const char* c = date_string.c_str();
    while (*c++ != ' '); c--; while (*c++ == ' '); c--; day = atoi(c);
    while (*c++ != ' '); c--; while (*c++ == ' '); c--; month = numberOfMonth(c);
    while (*c++ != ' '); c--; while (*c++ == ' '); c--; year = atoi(c);
    while (*c++ != ' '); c--; while (*c++ == ' '); c--; hours = atoi(c);
    while (*c++ != ':'); minutes = atoi(c);
    while (*c++ != ':'); seconds = atoi(c);
    rtc.setTime(seconds, minutes, hours, day, month, year);
    rtc.setTime(rtc.getEpoch() + curTimezone);
}

bool getLocalInfo() {
    String icon = "";
    if (WiFi.status() == WL_CONNECTED) {  // Check the current connection status
        HTTPClient http;
        int httpCode;

        http.begin("http://ipinfo.io/");
        http.addHeader("Accept", "application/json");
        httpCode = http.GET();  // Make the request

        if (httpCode == 200) {  // Check for the returning code
            String payload = http.getString();
            StaticJsonDocument<1000> doc;
            deserializeJson(doc, payload.c_str());

            String _ip = doc["ip"];
            String _city = doc["city"];
            String _region = doc["region"];
            String _country = doc["country"];
            String _loc = doc["loc"];
            String _org = doc["org"];
            String _postal = doc["postal"];
            String _tz = doc["timezone"];

            ipinfo_io = "   Wi-Fi IP: " + WiFi.localIP().toString() +
                        "   Internet IP: " + _ip +
                        "   Local city: " + _city +
                        "   Local state: " + _region +
                        "   Local country: " + _country +
                        "   Local position: " + _loc +
                        "   Local Postal: " + _postal +
                        "   Local Timezone: " + _tz +
                        "   Local from http://ipinfo.io/   ";
        }
        else {
            Serial.println("Error on HTTP request [" + String(httpCode) + "]");
            return false;
        }

        http.end(); //Free the resources
    }
    else {
        Serial.println("No internet connection!");
        return false;
    }
    return true;
}

void updateFooter(String extraInfo) {
    String _footer = extraInfo + ipinfo_io;
    int _length = _footer.length();
    _footer += _footer.substring(0, 30);
    footer = (char*) malloc(_footer.length() + 1);
    strcpy(footer, _footer.c_str());
    footer_end = footer + _length ;
    footer_pos = footer;
}

#include <TFT_eSPI.h>        // Hardware-specific: user must select board in library
#include <ESP32Time.h>       // replace #include <NTPClient.h>  // https://github.com/taranais/NTPClient
#include <HTTPClient.h>
#include <ArduinoJson.h>     // https://github.com/bblanchon/ArduinoJson.git
#include <ESPAsyncWebServer.h>  // https://github.com/JuniorPolegato/ESPAsyncWebServer

#include "ESPUserConnection.h"
#include "fs_operations.h"
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

#define USE_STRPTIME

typedef struct {
    String city;
    String country;
} Cities;

Cities *cities = NULL;
size_t qtd_cities = 0;

// Initialization

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);
ESP32Time rtc(0);  // GMT by default

const int pwmFreq = 5000;
const int pwmResolution = 8;
const int pwmLedChannelTFT = 0;
const int backlight[5] = {10, 30, 60, 120, 220};

String endpoint;

// Variables for loop operation
String curDate = "";
String curTime = "";
String curSeconds = "";
String curTemperature = "";
String curHumidity = "";
byte curBright = 1;
int curTimezone = 0;
int curCity = 0;
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
bool ap_mode = false;

void load_cities() {
    int i, d;

    String file_data = readFile("/cities.txt");
    if (file_data.length() < 6)
        file_data = "Ribeirão Preto\tBR\n";
    else if (!file_data.endsWith("\n"))
        file_data += '\n';

    qtd_cities = 0;
    i = 0;
    while ((i = file_data.indexOf('\n', ++i)) != -1)
        qtd_cities++;

    if (cities)
        free(cities);
    cities = (Cities*)calloc(qtd_cities, sizeof(Cities));

    Cities *c = cities;
    i = 0;
    for (;;) {
        d = file_data.indexOf('\t', i);
        if (d == -1) break;
        c->city = file_data.substring(i, d++);
        i = file_data.indexOf('\n', d);
        if (i == -1) break;
        c->country = file_data.substring(i++, d);
        Serial.println(c->city + " - " + c->country);
        c++;
    }
    curCity = 0;
}

void update_city() {
    tft.fillRect(0, 60, 135, 27, TFT_BLACK);
    tft.setCursor(6, 82);
    tft.setFreeFont(&Orbitron_Medium_20);
    if (tft.textWidth(cities[curCity].city) > tft.width() - 12)
        tft.setFreeFont(&Orbitron_Bold_14);
    tft.println(cities[curCity].city);
    loopCount = 3000;  // force to getData for current city
}

// Customizations from ESPUserConnection.cpp
extern AsyncWebServer webserver;

extern void user_request_data(AsyncWebServerRequest *request, bool restart=true);

void custom_user_request_data(AsyncWebServerRequest *request) {
    int params = request->params();

    if (params >= 3) {
        String key = request->getParam(2)->value();

        if (key.length() == 32)
            writeFile("/key.txt", key, true);

        else if (key == "delete")
            deleteFile("/key.txt");

        user_request_data(request);
    }
    else
        request->send(500);
}

void append_to_webserver() {
    webserver.on("/list_cities", HTTP_GET, [](AsyncWebServerRequest *request) {
        int i = 0, d;
        String city, country;

        String file_data = readFile("/cities.txt");

        String resp = "[{\"city\":\"Click here to add\",\"country\":\"BR\"}";
        for (;;) {
            d = file_data.indexOf('\t', i);
            if (d == -1) break;
            city = file_data.substring(i, d++);
            i = file_data.indexOf('\n', d);
            if (i == -1) break;
            country = file_data.substring(d, i++);
            resp += ",{\"city\":\"" + city + "\","
                    "\"country\":\"" + country + "\"}";
        }

        request->send(200, "application/json", resp + ']');
    });

    webserver.on("/config_cities", HTTP_POST, [](AsyncWebServerRequest *request) {
        int params = request->params();

        if (params == 3) {
            String city = request->getParam(0)->value();
            String country = request->getParam(1)->value();
            String operation = request->getParam(2)->value();
            int i = -1, n;

            if (operation != "add" && operation != "delete") {
                request->send(500);
                return;
            }

            String file_data = readFile("/cities.txt");
            renameFile("/cities.txt", "/cities.txt.bak");

            String line = city + '\t' + country + '\n';
            i = file_data.indexOf(line);
            if (i > -1)  // delete the line
                file_data = file_data.substring(0, i) + file_data.substring(i + line.length());

            if (operation == "add")
                file_data = line + file_data;

            if (writeFile("/cities.txt", file_data, true))
                deleteFile("/cities.txt.bak");
            else
                renameFile("/cities.txt.bak", "/cities.txt");

            request->send(200, "text/html", go_back_html);

            load_cities();
            update_city();
        }
        else
            request->send(500);
    });
}

void setup(void) {
    Serial.begin(115200);
    pinMode(0,INPUT_PULLUP);
    pinMode(35,INPUT);

    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(1);
    tft.setTextSize(1);

    spr.setColorDepth(16);
    spr.createSprite(50, 50);
    spr.setSwapBytes(true);

    ledcSetup(pwmLedChannelTFT, pwmFreq, pwmResolution);
    ledcAttachPin(TFT_BL, pwmLedChannelTFT);
    ledcWrite(pwmLedChannelTFT, backlight[curBright]);

    // Wi-Fi connection

#ifdef OUTPUT_IS_TFT
    if (!connect_wifi(&tft, ap_mode)){
#else
    if (!connect_wifi(ap_mode)){
#endif  // OUTPUT_IS_TFT
        ap_mode = true;
        return;
    }

    append_to_webserver();

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

    load_cities();
    if (tft.textWidth(cities[curCity].city) > tft.width() - 12)
        tft.setFreeFont(&Orbitron_Bold_14);
    tft.println(cities[curCity].city);

    for(int i = 0; i <= curBright; i++)
        tft.fillRect(93 + (i * 7), 216, 3, 10, TFT_BLUE);

    String key = readFile("/key.txt").substring(0, 32);
    // Serial.println("[" + key + "]");
    endpoint = "http://api.openweathermap.org/data/2.5/weather?units=metric&APPID=" + key + "&q=";

    String api_error;
    for (;;) {
        api_error.clear();

        if (!getLocalInfo())
            api_error = "IP Info";

        if (!getData())
            if (api_error.length())
                api_error += "\nOpen Weather";
            else
                api_error = "Open Weather";

        if (!api_error.length()) break;

        key.clear();
        tft.fillRect(0, 0, tft.getViewportWidth(), tft.getViewportHeight(), TFT_DARKRED);
        tft.setFreeFont(&Orbitron_Bold_14);
        tft.setTextColor(TFT_WHITE);
        tft.setCursor(0, 20);
        api_error = "Error!\n\n"
                    + api_error + "\n\n"
                    "Please, check:\n"
                    "- internet\n"
                    "- cities\n"
                    "- countries\n"
                    "- OpenWeather\n"
                    "                    key\n\n\n";
        tft.println(api_error);
        tft.println(WiFi.localIP());
        Serial.println(api_error);
        Serial.println(WiFi.localIP());
        tft.print("10 s"); tft.flush();
        Serial.print("10 s"); Serial.flush();
        for (int i=0; i < 10; i++) {
            tft.print('.'); tft.flush();
            Serial.print('.'); Serial.flush();
            delay(1000);
        }
    }
    if (!key.length())
        setup();
}

void loop() {
    if (ap_mode) {
        if (WiFi.softAPgetStationNum() == 0) {

#ifdef OUTPUT_IS_TFT
            tft.print('.'); tft.flush();
#else
            Serial.print('.'); Serial.flush();
#endif  // OUTPUT_IS_TFT

            delay(1000);
            return;
        }

#ifdef OUTPUT_IS_TFT
        tft.println("\nConnected!\n");
#else
        Serial.println("\nConnected!\n");
#endif  // OUTPUT_IS_TFT

        vTaskDelete(NULL);  // Stop loop task, run just WebServer in AP mode
        return;
    }

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
            if (++curCity == qtd_cities)
                curCity = 0;
            update_city();
        }
    }
    else
        press1 = 0;

    if (++loopCount > 3000) {  /// about 5 minutes
        while (!getData())
            delay(1000);
        loopCount = 0;
    }

    tft.setCursor(2, 232, 1);
    footer_pos += *footer_pos > 127;
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

        String _endpoint = endpoint + cities[curCity].city + "," + cities[curCity].country;
        // Serial.println("[" + _endpoint + "]");
        http.begin(_endpoint);
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

            String _name = String("   ") + cities[curCity].city;

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
            Serial.println("Error on\nHTTP OpenWeather\nrequest [" + String(httpCode) + "]");
            return false;
        }

        http.end(); //Free the resources
    }
    else {
        Serial.println("No internet connection!");
        return false;
    }

    Serial.println("_____________________________________________________________\n");
    Serial.println("City: [" + String(curCity) + "] " + cities[curCity].city + ", " + cities[curCity].country);
    Serial.println("Date and time: " + rtc.getDateTime());
    Serial.println("Temperature: " + curTemperature);
    Serial.println("Humidity: " + curHumidity);
    Serial.println("Icon: " + icon);
    Serial.println("_____________________________________________________________\n");
    return true;
}

#ifndef USE_STRPTIME
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
#endif // !USE_STRPTIME

void setRtcTime(String date_string){
#ifdef USE_STRPTIME
    struct tm t;
    strptime(date_string.c_str(), "%a, %d %b %Y %H:%M:%S GMT", &t);
    rtc.setTime(mktime(&t) + curTimezone);
#else
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
#endif // USE_STRPTIME
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

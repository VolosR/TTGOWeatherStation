#include <ESPAsyncWebServer.h>  // https://github.com/JuniorPolegato/ESPAsyncWebServer

#include "ESPUserConnection.h"
#include "fs_operations.h"

#ifdef IP_ON_BLUETOOTH_NAME
    #include <BluetoothSerial.h>
    BluetoothSerial SerialBT;
#endif

AsyncWebServer webserver(80);
bool webserver_configured = false;

// This is an example to request also a key to the user via html page
#ifdef CUSTOM_USER_REQUEST_DATA
extern void custom_user_request_data(AsyncWebServerRequest *request);
#endif  // CUSTOM_USER_REQUEST_DATA

void user_request_data(AsyncWebServerRequest *request, bool restart=true) {
    int params = request->params();

    if (params >= 2) {
        String ssid = request->getParam(0)->value();
        String passwd = request->getParam(1)->value();
        int i = -1, n;

        if (passwd.length() > 0) {
            String file_data = readFile("/known_wifis.txt");
            renameFile("/known_wifis.txt", "/known_wifis.txt.bak");

            if (file_data.startsWith(ssid + '\t') || (i = file_data.indexOf('\n' + ssid + '\t')) > 0) {
                n = file_data.indexOf('\n', ++i);
                file_data = (passwd == "delete" ? String() : ssid + '\t' + passwd + '\n') +
                            file_data.substring(0, i) +
                            (n == -1 ? String() : file_data.substring(n + 1));
            }
            else if (passwd != "delete")
                file_data = ssid + '\t' + passwd + '\n' + file_data;

            if (writeFile("/known_wifis.txt", file_data, true))
                deleteFile("/known_wifis.txt.bak");
            else
                renameFile("/known_wifis.txt.bak", "/known_wifis.txt");
        }

        request->send(200, "text/html", go_back_html);

        if (restart) ESP.restart();
    }
    else
        request->send(500);
}

// Settings for your TFT device and/or Serial
#define PRINT if (WiFi.status() != WL_CONNECTED) output->print
#define PRINTLN if (WiFi.status() != WL_CONNECTED) output->println
#ifdef OUTPUT_IS_TFT
    #include <TFT_eSPI.h>
    TFT_eSPI *output;
#else
    #define output (&Serial)
    // Configure how to clear screen on your serial terminal
    // This line clear screen for https://github.com/JuniorPolegato/gtkterm
    #define SERIAL_CLEAR_SCREEN_COMMAND PRINT('\x1b')
#endif  // OUTPUT_IS_TFT

void clear(){
    if (WiFi.status() != WL_CONNECTED) {
#ifdef OUTPUT_IS_TFT
        output->fillRect(0, 0, output->getViewportWidth(), output->getViewportHeight(), TFT_BACKGROUND);
        output->setTextColor(TFT_TEXT_COLOR);
        output->setCursor(0, 0);
#else
    SERIAL_CLEAR_SCREEN_COMMAND;
#endif  // OUTPUT_IS_TFT
        PRINTLN(PROJECT_NAME);
        PRINTLN();
    }
}

String scan(){
    String resp;
    String ssid;
    int scan_status = WiFi.scanComplete();

    clear();
    PRINTLN("Searching Wi-Fi...\n");

    if (scan_status == WIFI_SCAN_FAILED) {
        PRINTLN("Starting...\n");
        WiFi.scanNetworks(true);
        delay(1000);
    }

    while ((scan_status = WiFi.scanComplete()) == WIFI_SCAN_RUNNING) {
        PRINT('.'); output->flush();
        delay(100);
    }
    PRINTLN();

    if (scan_status == WIFI_SCAN_FAILED) {
        PRINTLN("Wait a moment...");
        return "[{\"name\":\"Trying again...\","
                 "\"quality\":\"Wait a moment...\","
                 "\"security\":\"Wait a moment...\"}]";
    }

    PRINTLN("Networks found: " + String(scan_status));
    for (int  i = 0; i < scan_status; i++) {
        resp += ",{"
                "\"name\":\"" + WiFi.SSID(i) + "\","
                "\"quality\":" + WiFi.RSSI(i) + ","
                "\"security\":" + WiFi.encryptionType(i) + "}";
        if (i < 20) PRINTLN(WiFi.SSID(i) + '[' + WiFi.RSSI(i) + ']');
    }

    WiFi.scanDelete();

    return '[' + resp.substring(1) + ']';
}

void upload_handler(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    PRINTLN(filename + '[' + len + ']');

    if (!index) {
        String filepath = filename;

        if (!filename.startsWith("/"))
            filepath = "/" + filename;

        request->_tempFile = LittleFS.open(filepath, FILE_WRITE);
    }

    if (len)
        request->_tempFile.write(data, len);

    if (final) {
        request->_tempFile.close();
        request->send(200, "text/html", go_back_html);
    }
}

void start_AP() {
    clear();
    PRINTLN("Connect your\n"
                    "Wi-Fi device to:\n\n"
                    + String(PROJECT_NAME) + "\n\n"
                    "Password: 12345678\n");

    const char *ssid = PROJECT_NAME;
    const char *passwd = "12345678";
    WiFi.softAP(ssid, passwd);
    IPAddress ip = WiFi.softAPIP();
    delay(1000);

    webserver.begin();
    PRINTLN("Then access:\n\n"
                  "http://" + ip.toString() + "/wifi\n\n"
                  "and select Wi-Fi\n"
                  "network for your\n"
                  + String(PROJECT_NAME) + "\n\n"
                  "Waiting for you...\n");

#ifdef IP_ON_BLUETOOTH_NAME
    SerialBT.end();
    SerialBT.begin("ESP32 " + ip.toString());
#endif

    delay(1000);
}

void config_webserver() {
    webserver.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (findFile("/index.html"))
            request->send(LittleFS, "/index.html", "text/html");
        else {
            PRINTLN("Send \"index.html\" file!");
            request->send(200, "text/html", sendfiles_html);
        }
    });

    webserver.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
        WiFi.scanNetworks(true);
        request->send(200, "application/json", scan());
    });

    webserver.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
#ifdef CUSTOM_USER_REQUEST_DATA
        String user_config = "/custom_user_config.html";
#else
        String user_config = "/user_config.html";
#endif  // CUSTOM_USER_REQUEST_DATA
        if (findFile(user_config)) {
            PRINTLN(
                "Now select a Wi-Fi\n"
                "network for ESP\n"
                "connect to\n");
            request->send(LittleFS, user_config, "text/html");
        }
        else {
            PRINTLN(
                "First access.\n"
                "It's needed to\n"
                "send \"" + user_config.substring(1) + "\".\n");
            request->send(200, "text/html", sendfiles_html);
        }
    });

    webserver.on("/wifi", HTTP_POST, [](AsyncWebServerRequest *request) {
#ifdef CUSTOM_USER_REQUEST_DATA
        custom_user_request_data(request);
#else
        user_request_data(request);
#endif  // CUSTOM_USER_REQUEST_DATA
    });

    webserver.on("/send_file", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", sendfiles_html);
        PRINTLN("Ready to send a file!");
    });

    webserver.on("/send_file", HTTP_POST, [](AsyncWebServerRequest *request) {
        clear();
        PRINTLN("Uploading the file...");
        request->send(200);
    }, upload_handler);

    // Here you can add your custom endpoints

    webserver_configured = true;
}

#ifdef OUTPUT_IS_TFT
bool connect_wifi(void *tft, bool force_ap_mode, bool show_connected_ip){
    output = (TFT_eSPI*)tft;
#else
bool connect_wifi(bool force_ap_mode, bool show_connected_ip){
#endif  // OUTPUT_IS_TFT

    String wifi, passwd, known_wifis;
    int i = 0, d;  // i = inital line position | d = position of delimiter \t

    if (webserver_configured)
        webserver.end();
    else
        config_webserver();

    if (force_ap_mode) {
        PRINTLN("Forcing AP Mode!\n");
        WiFi.disconnect(false, true);
        delay(1000);
        start_AP();
        return false;
    }

    known_wifis = readFile("/known_wifis.txt");
    while (WiFi.status() != WL_CONNECTED) {
        clear();

        d = known_wifis.indexOf('\t', i);
        wifi = known_wifis.substring(i, d);
        PRINTLN(wifi);

        i = d == -1 ? -1 : known_wifis.indexOf('\n', ++d);

        if (i == -1) {  // No known wi-fi found
            PRINTLN("\n\nNo known WiFi\n\nmatches in the\n\nneighborhood.\n");
            WiFi.disconnect(false, true);
            delay(1000);
            start_AP();
            return false;
        }

        passwd = known_wifis.substring(d, i++);
        // PRINTLN("  \->|" + passwd + "|");

        PRINT("Connecting...");
        WiFi.begin(wifi, passwd == "open" ? (const char *)NULL : passwd.c_str());

        d = 0;
        while (WiFi.status() != WL_CONNECTED && d++ < 20) {
            delay(500);
            PRINT("."); output->flush();
        }
    }

    if (show_connected_ip) {
        output->println("\n\nWiFi connected!");
        output->println("\nIP address:");
        output->println(WiFi.localIP());
    }

#ifdef IP_ON_BLUETOOTH_NAME
    SerialBT.end();
    SerialBT.begin("ESP32 " + WiFi.localIP().toString());
#endif

    webserver.begin();

    return true;
}

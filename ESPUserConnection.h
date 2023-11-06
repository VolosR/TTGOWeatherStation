#ifndef ESP_USER_CONNECTION_H
#define ESP_USER_CONNECTION_H

#define PROJECT_NAME "TTGO Weather Station"

// Uncomment this if you use custom user_config.html to get extra user data.
// You need to provide the function "custom_user_request_data(request)"
// into ESPUserConnection.cpp (see exemple there), also is needed to create
// custom_user_config.html based on user_config.html (see diffs between these)
// and upload this file to ESP at first boot. Create and upload index.html.
// You also need to customize sendfiles_html and go_back_html bellow.
#define CUSTOM_USER_REQUEST_DATA

// Uncomment and adjust next lines to output info to TFT
#define OUTPUT_IS_TFT
#define TFT_BACKGROUND TFT_DARKGREEN
#define TFT_TEXT_COLOR TFT_YELLOW

// Uncomment this to see the IP to connect to on Bluetooh name
// #define IP_ON_BLUETOOTH_NAME

#ifdef CUSTOM_USER_REQUEST_DATA
const char sendfiles_html[] PROGMEM = R"===(
<!DOCTYPE html>
<html lang="en-us">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>TTGO Weather Station - by Junior Polegato</title>
</head>
<body>
    <h1>TTGO Weather Station - by Junior Polegato</h1>
    <h2>Select and send files to TTGO Weather Station board<h2>
    <h3>Please, pay attention! You can destroy data and crash your Weather Station.<h3>
    <form action="/send_file" method="POST" enctype="multipart/form-data">
        <input type="file" name="file" id="file">
        <button type="submit">Send</button>
    </form>
</body>
</html>
)===";

const char go_back_html[] PROGMEM = R"===(
<!DOCTYPE html>
<html lang="en-us">
<head>
    <meta charset="UTF-8">
    <meta http-equiv="refresh" content="1; url=/">
    <title>TTGO Weather Station - by Junior Polegato</title>
</head>
<body>
    <h1>TTGO Weather Station - by Junior Polegato</h1>
</body>
</html>
)===";
#else
const char sendfiles_html[] PROGMEM = R"===(
<!DOCTYPE html>
<html lang="en-us">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP User Connection - by Junior Polegato</title>
</head>
<body>
    <h1>ESP User Connection - by Junior Polegato</h1>
    <h2>Select and send files to ESP board<h2>
    <h3>Please, pay attention! You can destroy data and crash your ESP.<h3>
    <form action="/send_file" method="POST" enctype="multipart/form-data">
        <input type="file" name="file" id="file">
        <button type="submit">Send</button>
    </form>
</body>
</html>
)===";

const char go_back_html[] PROGMEM = R"===(
<!DOCTYPE html>
<html lang="en-us">
<head>
    <meta charset="UTF-8">
    <meta http-equiv="refresh" content="1; url=/">
    <title>ESP User Connection - by Junior Polegato</title>
</head>
<body>
    <h1>ESP User Connection - by Junior Polegato</h1>
</body>
</html>
)===";
#endif  // CUSTOM_USER_REQUEST_DATA

#ifdef OUTPUT_IS_TFT
bool connect_wifi(void *tft, bool force_ap_mode=false, bool show_connected_ip=true);
#else
bool connect_wifi(bool force_ap_mode=false, bool show_connected_ip=true);
#endif  // OUTPUT_IS_TFT

#endif // ESP_USER_CONNECTION_H

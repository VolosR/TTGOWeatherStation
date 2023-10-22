# Weather Station by Junior Polegato

I got the original code for study, I organized it and many new features!

![Weather Station by Junior Polegato](https://raw.githubusercontent.com/JuniorPolegato/TTGOWeatherStation/main/assets/TTGOWeatherStation.jpg)
[REPO](https://github.com/JuniorPolegato/TTGOWeatherStation)

## How to get it on

1. Clone or download this repository to your local machine into the Arduino directory.
[REPO](https://github.com/JuniorPolegato/TTGOWeatherStation)

2. Open with Arduino IDE or VSCodium (ou VSCode) with extensions vscode-arduino extension and vscode-cpptools.

3. Install libraries TFT_eSPI from the interface tools.

4. Configure TFT_eSPI editing file User_Setup_Select.h, where you need comment “#include <User_Setup.h>” and uncomment line for your device, just one line, for my case “#include <User_Setups/Setup25_TTGO_T_Display.h>” line.

5. Clone or download ArduinoJson from [REPO](https://github.com/bblanchon/ArduinoJson.git) into your Arduino/libraries directory.

6. Get your free API Key from https://openweathermap.org/.

7. Put your Wi-Fi name and password, plus your key that you got above:

|Name|Description|
|----|-----------|
|ssid|Name of the WiFi network|
|password|WiFi password|
|key|Open Weather API key|

## Nice features

- Show local time with timezone and date
- Show city / town name with nice font and size adjust
- You can put a list of towns in code and swap its by button 1 (left)
- Weather icon with animation
- Temperature and Humidity with nice font and big size
- Brightness level control by button 2 (right)
- Wi-Fi IP address, internet IP address, local informations and extra town informations
- Font editor, see [REPO](https://github.com/JuniorPolegato/Adafruit-GFX-font-editor)
- Image converter, see [REPO](https://github.com/JuniorPolegato/image-to-rgb565)

## Work description

- Replaced the inverse display function for button 1 by changing over a list of towns

- Additionally I create a Python script to add bitmap characters into the font file (.h).
[REPO](https://github.com/JuniorPolegato/Adafruit-GFX-font-editor)

- I added a degree symbol to the font and now the temperature displays °C, so I needed to adjust the layout.

- I also added some required utf-8 symbols for towns.

- For towns with large names, like "Ribeirão Preto", I got font size 14, so it's verified if it needs to change font size from 20 to 14.

- I also created a Python script to download weather icons, render over a background, convert from 32 bits pixels (RGBA) to 16 bits (RGB565), and you can put the output into a "icons.h" file.

- With the icon code returned from OpenWeather API, the program selects the correct icon from "icons.h" and puts it into a sprite, which floats over the animation background.

- The program also gets local informations from http://ipinfo.org/ and joins weather informations from https://openweathermap.org/.

- Now you can see a footer banner, rotating banner, with extra town informations and local informations.

- Finally, to get the time, I replaced NTP by ESP32 RTC plus “Date” http header, so the program gets the unix time and timezone from the API JSON response and updates RTC with the actual town's local time and date, therefore it's not necessary to access the NTP client many times per second.

- Aboud IDE, it used VSCodium (VSCode without telemetry/tracking), with vscode-arduino extension, plus vscode-cpptools.

### Credits

Here is the original [REPO](https://github.com/VolosR/TTGOWeatherStation).

Here is the font editor [REPO](https://github.com/JuniorPolegato/Adafruit-GFX-font-editor).

Here is the image converter [REPO](https://github.com/JuniorPolegato/image-to-rgb565).

### License

This software is licensed with GPL Version 3. https://www.gnu.org/licenses/gpl-3.0.html

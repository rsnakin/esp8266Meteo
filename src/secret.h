#pragma once

    struct WifiCredential {
       const char* ssid;
       const char* pass;
    };

    #define BOT_TOKEN "YourTelegramBotToken"              // Telegram BOT Token (Get from Botfather)
    #define LATITUDE latitude_of_your_weather_station     // For example 51.500833
    #define LONGITUDE longitude_of_your_weather_station   // For example -0.124444

    #define WIFI_CREDENTIALS_INIT \
    {\
       {"YourWiFi-1","YourPassword-1"},\
       {"YourWiFi-2","YourPassword-2"},\
       ...
       {"YourWiFi-N","YourPassword-"}\
    }
    const WifiCredential WIFI_CREDENTIALS[] PROGMEM = WIFI_CREDENTIALS_INIT;
    #define WIFI_CREDENTIALS_COUNT (sizeof(WIFI_CREDENTIALS)/sizeof(WIFI_CREDENTIALS[0]))
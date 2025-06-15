/*
MIT License

Copyright (c) 2025 Richard Snakin

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <Adafruit_BMP085.h>
#include <Arduino.h>
#include <DFRobot_DHT11.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <LittleFS.h>
#include <OneWire.h>
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>
#include <ctime>
#include <ArduinoOTA.h>
#include "secret.h"
#include "TrendTracker.h"
#include "esp8266Meteo.h"

time_t            localStTime;
time_t            logTime;
time_t            startTime;
float             DS18B20Temp;
float             BMP180Temp;
float             BMP180Pressure;
float             BMP180PressureMM;
byte              sensor = DS18B20;
byte              gHumidity;
int               greenLed;
int               DHThumidity;
int               DHThumidityRealValue;
int               DHTTemp;
char              startBuffer[32];
char              lastReadTimeBuffer[32];
char              sensorName[10];
unsigned long int botLastTime;
unsigned long int readDataCounter = 0;
unsigned long int runTime;
unsigned long int lastReadDataTime = 0;
unsigned long int userMessages = 0;
WiFiClientSecure  netClient;
DFRobot_DHT11     DHT;
IPAddress         myIP;
Adafruit_BMP085   bmp;
DeviceAddress     temperatureSensors[3];
TrendTracker      trendTemperature;
TrendTracker      trendPressure;
TrendTracker      trendHumidity;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
AsyncWebServer server(80);
#if SECURE_CLIENT == 1
X509List cert(TELEGRAM_CERTIFICATE_ROOT);
#endif
UniversalTelegramBot bot(BOT_TOKEN, netClient);
ADC_MODE(ADC_VCC);

/*######################################################################################*/

bool isRainLikely(float pressureSlope, float humidity, float tempSlope) {
    if (pressureSlope < -0.05f && humidity > 80.0f) {
        return true;
    }
    if (pressureSlope < -0.03f && humidity > 75.0f && tempSlope < 0.0f) {
        return true;
    }
    return false;
}

/*######################################################################################*/

void setThresholds() {

    static float tTemperature = 0;
    static float tPressure = 0;
    static float tHumidity = 0;

    static unsigned long lastExec = 0;

    if(lastExec && millis() - lastExec < 300000UL) return;
    lastExec = millis();

    File tFile = LittleFS.open(THRESHOLD_DATA, "r");

    if (!tFile) {
        trendTemperature.threshold = TEMPERATURE_THRESHOLD;
        trendPressure.threshold = PRESSURE_THRESHOLD;
        trendHumidity.threshold = HUMIDITY_THRESHOLD;
        if(
            trendTemperature.threshold != tTemperature || 
            trendPressure.threshold != tPressure ||
            trendHumidity.threshold != tHumidity
        ) {
            char data[128];
            snprintf(data, sizeof(data), "{\"M\":\"Set default thresholds: T=%.3f, P=%.3f, H=%.3f\"}",
                trendTemperature.threshold, trendPressure.threshold, trendHumidity.threshold
            );
        }
        return;
    }

    auto parseLine = [](const char *&p) -> float {
        while (*p == '\r' || *p == '\n') ++p;

        char buf[32];
        size_t i = 0;
        while (*p && *p != '\n' && *p != '\r' && i < sizeof(buf) - 1)
            buf[i++] = *p++;
        buf[i] = '\0';

        while (*p == '\n' || *p == '\r') ++p;
        return strtof(buf, nullptr);
    };
    
    char buffer[32];
    size_t fileSize = tFile.size();

    tFile.readBytes(buffer, fileSize);
    buffer[fileSize] = '\0';
    const char *ptr = buffer;

    trendTemperature.threshold = *ptr ? parseLine(ptr) : TEMPERATURE_THRESHOLD;
    trendPressure.threshold    = *ptr ? parseLine(ptr) : PRESSURE_THRESHOLD;
    trendHumidity.threshold    = *ptr ? parseLine(ptr) : HUMIDITY_THRESHOLD;

    if(
        trendTemperature.threshold != tTemperature || 
        trendPressure.threshold != tPressure ||
        trendHumidity.threshold != tHumidity
    ) {
        tTemperature = trendTemperature.threshold;
        tPressure = trendPressure.threshold;
        tHumidity = trendHumidity.threshold;
        char data[128];
        snprintf(data, sizeof(data), "{\"M\":\"Got new thresholds: T=%.3f, P=%.3f, H=%.3f\"}",
            trendTemperature.threshold, trendPressure.threshold, trendHumidity.threshold
        );
        toLog(SYS_LOG, "IN", data);
    }

    tFile.close();
    delay(0);
}

/*######################################################################################*/

void readData() {

    unsigned long int nowMills = millis();
    static bool firstTime = true;

    if (((nowMills - lastReadDataTime) < SENSORS_DELAY) && !firstTime) return;

    setThresholds();
    
    lastReadDataTime = millis();
    if (greenLed != 0) {
        digitalWrite(BLUE_PIN, HIGH);
    } else {
        digitalWrite(GREEN_PIN, HIGH);
    }

    if (sensor == DS18B20 || firstTime) {
        sensors.requestTemperatures();
        DS18B20Temp = sensors.getTempC(temperatureSensors[0]);
        TrendTracker_add(&trendTemperature, DS18B20Temp);
#if SERIAL_OUT == 1
        Serial.print(F("DS18B20Temp: "));
        Serial.println(DS18B20Temp);
#endif
    }

    if (sensor == BMP180 || firstTime) {
        BMP180Temp = bmp.readTemperature();
        BMP180Pressure = bmp.readPressure();
        BMP180PressureMM = BMP180Pressure * 0.00750063755419211;
        TrendTracker_add(&trendPressure, BMP180Pressure);
    }

    if (sensor == DHT11 || firstTime) {
        DHT.read(DHT11_PIN);
        DHThumidityRealValue = DHT.humidity;
        DHThumidity = static_cast<int>(DHThumidityRealValue * HUMIDITY_CORRECTION);
        if (DHThumidity > 100) DHThumidity = 100;
        TrendTracker_add(&trendHumidity, static_cast<float>(DHThumidityRealValue));
        DHTTemp = DHT.temperature;
    }

#if SERIAL_OUT == 1
    Serial.println(F("###############################"));
    if (firstTime) {
        Serial.println(F("SENSOR: ALL"));
    } else {
        Serial.print(F("SENSOR: "));
        Serial.println(sensor);
    }
    Serial.print(F("DS18B20Temp: "));
    Serial.println(DS18B20Temp);
    Serial.print(F("BMP180Temp: "));
    Serial.println(BMP180Temp);
    Serial.print(F("BMP180Pressure: "));
    Serial.println(BMP180Pressure);
    Serial.print(F("BMP180PressureMM: "));
    Serial.println(BMP180PressureMM);
    Serial.print(F("DHThumidity: "));
    Serial.println(DHThumidity);
    Serial.print(F("DHTTemp: "));
    Serial.println(DHTTemp);
#endif

    if (sensor == DS18B20) {
        memCopy(sensorName, "DS18B20", sizeof(sensorName));
        sensor = BMP180;
        runTime = millis() - nowMills;
        if (runTime > MAX2LOG_DS18B20 && !firstTime) {
            char data[128];
            snprintf(data, sizeof(data), "{\"RT\":\"%lu\",\"SN\":\"%s\"}", runTime, sensorName);
            toLog(SYS_LOG, "RS", data);
        }
    } else if (sensor == BMP180) {
        memCopy(sensorName, "BMP180", sizeof(sensorName));
        sensor = DHT11;
        runTime = millis() - nowMills;
        if (runTime > MAX2LOG_BMP180) {
            char data[128];
            snprintf(data, sizeof(data), "{\"RT\":\"%lu\",\"SN\":\"%s\"}", runTime, sensorName);
            toLog(SYS_LOG, "RS", data);
        }
    } else if (sensor == DHT11) {
        memCopy(sensorName, "DHT11", sizeof(sensorName));
        sensor = DS18B20;
        runTime = millis() - nowMills;
        if (runTime > MAX2LOG_DHT11) {
            char data[128];
            snprintf(data, sizeof(data), "{\"RT\":\"%lu\",\"SN\":\"%s\"}", runTime, sensorName);
            toLog(SYS_LOG, "RS", data);
        }
    }

    if (firstTime) memCopy(sensorName, "ALL", sizeof(sensorName));

    firstTime = false;
    time_t ltime;
    time(&ltime);
    ltime = ltime + TIMEZONE_OFFSET_SEC;
    snprintf(lastReadTimeBuffer, sizeof(lastReadTimeBuffer), "%s", ctime(&ltime));
    lastReadTimeBuffer[strlen(lastReadTimeBuffer) - 1] = '\0';

    readDataCounter++;

#if SERIAL_OUT == 1
    Serial.print(F("Reading run time: "));
    Serial.print(runTime);
    Serial.println(F(" msec"));
#endif

    if (greenLed != 0) {
        digitalWrite(BLUE_PIN, LOW);
    } else {
        digitalWrite(GREEN_PIN, LOW);
    }

    delay(0);

    return;
}

/*######################################################################################*/

String getContentType(String filename, AsyncWebServerRequest *request) {
    if(request->hasParam("download"))
        return "application/octet-stream";
    else if (filename.endsWith(".htm"))
        return "text/html";
    else if (filename.endsWith(".html"))
        return "text/html";
    else if (filename.endsWith(".css"))
        return "text/css";
    else if (filename.endsWith(".js"))
        return "application/javascript";
    else if (filename.endsWith(".png"))
        return "image/png";
    else if (filename.endsWith(".gif"))
        return "image/gif";
    else if (filename.endsWith(".jpg"))
        return "image/jpeg";
    else if (filename.endsWith(".ico"))
        return "image/x-icon";
    else if (filename.endsWith(".xml"))
        return "text/xml";
    else if (filename.endsWith(".pdf"))
        return "application/x-pdf";
    else if (filename.endsWith(".zip"))
        return "application/x-zip";
    else if (filename.endsWith(".gz"))
        return "application/x-gzip";
    return "text/plain";
}

/*######################################################################################*/

char* formatBytes(char* buf, size_t bufSize, size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    double size = bytes;
    int unitIndex = 0;

    while (size >= 1024.0 && unitIndex < 3) {
        size /= 1024.0;
        ++unitIndex;
    }

    if (unitIndex == 0)
        snprintf(buf, bufSize, "%u%s", (unsigned)bytes, units[unitIndex]);
    else
        snprintf(buf, bufSize, "%.3f%s", size, units[unitIndex]);

    return buf;
}

/*######################################################################################*/

void returnFail(AsyncWebServerRequest *request, const char *msg) {
    int msgSize = sizeof(msg) / sizeof(char);
    char *showMsg = static_cast<char *>(malloc(msgSize * sizeof(char) + 2));
    if (showMsg) {
        strcpy(showMsg, msg);
        strcat(showMsg, "\r\n");
        request->send(500, "text/plain", showMsg);
        free(showMsg);
    } else {
        request->send(500, "text/plain", "System error\r\n");
    }
}

/*######################################################################################*/

void *memCopy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

/*######################################################################################*/

void diskUsage(AsyncWebServerRequest *request) {
    digitalWrite(BLUE_PIN, HIGH);

    FSInfo fs_info;
    LittleFS.info(fs_info);
    long usedBytes = 0;
    long totalBytes = fs_info.totalBytes;

    Dir dir = LittleFS.openDir("/");
    while (dir.next()) {
        usedBytes = usedBytes + dir.fileSize();
    }

    char buffer[256];
    char formatedTotalBytes[20];
    char formatedUsedBytes[20];
    snprintf(buffer, sizeof(buffer), "{\"T\":\"%s\",\"U\":\"%s\",\"TU\":\"%ld\",\"UU\":\"%ld\"}",
            formatBytes(formatedTotalBytes, sizeof(formatedTotalBytes), totalBytes), 
            formatBytes(formatedUsedBytes, sizeof(formatedUsedBytes), usedBytes), totalBytes,
            usedBytes);
#if SERIAL_OUT == 1
    Serial.print(F("diskUsage: "));
    Serial.println(buffer);
#endif

    request->send(200, "text/json", buffer);
    digitalWrite(BLUE_PIN, LOW);

    return;
}

/*######################################################################################*/

void logsList(AsyncWebServerRequest *request) {
    digitalWrite(BLUE_PIN, HIGH);

    int logNumber = 1;
    char logName[24];
    String output;
    output = "[";
    snprintf(logName, sizeof(logName), TMPL_LOG, logNumber);
    while (LittleFS.exists(logName)) {
        output += "{\"LN\":\"" + String(logName) + "\"},";
        logNumber++;
        snprintf(logName, sizeof(logName), TMPL_LOG, logNumber);
    }
    output += "{\"END\":\"NULL\"}]";
    request->send(200, "text/json", output);

    digitalWrite(BLUE_PIN, LOW);
    return;
}

/*######################################################################################*/

void clearLogs(AsyncWebServerRequest *request) {
    digitalWrite(BLUE_PIN, HIGH);

    int logNumber = 1;
    char logName[24];
    String output;
    output = "[";
    snprintf(logName, sizeof(logName), TMPL_LOG, logNumber);
    while (LittleFS.exists(logName)) {
        output += "{\"LN\":\"" + String(logName) + "\"},";
#if SERIAL_OUT == 1
        Serial.print(F("Deleted log: "));
        Serial.println(logName);
#endif
        LittleFS.remove(logName);
        char data[128];
        snprintf(data, sizeof(data), "{\"FL\":\"%s\"}", logName);
        toLog(SYS_LOG, "RM", data);
        logNumber++;
        snprintf(logName, sizeof(data), TMPL_LOG, logNumber);
    }
    output += "{\"END\":\"NULL\"}]";
    request->send(200, "text/json", output);

    digitalWrite(BLUE_PIN, LOW);
    return;
}

/*######################################################################################*/

void getLog(AsyncWebServerRequest *request) {
    digitalWrite(BLUE_PIN, HIGH);
    String path;
    if(request->hasParam("log")) {
        path = request->getParam("log")->value();
    } else {
        path = SYS_LOG;
#if SERIAL_OUT == 1
        Serial.println("Default getLog: " + path);
#endif
    }
    if (!LittleFS.exists(path)) {
#if SERIAL_OUT == 1
        Serial.println("Error getLog: [" + path + "]");
#endif
        request->send(200, "text/plain", "{\"E\":\"Log not found\"}");
        digitalWrite(BLUE_PIN, LOW);
        return;
    }
    File file = LittleFS.open(path, "r");
    request->send(LittleFS, path, "text/plain");
#if SERIAL_OUT == 1
    Serial.println("getLog: " + path);
#endif
    file.close();
    digitalWrite(BLUE_PIN, LOW);
    return;
}

/*######################################################################################*/

bool handleFileRead(AsyncWebServerRequest *request) {
    digitalWrite(BLUE_PIN, HIGH);
    String path = request->url();
    if (path.endsWith("/")) path += "index.html";
    String contentType = getContentType(path, request);
    String pathWithGz = path + ".gz";
    if (LittleFS.exists(pathWithGz) || LittleFS.exists(path)) {
        if (LittleFS.exists(pathWithGz)) path += ".gz";
        File file = LittleFS.open(path, "r");
        request->send(LittleFS, path, contentType);
#if SERIAL_OUT == 1
        Serial.println("handleFileRead: " + path);
#endif
        file.close();
        digitalWrite(BLUE_PIN, LOW);
        return true;
    }
    digitalWrite(BLUE_PIN, LOW);
    return false;
}

/*######################################################################################*/

void handleFileDelete(AsyncWebServerRequest *request) {
    if (request->params() == 0) { // request->hasParam
        request->send(500, "text/plain", "Invalid request");
        return;
    }
    String path = request->getParam((int)0)->value();
#if SERIAL_OUT == 1
    Serial.println("handleFileDelete: " + path);
#endif
    if (path == "/") {
        request->send(500, "text/plain", "Invalid file path");
        return;
    }
    if (!LittleFS.exists(path)) {
        request->send(500, "text/plain", "File not found");
        return;
    }
    LittleFS.remove(path);
    request->send(200, "text/plain", "");
    char data[128];
    snprintf(data, sizeof(data), "{\"FL\":\"%s\"}", path.c_str());
    toLog(SYS_LOG, "RM", data); // RM Removed File
    path = String();
}

/*######################################################################################*/

void handleFileList(AsyncWebServerRequest *request) {
    if (!request->hasParam("dir")) {
        returnFail(request, "Incorrect arguments");
        return;
    }

    String path = request->getParam("dir")->value();
#if SERIAL_OUT == 1
    Serial.println("handleFileList: " + path);
#endif
    Dir dir = LittleFS.openDir(path);
    path = String();

    String output = "[";
    while (dir.next()) {
        File entry = dir.openFile("r");
        if (output != "[")
        output += ',';
        size_t fileSize;
        char sizeFormatedBytes[20];
        fileSize = dir.fileSize();
        output += "{\"type\":\"";
        output += "file";
        output += "\",\"name\":\"";
        output += entry.name();
        output += "\",\"size\":\"";
        output += formatBytes(sizeFormatedBytes, sizeof(sizeFormatedBytes), fileSize);
        output += "\"}";
        entry.close();
    }

    output += "]";
    request->send(200, "text/json", output);
}

/*######################################################################################*/

void getAllData(AsyncWebServerRequest *request) {
    digitalWrite(BLUE_PIN, HIGH);
    time_t ltime;
    time(&ltime);
    long int upTime = static_cast<long int>(ltime - startTime);

    int days  = upTime / 86400;
    int rem   = upTime % 86400;
    int hours = rem / 3600;
    rem       = rem % 3600;
    int mins  = rem / 60;
    int secs  = rem % 60;

    ltime = ltime + TIMEZONE_OFFSET_SEC;

    char curTime[32];
    snprintf(curTime, sizeof(curTime), "%s", ctime(&ltime));
    curTime[strlen(curTime) - 1] = '\0';

    float volts = ESP.getVcc() / 1000.0;

#if SERIAL_OUT == 1
    char SerialOut[] = "ON";
#else
    char SerialOut[] = "OFF";
#endif

    char response[1024];
    float pressureTrend = TrendTracker_getSlope(&trendPressure);
    float temperatureTrend = TrendTracker_getSlope(&trendTemperature);
    char __isRainLikely[] = "&#x1F327; Rain is likely soon";
    if (!isRainLikely(pressureTrend, DHThumidity, temperatureTrend)) {
        snprintf(__isRainLikely, sizeof(__isRainLikely), "&#x2600; No rain expected");
    }

    snprintf(
        response, sizeof(response),
    "{\"DS18B20_t\":\"%s %+2.2f°C\",\"BMP180_t\":\"%+2.2f°C\",\"DHT11_t\":\"%+d°C\",\
    \"BMP180_p\":\"%s %2.2f mmHg (%2.3f kPa)\",\"DHT11_h\":\"%s %d%% (%d%%)\",\"version\":\"%s SerialOut:%s Build:%s\",\
    \"ip\":\"%d.%d.%d.%d\",\"uptime\":\"%d day(s) %02d:%02d:%02d\",\"time\":\"%s\",\"start_time\":\"%s\",\
    \"read_time\":\"%s\",\"read_counter\":\"%lu [%s: %lu msec]\",\"msgs\":\"%lu\",\"volts\":\"%2.3fV\",\
    \"freq\":\"%u MHz\", \"rain\":\"%s\"}",
        TrendTracker_getArrow(&trendTemperature), DS18B20Temp, BMP180Temp, DHTTemp, 
        TrendTracker_getArrow(&trendPressure), BMP180PressureMM,
        BMP180Pressure / 1000, TrendTracker_getArrow(&trendHumidity), DHThumidity, DHThumidityRealValue,
        VERSION, SerialOut, BUILD, myIP[0], myIP[1], myIP[2], myIP[3], days,
        hours, mins, secs, curTime, startBuffer, lastReadTimeBuffer,
        readDataCounter, sensorName, runTime, userMessages,
        volts, ESP.getCpuFreqMHz(), __isRainLikely);
    request->send(200, "text/json", response);
    digitalWrite(BLUE_PIN, LOW);
}

/*######################################################################################*/

void handleNotFound(AsyncWebServerRequest *request) {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += request->url();
    message += "\nMethod: ";
    message += (request->method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += request->params();
    message += "\n";

    for (uint8_t i = 0; i < request->params(); i++) {
        message += " " + request->getParam(i)->name() + ": " + request->getParam(i)->value() + "\n";
    }

    request->send(404, "text/plain", message);
}

/*######################################################################################*/

void handleNewMessages(int numNewMessages) {

    char buffer[1024];
    long int startT = millis();

    for (int i = 0; i < numNewMessages; i++) {
        userMessages++;
        digitalWrite(BLUE_PIN, HIGH);
        if (bot.messages[i].text == "/info") {
            time_t ltime;
            time(&ltime);
            ltime = ltime + TIMEZONE_OFFSET_SEC;
            snprintf(buffer, sizeof(buffer),
                    "<b>Version:</b> <code>%s</code>\n<b>IP</b>: "
                    "<code>%d.%d.%d.%d</code>\n<b>Time</b>: <code>%s</code>",
                    VERSION, myIP[0], myIP[1], myIP[2], myIP[3], ctime(&ltime));
#if SERIAL_OUT == 1
            Serial.println(buffer);
#endif
            bot.sendMessage(bot.messages[i].chat_id, buffer, "HTML");
            char data[128];
            snprintf(data, sizeof(data), "{\"U\":\"%s\",\"RT\":\"%lu\",\"C\":\"i\"}",
                    bot.messages[i].from_name.c_str(), millis() - startT);
            toLog(SYS_LOG, "SM", data);
            digitalWrite(BLUE_PIN, LOW);
            return;
        }
        if (bot.messages[i].text == "/meteo") {
            float pressureTrend = TrendTracker_getSlope(&trendPressure);
            float temperatureTrend = TrendTracker_getSlope(&trendTemperature);
            char __isRainLikely[] = "&#x1F327; Rain is likely soon";
            if (!isRainLikely(pressureTrend, DHThumidity, temperatureTrend)) {
                snprintf(__isRainLikely, sizeof(__isRainLikely), "&#x2600; No rain expected");
            }
            snprintf(
                buffer, sizeof(buffer),
                "Hello, <b>%s</b>!\n<b>Temperature: %s</b> "
                "<code>%+2.2f°C</code>\n<b>Pressure: %s</b> <code>%2.2f mmHg "
                "(%2.3f kPa)</code>\n<b>Humidity: %s</b> <code>%d%%</code>\n%s",
                bot.messages[i].from_name.c_str(),
                TrendTracker_getArrow(&trendTemperature), DS18B20Temp,
                TrendTracker_getArrow(&trendPressure), BMP180PressureMM, BMP180Pressure / 1000,
                TrendTracker_getArrow(&trendHumidity), DHThumidity, __isRainLikely);
#if SERIAL_OUT == 1
            Serial.println(buffer);
#endif
            bot.sendMessage(bot.messages[i].chat_id, buffer, "HTML");
            char data[128];
            snprintf(data, sizeof(data), "{\"U\":\"%s\",\"RT\":\"%lu\",\"C\":\"m\"}", bot.messages[i].from_name.c_str(), millis() - startT);
            toLog(SYS_LOG, "SM", data);
            digitalWrite(BLUE_PIN, LOW);
            return;
        }
        if(bot.messages[i].text == "/location") {
#if SERIAL_OUT == 1
            Serial.print(F("Long: "));
            Serial.println(LONGITUDE);
            Serial.print(F("Lat: "));
            Serial.println(LATITUDE);
#endif
            bot.sendLocation(bot.messages[i].chat_id, LATITUDE, LONGITUDE, 0);
            char data[128];
            snprintf(data, sizeof(data), "{\"U\":\"%s\",\"RT\":\"%lu\",\"C\":\"l\"}", bot.messages[i].from_name.c_str(), millis() - startT);
            toLog(SYS_LOG, "SM", data);
            digitalWrite(BLUE_PIN, LOW);
            return;
        }

        memCopy(buffer,
            "<b>The following commands are available:</b>\n"
            "<code>/meteo</code> - show weather\n"
            "<code>/info</code> - show information about the bot\n"
            "<code>/location</code> - show weather station location information\n",
            sizeof(buffer)
        );

#if SERIAL_OUT == 1
        Serial.println(buffer);
#endif
        bot.sendMessage(bot.messages[i].chat_id, buffer, "HTML");
        char data[128];
        snprintf(data, sizeof(data), "{\"U\":\"%s\",\"RT\":\"%lu\",\"C\":\"h\"}",
            bot.messages[i].from_name.c_str(), millis() - startT);
        toLog(SYS_LOG, "SM", data);
        digitalWrite(BLUE_PIN, LOW);
    }
    digitalWrite(BLUE_PIN, LOW);
    return;
}

/*######################################################################################*/

void createLog(const char *fileNamePath) {
    File fileToWrite = LittleFS.open(fileNamePath, "w");
    if (fileToWrite) {
        char printBuffer[512];
        time(&logTime);
        snprintf(
            printBuffer, sizeof(printBuffer),
            "{\"T\":\"%ld\",\"A\":\"SL\",\"D\":{\"LF\":\"%s\",\"B\":\"%s\"}}", 
            static_cast<long int>(logTime), fileNamePath, BUILD); // SL - Start Log
        fileToWrite.print(printBuffer);
        fileToWrite.close();
    }
}

/*######################################################################################*/

void toLog(const char *fileNamePath, const char *action, char *data) {
    File fileToWrite = LittleFS.open(fileNamePath, "a");
    if (fileToWrite) {
        if (fileToWrite.size() > MAX_LOG_SIZE) {
            fileToWrite.close();
            rotateLogs();
            createLog(SYS_LOG);
            toLog(SYS_LOG, action, data);
            return;
        }
        char printBuffer[512];
        time_t ltime;
        time(&ltime);
        int timeOffset = static_cast<long int>(ltime - logTime);
        snprintf(printBuffer, sizeof(printBuffer), ",{\"T\":\"%d\",\"A\":\"%s\",\"D\":%s}", timeOffset,
                action, data);
        fileToWrite.print(printBuffer);
        fileToWrite.close();
    }
    return;
}

/*######################################################################################*/

int getMaxLogNumber() {
    int maxLogNumber = 1;
    char currentLog[24];
    snprintf(currentLog, sizeof(currentLog), TMPL_LOG, maxLogNumber);
#if SERIAL_OUT == 1
    Serial.print(F("getMaxLogNumber "));
    Serial.println(currentLog);
#endif
    while (LittleFS.exists(currentLog)) {
        maxLogNumber++;
        snprintf(currentLog, sizeof(currentLog), TMPL_LOG, maxLogNumber);
#if SERIAL_OUT == 1
        Serial.print("getMaxLogNumber ");
        Serial.println(currentLog);
#endif
    }
    maxLogNumber--;
    return (maxLogNumber);
}

/*######################################################################################*/

void rotateLogs() {
    int maxLogNumber = getMaxLogNumber();
    int i;
#if SERIAL_OUT == 1
    Serial.print(F("maxLogNumber "));
    Serial.println(maxLogNumber);
#endif
    for (i = maxLogNumber; i > 0; i--) {
        char currentLog[24];
        snprintf(currentLog, sizeof(currentLog), TMPL_LOG, i);
        if (LittleFS.exists(currentLog)) {
        char nextLog[24];
        snprintf(nextLog, sizeof(nextLog), TMPL_LOG, i + 1);
#if SERIAL_OUT == 1
        Serial.print(F("currentLog: "));
        Serial.println(currentLog);
#endif
        LittleFS.rename(currentLog, nextLog);
        }
    }
    char currentLog1[24];
    snprintf(currentLog1, sizeof(currentLog1), TMPL_LOG, 1);
    LittleFS.rename(SYS_LOG, currentLog1);
#if SERIAL_OUT == 1
    Serial.println(F("rotateLogs finished..."));
#endif
}

/*######################################################################################*/

void setup() {

    pinMode(BLUE_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);

    bool ledOnOff = true;
    Serial.begin(115200);
#if SERIAL_OUT == 1
    for (uint8_t t = 10; t > 0; t--) {
        Serial.printf("[SETUP] FIRST WAIT %d...\n", t);
        Serial.flush();
        if (ledOnOff) {
        digitalWrite(GREEN_PIN, HIGH);
        ledOnOff = false;
        } else {
        digitalWrite(GREEN_PIN, LOW);
        ledOnOff = true;
        }
        delay(1000);
    }
    Serial.println(F("#### START ####"));
#endif

    digitalWrite(BLUE_PIN, LOW);
    digitalWrite(GREEN_PIN, LOW);

    if (!LittleFS.begin()) {
#if SERIAL_OUT == 1
        Serial.println(F("LittleFS mount failed!"));
        Serial.flush();
#endif
        while(true) {
            if (ledOnOff) {
                digitalWrite(BLUE_PIN, HIGH);
                ledOnOff = false;
            } else {
                digitalWrite(BLUE_PIN, LOW);
                ledOnOff = true;
            }
            delay(1000);
        }
    }

#if SERIAL_OUT == 1
    Serial.println(F("#### LittleFS ####"));
    Dir dir = LittleFS.openDir("/");
    while (dir.next()) {
        String fileName = dir.fileName();
        size_t fileSize = dir.fileSize();
        char sizeFormatedBytes[20];
        Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), 
        formatBytes(sizeFormatedBytes, sizeof(sizeFormatedBytes), fileSize));
    }
    Serial.printf("\n");
    Serial.flush();
#endif

    bmp.begin();

    sensors.begin();
    uint8_t deviceCount = sensors.getDeviceCount();

    for (uint8_t index = 0; index < deviceCount; index++) {
        sensors.getAddress(temperatureSensors[index], index);
    }

    ledOnOff = true;
    for (uint8_t t = 4; t > 0; t--) {
        if (ledOnOff) {
            digitalWrite(GREEN_PIN, HIGH);
            ledOnOff = false;
        } else {
            digitalWrite(GREEN_PIN, LOW);
            ledOnOff = true;
        }
#if SERIAL_OUT == 1
        Serial.printf("[SETUP] SECOND WAIT %d...\n", t);
        Serial.flush();
#endif
        delay(1000);
    }

    digitalWrite(GREEN_PIN, LOW);
    configTime(0, 0, NTP_SERVER);
#if SECURE_CLIENT == 1
    netClient.setTrustAnchors(&cert);
#else
    netClient.setInsecure();
#endif
    bot.updateToken(BOT_TOKEN);

#if SERIAL_OUT == 1
    Serial.print(F("Connecting to Wifi SSID "));
    Serial.print(SSID);
#endif
    WiFi.begin(SSID, PASSWORD);
    ledOnOff = false;
    while (WiFi.status() != WL_CONNECTED) {
#if SERIAL_OUT == 1
        Serial.print(".");
#endif
        if (ledOnOff) {
            digitalWrite(BLUE_PIN, HIGH);
            ledOnOff = false;
        } else {
            digitalWrite(BLUE_PIN, LOW);
            ledOnOff = true;
        }
        delay(500);
    }

    ArduinoOTA.setHostname("esp8266meteo");
    ArduinoOTA.begin();
    
    server.on("/list", HTTP_GET, handleFileList);
    server.on("/all", HTTP_GET, getAllData);
    server.on("/disk_usage", HTTP_GET, diskUsage);
    server.on("/logs_list", HTTP_GET, logsList);
    server.on("/clear_logs", HTTP_GET, clearLogs);
    server.on("/get_log", HTTP_GET, getLog);
    server.on("/edit", HTTP_DELETE, handleFileDelete);
    server.on("/edit", HTTP_POST,[](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "");
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
        static File uploadFile;
        if(index == 0) {
#if SERIAL_OUT == 1
            Serial.printf("UploadStart: %s\n", filename.c_str());
#endif
            if (!filename.startsWith("/")) filename = "/" + filename;
            if (LittleFS.exists(filename)) {
                LittleFS.remove(filename);
                char data[128];
                snprintf(data, sizeof(data), "{\"FL\":\"%s\"}", filename.c_str());
                toLog(SYS_LOG, "RM", data);
            }
            uploadFile = LittleFS.open(filename, "w");
        }
        if(uploadFile) {
            uploadFile.write(data, len);
        }
        if (final) {
#if SERIAL_OUT == 1
            Serial.printf("UploadEnd: %s (%u)\n", filename.c_str(), index + len);
#endif
            char data[128];
            snprintf(data, sizeof(data), "{\"FL\":\"%s\",\"SZ\":\"%d\"}", filename.c_str(), index + len);
            toLog(SYS_LOG, "UF", data); // UF Uploaded File
            if (uploadFile) {
                uploadFile.close();
            }
        }
    });

    server.on("/inline", [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "This works as well");
    });
    server.serveStatic("/", LittleFS, "/")
        .setDefaultFile("index.html")
        .setCacheControl(CACHE_MAX_AGE);
    server.onNotFound([](AsyncWebServerRequest *request) {
        if (!handleFileRead(request)) {
            request->send(404, "text/plain", "File Not Found");
        }
    });
    server.begin();

#if SERIAL_OUT == 1
    Serial.println(F("HTTP server started"));
    Serial.print(F("\nWiFi connected. IP address: "));
    Serial.println(WiFi.localIP());
#endif

    myIP = WiFi.localIP();

#if SERIAL_OUT == 1
    Serial.print(F("Retrieving time: "));
#endif
    time_t now = time(nullptr);
    ledOnOff = false;
    while (now < 86400) {
        if (ledOnOff) {
            digitalWrite(GREEN_PIN, HIGH);
            ledOnOff = false;
        } else {
            digitalWrite(GREEN_PIN, LOW);
            ledOnOff = true;
        }
#if SERIAL_OUT == 1
        Serial.print(".");
#endif
        delay(200);
        now = time(nullptr);
    }

    digitalWrite(GREEN_PIN, LOW);
    digitalWrite(BLUE_PIN, LOW);

#if SERIAL_OUT == 1
    Serial.println(now);
#endif
    startTime = now;
    localStTime = startTime + TIMEZONE_OFFSET_SEC;
    snprintf(startBuffer, sizeof(startBuffer), "%s", ctime(&localStTime));
    startBuffer[strlen(startBuffer) - 1] = '\0';
    digitalWrite(GREEN_PIN, HIGH);
    greenLed = millis();

    rotateLogs();
    createLog(SYS_LOG);

    TrendTracker_init(&trendTemperature, TEMPERATURE_THRESHOLD);
    TrendTracker_init(&trendPressure, PRESSURE_THRESHOLD);
    TrendTracker_init(&trendHumidity, HUMIDITY_THRESHOLD);

    setThresholds();
}

/*######################################################################################*/

void loop() {

    readData();
    MDNS.update();
    if (millis() - botLastTime > BOT_MTBS) {
        int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
        while (numNewMessages) {
#if SERIAL_OUT == 1
            Serial.println(F("got response"));
#endif
            handleNewMessages(numNewMessages);
            numNewMessages = bot.getUpdates(bot.last_message_received + 1);
        }
        botLastTime = millis();
    }
    if (greenLed != 0 && (millis() - greenLed) > 60000) {
        digitalWrite(GREEN_PIN, LOW);
        greenLed = 0;
    }
    ArduinoOTA.handle();

}

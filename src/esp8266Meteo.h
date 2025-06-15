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

/*######################################################################################

Connecting ESP8266EX NodeMCU 1.0 (ESP-12E Module) pins:

  * DS18B20:
      GPIO0        -> D3
  * BMP180 GY-68:
      SCL: GPIO5   -> D1
      SDA: GPIO4   -> D2
  * DHT11:
      GPIO2        -> D4
  * LEDS:
      BLUE GPIO14  -> D5
      GREEN GPIO12 -> D6

######################################################################################*/

#pragma once

#define VERSION       "3.8.0"
#define BUILD         "00282"
#define SERIAL_OUT    0 // 1 ON or 0 OFF

#define SERVER_STATIC 1 // 1 YES or 0 NO
#define CACHE_MAX_AGE "max-age=86400"

#define SECURE_CLIENT 0 // 1 YES or 0 NO

#define TIMEZONE_OFFSET      3 // Set your time zone offset. For example: Europe/Moscow (UTC+3) -> offset = 3
#define TIMEZONE_OFFSET_SEC (TIMEZONE_OFFSET * 3600)
#define NTP_SERVER          "pool.ntp.org" // Set the closest available NTP server for time synchronization

#define BLUE_PIN      14       // GPIO14 - D5 LED BLUE
#define GREEN_PIN     12       // GPIO12 - D6 LED GREEN
#define DHT11_PIN     2        // GPIO2  - D4 DHT11
#define ONE_WIRE_BUS  0        // GPIO0  - D3 DS18B20

// Logs keys: SL - Start Log, RS - Readed Sensor, SM - Sent Message, UF - File Uploaded, RM - Removed File
#define SYS_LOG       "/system.log" 
#define TMPL_LOG      "/system.%03d.log"
#define MAX_LOG_SIZE  5120

// Set thresholds for TrendTracker to filter out minor fluctuations and detect clear trends. 
// You can adjust them in the file flashFS.src/thresholds.dat.
#define TEMPERATURE_THRESHOLD 0.05f
#define PRESSURE_THRESHOLD    0.05f
#define HUMIDITY_THRESHOLD    0.06f
#define THRESHOLD_DATA        "/thresholds.dat"

#define HUMIDITY_CORRECTION 1.27f

#define MAX2LOG_DS18B20 510L
#define MAX2LOG_BMP180   53L
#define MAX2LOG_DHT11   235L

#define DS18B20       1
#define BMP180        2
#define DHT11         3
#define BOT_MTBS      2500L
#define SENSORS_DELAY 30000L

void   readData();
String getContentType(String filename, AsyncWebServerRequest *request);
char   *formatBytes(char *formatedBytes, size_t sizeFormatedBytes, size_t bytes);
void   *memCopy(void *dest, const void *src, size_t n);
void   returnFail(AsyncWebServerRequest *request, const char *msg);
void   diskUsage(AsyncWebServerRequest *request);
void   logsList(AsyncWebServerRequest *request);
void   clearLogs(AsyncWebServerRequest *request);
void   getLog(AsyncWebServerRequest *request);
bool   handleFileRead(AsyncWebServerRequest *request);
bool   isRainLikely(float pressureSlope, float humidity, float tempSlope);
void   setThresholds();
//void   handleFileUpload();
void   handleFileDelete(AsyncWebServerRequest *request);
void   handleFileList(AsyncWebServerRequest *request);
void   getAllData(AsyncWebServerRequest *request);
void   handleNotFound(AsyncWebServerRequest *request);
void   handleNewMessages(int numNewMessages);
void   createLog(const char *fileNamePath);
void   toLog(const char *fileNamePath, const char *action, char *data);
int    getMaxLogNumber();
void   rotateLogs();

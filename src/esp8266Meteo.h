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

#pragma once

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

#define VERSION       "3.7.2"
#define BUILD         "00263"
#define SERIAL_OUT    0 // 1 ON or 0 OFF

#define SERVER_STATIC 1 // 1 ON or 0 OFF
#define CACHE_MAX_AGE "max-age=86400"

#define SECURE_CLIENT 0 // 1 ON or 0 OFF

#define BLUE_PIN      14       // GPIO14 - D5 LED BLUE
#define GREEN_PIN     12       // GPIO12 - D6 LED GREEN
#define DHT11_PIN     2        // GPIO2  - D4 DHT11
#define ONE_WIRE_BUS  0        // GPIO0  - D3 DS18B20

// Logs keys: SL - Start Log, RS - Readed Sensor, SM - Sent Message, UF - File Uploaded, RM - Removed File
#define SYS_LOG       "/system.log" 
#define TMPL_LOG      "/system.%03d.log"
#define MAX_LOG_SIZE  5120

#define DS18B20       1
#define BMP180        2
#define DHT11         3
#define BOT_MTBS      2000L


uint8_t           deviceCount = 0;
time_t            localStTime;
time_t            logTime;
time_t            startTime;
float             volts;
float             DS18B20Temp;
float             BMP180Temp;
float             BMP180Pressure;
float             BMP180PressureMM = 748.0;
float             BMP180PressureMMPrev;
float             humidityCorrection = 1.27;
bool              firstTime = true;
byte              sensor = DS18B20;
byte              gHumidity;
int               greenLed;
int               DHThumidity;
int               DHThumidityRealValue;
int               DHTTemp;
char              startBuffer[32];
char              lastReadTimeBuffer[32];
char              sensorName[10];
unsigned long int readDataDelay = 60000;
unsigned long int botLastTime;
unsigned long int readDataCounter = 0;
unsigned long int runTime;
unsigned long int lastReadDataTime = 0;
unsigned long int max2log_DS18B20 = 508;
unsigned long int max2log_BMP180 = 46;
unsigned long int max2log_DHT11 = 227;
unsigned long int userMessages = 0;
File              fsUploadFile;
WiFiClientSecure  netClient;
DFRobot_DHT11     DHT;
IPAddress         myIP;
Adafruit_BMP085   bmp;
DeviceAddress     temperatureSensors[3];

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
ESP8266WebServer server(80);
#if SECURE_CLIENT == YES
X509List cert(TELEGRAM_CERTIFICATE_ROOT);
#endif
UniversalTelegramBot bot(BOT_TOKEN, netClient);
ADC_MODE(ADC_VCC);

float  getVoltage();
void   readData();
String getContentType(String filename);
char   *formatBytes(char *formatedBytes, size_t sizeFormatedBytes, size_t bytes);
void   returnFail(const char *msg);
void   diskUsage();
void   logsList();
void   clearLogs();
void   getLog();
bool   handleFileRead(String path);
void   handleFileUpload();
void   handleFileDelete();
void   handleFileList();
void   getAllData();
void   handleNotFound();
void   handleNewMessages(int numNewMessages);
void   createLog(const char *fileNamePath);
void   toLog(const char *fileNamePath, const char *action, char *data);
int    getMaxLogNumber();
void   rotateLogs();
//void   sendLocation(const String chatId, float latitude, float longitude);

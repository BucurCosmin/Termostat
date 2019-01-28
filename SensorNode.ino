
#include "Arduino.h"
#include "src/futils.h"
#include "src/dataAcq.h"
#include "WiFi.h"
#include "time.h"
#include "WebSocketsServer.h"
#include "FreeRTOS.h"
#include "esp_attr.h"

#define FORMAT_SPIFFS_IF_FAILED true
#define WEBSOCKETS_NETWORK_TYPE NETWORK_ESP8266
#define configSUPPORT_DYNAMIC_ALLOCATION 1
double T,P,p0,a;
SFE_BMP180 sensor;
volatile SemaphoreHandle_t xMutex;


const char* ssid       = "scada";
const char* password   = "cerberus";
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;
struct tm timeinfo;
WebSocketsServer server1=WebSocketsServer(81);
char logfile[4096];
char settings[1024];
char timestamp[10];
unsigned long logsize;
double oldTemp;
String TempString;
double setpointTemp;
char setpointWeb[6];
char Thermostat_activated;
char activated[1];

void setup()
{
    xMutex=xSemaphoreCreateBinary();
    //vTaskStartScheduler();
    InitializeController();
    configTime(gmtOffset_sec,daylightOffset_sec,ntpServer);
    getLocalTime(&timeinfo);
    if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
    {
        Serial.println("SPIFFS Mount Failed");
        return;
    }
    xTaskCreatePinnedToCore(taskAcq,"Task1",10000,NULL,100,NULL,1);
    server1.begin();
    server1.onEvent(SocketEvent);
    xTaskCreatePinnedToCore( WebSock,"TaskWS",10000,NULL,5,NULL,1);
}

void loop()
{
}

void taskAcq(void* param)
{
    while (true)
    {
        ReadBMPSensor(sensor,&T,&P);
        configTime(gmtOffset_sec,daylightOffset_sec,ntpServer);
        getLocalTime(&timeinfo);

        xSemaphoreTake(xMutex,0);
        taskYIELD();
        int sec=timeinfo.tm_sec;
        char numberstring[(((sizeof sec) * CHAR_BIT) + 2)/3 + 2];
        sprintf(numberstring, "%d", sec);
        strcat(timestamp,numberstring);
        int min=timeinfo.tm_min;
        char numberstring1[(((sizeof min) * CHAR_BIT) + 2)/3 + 2];
        sprintf(numberstring1, "%d", min);
        strcat(timestamp," ");
        strcat(timestamp,numberstring1);
        int hour=timeinfo.tm_hour;
        char numberstring2[(((sizeof hour) * CHAR_BIT) + 2)/3 + 2];
        sprintf(numberstring2, "%d", hour);
        strcat(timestamp," ");
        strcat(timestamp,numberstring2);
        //logsize= ReadFileW(SPIFFS,"/log.txt",logfile);
        if (abs(T-oldTemp)>0.5)
        {
            WriteValue(SPIFFS,String(T,2),timestamp,"/log.txt");
            oldTemp=T;
        }
        //ReadFile(SPIFFS,"/log.txt");
        xSemaphoreGive(xMutex);
        taskYIELD();
        strcpy (timestamp, "");
        delay(1000);
    }
}

void keepTime(void* param)
{
    delay(1000);
    timeinfo.tm_sec+=1;
    if (timeinfo.tm_sec==60)
    {
        timeinfo.tm_sec=0;
        timeinfo.tm_min+=1;
        if (timeinfo.tm_min==60)
        {
            timeinfo.tm_min=0;
            timeinfo.tm_hour+=1;
            if(timeinfo.tm_hour==24)
            {
                timeinfo.tm_hour=0;
            }
        }
    }

    //configTime(gmtOffset_sec,daylightOffset_sec,ntpServer);
   // getLocalTime(&timeinfo);
}

void SocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
    switch(type)
    {
        case WStype_DISCONNECTED:
        Serial.printf("[%u] Disconnected! \n",num);
        break;
        case WStype_CONNECTED:
        {
            IPAddress ip = server1.remoteIP(num);
            Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        }
        break;
        case WStype_TEXT:
        {
            switch(*payload)
            {
                case '0':
                setpointWeb[0]=payload[2];
                setpointWeb[1]=payload[3];
                setpointWeb[2]=payload[4];
                setpointWeb[3]=payload[5];
                setpointWeb[4]=payload[6];
                server1.sendTXT(num,setpointWeb);
                setpointTemp= atof(setpointWeb);
                WriteSetpoint1(SPIFFS,String(setpointTemp,2),"/settings.txt");
                break;
                case '1':
                Serial.println("interogation received");
                TempString="1 "+ String(T,2);
                server1.sendTXT(num,TempString);
                break;
                case '2':
                xSemaphoreTake(xMutex,portMAX_DELAY);
                taskYIELD();
                logsize= ReadFileW(SPIFFS,"/log.txt",logfile);
                server1.sendTXT(num,"2 "+String(logfile));
                xSemaphoreGive(xMutex);
                taskYIELD();
                break;
                case '3':
                deleteLOG(SPIFFS,"/log.txt");
                break;
                case '4':
                ReadFileW(SPIFFS,"/settings.txt",settings);
                server1.sendTXT(num,"4 "+String(settings));
                break;
                case '5':
                TempString="5 "+String(GetLogSize(SPIFFS,"/log.txt"));
                server1.sendTXT(num,TempString);
                break;
                case '6':
                Thermostat_activated=payload[2];
                if (Thermostat_activated=='0')
                {
                    WriteSetpoint1(SPIFFS,"0","/activated.txt");
                }
                else{
                    WriteSetpoint1(SPIFFS,"1","/activated.txt");
                }
                break;
                case '7':
                ReadFileByte(SPIFFS,"/activated.txt",activated);
                Thermostat_activated=activated[0];
                //server1.sendTXT(num,"7 "+String(activated));
                server1.sendTXT(num,"7 "+String(Thermostat_activated));
                break;
                case '8':
                deleteLOG(SPIFFS,"/activated.txt");
                break;
                default:
                server1.sendTXT(num,"100 "+String("not implemented"));
                break;
            }
        }
        break;
        case WStype_BIN:
        Serial.printf("[%u] get binary length: %u\n", num, length);
        break;
        case WStype_ERROR:      
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
        break;
    }
}

void WebSock(void* param)
{
    while(true)
    {
        server1.loop(); 
    }
}

void InitializeController()
{
    Serial.begin(115200);
    if (sensor.begin())
    {
        Serial.println("BMP180 init success");
    }
    else{
        Serial.println("BMP180 error");
    }
    WiFi.begin(ssid,password);
    while (WiFi.status()!=WL_CONNECTED)
    {
        delay(5000);
        Serial.print(".");
    }
    ReadFileW(SPIFFS,"/settings.txt",settings);
    setpointWeb[0]=settings[0];
    setpointWeb[1]=settings[1];
    setpointWeb[2]=settings[2];
    setpointWeb[3]=settings[3];
    setpointWeb[4]=settings[4];
    setpointWeb[5]=settings[5];
    setpointTemp=atof(setpointWeb);
    activated[0]='0';
    ReadFileByte(SPIFFS,"/activated.txt",activated);
    Thermostat_activated=activated[0];
    Serial.println(setpointTemp);
}
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include "FreeRTOS.h"
#include <EEPROM.h>
#include "TTGO-RGB-Pixel.h"
#include <ESPmDNS.h>

#define MAX_PIXEL_NUMS 999
#define DEFAULT_PIXEL_NUMS 30
#define WS2812B_OUTPUT_PIN 13

AsyncWebServer server(80);
uint8_t rgbMode = 0;
EventGroupHandle_t eventHandle = NULL;

enum
{
    TTGO_RGB_STATE,
    TTGO_RGB_MODE,
    TTGO_RGB_BRIGHTNESS,
    TTGO_RGB_NUMS,
    TTGO_RGB_PARAMS_MAX,
};

enum
{
    WIFI_CONNECTED = 1 << 2,
    WIFI_LOST_CONNECTED = 1 << 3,
};

#define ALL_BITS (RGB_RUN | RGB_STOP | WIFI_CONNECTED | WIFI_LOST_CONNECTED)

void WiFiEvent(WiFiEvent_t event)
{
    Serial.printf("[WiFi-event] event: %d\n", event);
    switch (event)
    {
    case SYSTEM_EVENT_STA_GOT_IP:
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        xEventGroupClearBits(eventHandle, WIFI_LOST_CONNECTED);
        xEventGroupSetBits(eventHandle, WIFI_CONNECTED);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.println("WiFi lost connection");
        xEventGroupClearBits(eventHandle, WIFI_CONNECTED);
        xEventGroupSetBits(eventHandle, WIFI_LOST_CONNECTED);
        break;
    }
}

void setup()
{
    Serial.begin(115200);

    if (!SPIFFS.begin())
    {
        Serial.println("SPIFFS is not database");
        Serial.println("Please use Arduino ESP32 Sketch data Upload files");
        while (1)
        {
            delay(1000);
        }
    }

    eventHandle = xEventGroupCreate();
    if (!eventHandle)
    {
        Serial.println("Create Failed");
        esp_restart();
    }

    WiFi.onEvent(WiFiEvent);
    WiFi.mode(WIFI_STA);
    WiFi.begin();

    if (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
        Serial.println("SmartConfig Begin ...");
        WiFi.beginSmartConfig();
        while (!WiFi.smartConfigDone())
        {
            Serial.print(".");
            delay(1000);
        }
        WiFi.stopSmartConfig();
        WiFi.setAutoConnect(true);
    }

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Wait for got ip address");
        delay(500);
    }

    if (MDNS.begin("ttgo"))
    {
        Serial.println("MDNS responder started");
    }

    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    server.on("/jquery-1.4.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/jquery-1.4.min.js", "application/javascript");
    });

    server.on("/nums", HTTP_POST, [](AsyncWebServerRequest *request) {
        Serial.print("Set Pixel Nums:");
        if (String("nums") == request->getParam(0)->name())
        {
            uint16_t nums = atoi(request->getParam(0)->value().c_str());
            Serial.println(nums);
            EventBits_t oldBits = xEventGroupGetBits(eventHandle);
            xEventGroupClearBits(eventHandle, oldBits);
            xEventGroupSetBits(eventHandle, RGB_STOP);
            delay(500);
            TTGO_RGB_setNums(nums);
            xEventGroupSetBits(eventHandle, oldBits);
            EEPROM.write(TTGO_RGB_NUMS, nums);
            EEPROM.commit();
        }
    });

    server.on("/state", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->params())
        {
            if (String("state") == request->getParam(0)->name())
            {
                uint8_t state = atoi(request->getParam(0)->value().c_str());
                if (state)
                {
                    Serial.println("ON");
                    xEventGroupClearBits(eventHandle, RGB_STOP);
                    xEventGroupSetBits(eventHandle, RGB_RUN);
                }
                else
                {
                    Serial.println("OFF");
                    xEventGroupClearBits(eventHandle, RGB_RUN);
                    xEventGroupSetBits(eventHandle, RGB_STOP);
                }
                EEPROM.write(TTGO_RGB_STATE, state);
                EEPROM.commit();
            }
        }
        request->send(200, "text/plain", "");
    });

    server.on("/mode", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->params())
        {
            if (String("mode") == request->getParam(0)->name())
            {
                rgbMode = atoi(request->getParam(0)->value().c_str());
                xEventGroupClearBits(eventHandle, RGB_RUN);
                xEventGroupSetBits(eventHandle, RGB_STOP);
                delay(500);
                Serial.println("RUN .. ");
                xEventGroupSetBits(eventHandle, RGB_RUN);
                EEPROM.write(TTGO_RGB_MODE, rgbMode);
                EEPROM.commit();
            }
        }
        request->send(200, "text/plain", "");
    });

    server.on("/brightness", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->params())
        {
            if (String("brightness") == request->getParam(0)->name())
            {
                uint8_t level = atoi(request->getParam(0)->value().c_str());
                EventBits_t oldBits = xEventGroupGetBits(eventHandle);
                xEventGroupClearBits(eventHandle, oldBits);
                TTGO_RGB_setBrightness(level);
                xEventGroupSetBits(eventHandle, oldBits);
                EEPROM.write(TTGO_RGB_BRIGHTNESS, level);
                EEPROM.commit();
            }
        }
        request->send(200, "text/plain", "");
    });

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
    });

    MDNS.addService("http", "tcp", 80);

    server.begin();

    EEPROM.begin(TTGO_RGB_PARAMS_MAX - 1);

    uint16_t nums = EEPROM.readUShort(TTGO_RGB_NUMS);
    if (nums >= MAX_PIXEL_NUMS || nums == 0)
    {
        nums = DEFAULT_PIXEL_NUMS;
        Serial.println("SET DEFAULT_PIXEL_NUMS 30");
    }

    TTGO_RGB_init(WS2812B_OUTPUT_PIN, nums);

    rgbMode = EEPROM.read(TTGO_RGB_MODE);
    if (rgbMode == 255)
    {
        Serial.printf("EEPROM mode:%u \n", rgbMode);
        rgbMode = 1;
    }

    TTGO_RGB_setBrightness(EEPROM.read(TTGO_RGB_BRIGHTNESS));

    if (EEPROM.read(TTGO_RGB_STATE))
    {
        Serial.println("ON");
        xEventGroupClearBits(eventHandle, RGB_STOP);
        xEventGroupSetBits(eventHandle, RGB_RUN);
    }
    else
    {
        Serial.println("OFF");
        xEventGroupClearBits(eventHandle, RGB_RUN);
        xEventGroupSetBits(eventHandle, RGB_STOP);
    }
}

void loop()
{
    EventBits_t eventBits;
    for (;;)
    {
        eventBits = xEventGroupWaitBits(eventHandle, ALL_BITS, pdFALSE, pdFALSE, portMAX_DELAY);
        Serial.println(eventBits);
        if (eventBits & RGB_RUN)
        {
            Serial.printf("RUN RGB MODE[%d]\n", rgbMode);
            switch (rgbMode)
            {
            case 0:
                TTGO_RGB_breathe();
                break;
            case 1:
                TTGO_RGB_theaterChaseRainbow(50);
                break;
            case 2:
                TTGO_RGB_rainbowCycle(50);
                break;
            case 3:
                TTGO_RGB_rainbow(50);
                break;
            case 4:
                rainbowFade2White(3, 3, 1);
                break;
            case 5:
                whiteOverRainbow(20, 75, 5);
                break;
            default:
                break;
            }
        }
        if (eventBits & RGB_STOP)
        {
            Serial.println("Recv OFF CMD");
            eventBits = xEventGroupGetBits(eventHandle);
            xEventGroupClearBits(eventHandle, eventBits);
            TTGO_RGB_setState(0);
        }
        if (eventBits & WIFI_LOST_CONNECTED)
        {
            Serial.println("reconnect");
            WiFi.reconnect();
        }
    }
}

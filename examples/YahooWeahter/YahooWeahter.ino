#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <time.h>
#include "YahooWeatherAPI.h"

#define WIFI_SSID               "YOUR_WIFI_SSID"
#define WIFI_PASSWORD           "YOUR_WIFI_PASSWORD"

#define YAHOO_APPID             "YAHOO_APPID"
#define YAHOO_CONSUMERKEY       "YAHOO_CONSUMERKEY"
#define YAHOO_CONSUMER_SECRET   "YAHOO_CONSUMER_SECRET"
#define CITY_NAME               "Shenzhen"      //Replace with the city name you need

#define TIME_ZONE               "CST-8"         //Replace with your time zone
#define NTP_SERVER              "pool.ntp.org"
#define FILESYSTEM              SPIFFS           //default SPIFFS or SD


YahooWeather yahoo(YAHOO_APPID, YAHOO_CONSUMERKEY, YAHOO_CONSUMER_SECRET);

const char *getDateTime(time_t timestamp)
{
    static char buf[100];
    struct tm *p;
    p = gmtime(&timestamp);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", p);
    return buf;
}

void setup()
{
    Serial.begin(115200);
    Serial.println();

    /*********************************
     *        FILESYSTEM
     ********************************/
    //! Yahoo weather forecast information is saved in SPIFFS by default, so you must call SPIFFS initialization or SD card
    if (!FILESYSTEM.begin(true)) {
        Serial.println("FILESYSTEM is not database");
        Serial.println("Please use Arduino ESP32 Sketch data Upload files");
        while ((1)) {
        }
    }


    /*********************************
     *        WiFi
     ********************************/
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        while ((1)) {
        }
    }
    Serial.print(F("WiFi connected"));
    Serial.print("  ");
    Serial.println(WiFi.localIP());


    /*********************************
     *        NTP
     ********************************/
    configTzTime(TIME_ZONE, NTP_SERVER);

    struct tm timeinfo;
    uint8_t retry = 0;
    while (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        if (++retry > 10) {
            delay(1000);
            esp_restart();
        }
    }
    time_t t;
    time(&t);
    Serial.printf("current timestamp : %ld\n", t);

    /*********************************
    *        Yahoo Setting
    ********************************/
    yahoo.setCityName(CITY_NAME);                       //use city name
    // yahoo.setLocationt("22.6205393","114.1021306");  //use location
    // yahoo.setWoeid("2161853");                       //use zip code
    yahoo.setUnitFormat(true);                          //true use imperial , false use metric ,defaule metric

    //! Yahoo authentication needs to use the current timestamp, otherwise it will fail
    if (yahoo.updateWeather(t)) {
        Serial.println("Update Weather Information success");
    } else {
        Serial.println("Update Weather Information failed");
    }

    if (yahoo.info.isValid()) {
        Serial.printf("Publish Time:%s\n", getDateTime(yahoo.info.pubDate));
        Serial.printf("CityName:%s\n", yahoo.info.city.c_str());
        Serial.printf("Weather Code: %d\n", yahoo.info.code);
        Serial.printf("Weather Text: %s\n", yahoo.info.text.c_str());
        Serial.printf("TimeZone:%s\n", yahoo.info.timeZone.c_str());
        Serial.printf("Temperature : %d %s\n", yahoo.info.temperature, yahoo.getUnit() ? "*F" : "*C");
        Serial.printf("Humidity: %d%%\n", yahoo.info.humidity);
        Serial.printf("Visibility: %.2f %s\n", yahoo.info.visibility, yahoo.getUnit() ? "mile" : "km");
        Serial.printf("Pressure: %.2f %s\n", yahoo.info.pressure, yahoo.getUnit() ? "inchHg" : "mbar");
        Serial.printf("Sunrise: %s\n", yahoo.info.sunrise.c_str());
        Serial.printf("Sunset: %s\n", yahoo.info.sunset.c_str());
        Serial.printf("State of the barometric pressure: %d\n", yahoo.info.rising);

        Serial.printf("Wind chill in degrees: %d\n", yahoo.info.chill);
        Serial.printf("Wind direction, in degrees: %d\n", yahoo.info.direction);
        Serial.printf("Wind speed: %d %s\n", yahoo.info.speed, yahoo.getUnit() ? "m/h" : "km/h");

        Serial.println("----------------------------------------------------------------------------------------");
        Serial.println(" Date                     Day     Low     High        Code        Text");
        Serial.println("----------------------------------------------------------------------------------------");
        for (int i = 0; i < yahoo.info.forecastsSize(); i++) {
            Serial.print(getDateTime(yahoo.info.forecasts[i].date));
            Serial.print("      ");
            Serial.print((yahoo.info.forecasts[i].day));
            Serial.print("      ");
            Serial.printf("%02d", (yahoo.info.forecasts[i].low));
            Serial.print("      ");
            Serial.printf("%02d", (yahoo.info.forecasts[i].high));
            Serial.print("          ");
            Serial.print((yahoo.info.forecasts[i].code));
            Serial.print("          ");
            Serial.print((yahoo.info.forecasts[i].text));
            Serial.println();
        }
    }
}

void loop()
{

}


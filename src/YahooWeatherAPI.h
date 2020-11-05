/*
YahooWeatherAPI.h

Arduino version Yahoo Weather library
https://github.com/lewisxhe/YahooWeatherAPI

Lewis He (1/28/2020) : Create and submit
       - (11/05/2020) Added cJSON support
Written by Lewis He
*/

#pragma once

#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>

// #define YAHOO_DEBUG
#define ENABLE_CJSON

#define _YAHOO_USE_CITYNAME     0
#define _YAHOO_USE_LOCTION      1
#define _YAHOO_USE_WOEID        2

/*
                Imperial ("u=f")    Metric ("u=c")
Temperature     Fahrenheit          Celsius
Distance        Mile                Kilometer
Wind Direction  Degree              Degree
Wind Speed      Mile Per Hour       Kilometer Per Hour
Humidity        Percentage          Percentage
Pressure        Inch Hg             Millibar
*/

class YahooWeather;

struct YahooWeatherInformation {
    friend class YahooWeather;
public:
    YahooWeatherInformation() : valid(false)
    {}
    bool isValid() const
    {
        return valid;
    }
    uint8_t forecastsSize() const
    {
        return 10;
    }
    String city;
    String timeZone;
    String region;
    String country;
    String timezone_id;

    float lat;
    float lon;

    int woeid;

    int chill;          //寒意
    int direction;      //方向
    int speed;          //风速

    int humidity;       //湿度
    float visibility;   //能见度
    float pressure;     //压力
    int rising;         //大气压力的状态 -> 稳定（0），上升（1）或下降（2）。 （整数：0、1、2）

    String sunrise;     //日出
    String sunset;      //日落

    String text;        //文本
    int code;           //天气代码
    int temperature;    //温度

    uint32_t pubDate;   //时间戳

    struct forecasts_struct {
        String day;     //星期
        String text;    //文本
        uint32_t date;  //时间戳
        int low;        //最低温度
        int high;       //最高温度
        int code;       //天气代码
    } forecasts[10];    //预报天气数量
private:
    bool valid;
};

class YahooWeather
{
public:
    YahooWeatherInformation info;

    YahooWeather(const char *app_id, const char *consumer_key, const char *consumer_secret, FS &fs = SPIFFS);
    void setCityName(const char *location);
    void setLocationt(const char *lat, const char *lon);
    void setWoeid(const char *id);
    bool updateWeather(long time);
    void setUnitFormat(bool isImperial = true);
    uint8_t getUnit();
private:
    bool parseJson();
    bool _isImperial;
    String _consumer_key;
    String _consumer_secret;
    String _app_id;
    String _location;
    String _woeid;
    String _lat, _lon;
    uint8_t _use_flag;
    FS *_fs;
};
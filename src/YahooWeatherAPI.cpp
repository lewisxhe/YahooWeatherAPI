/*
YahooWeatherAPI.cpp

Arduino version Yahoo Weather library
https://github.com/lewisxhe/YahooWeatherAPI

Lewis He (1/28/2020) : Create and submit
       - (11/05/2020) Added cJSON support
Written by Lewis He
*/
#include "YahooWeatherAPI.h"
#include <string.h>
#include <base64.h>
#include "mbedtls/md.h"
#include <HTTPClient.h>
#ifdef ENABLE_CJSON
#include "cJSON.h"
#else
#include <ArduinoJson.h>
#endif


#define YAHOO_JSON_FLIE         "/yahoo.json"
#define YAHOO_URL               "https://weather-ydn-yql.media.yahoo.com/forecastrss"
#define YAHOO_METHOD            "GET"
#define YAHOO_SIGNATURE_METHOD  "HMAC-SHA1"
#define CONCAT                  "&"

char *urlEncode(const char *input)
{
    const char *hex = "0123456789ABCDEF";
    char *output, *temp;

    temp = (char *) malloc(3 * strlen(input) * sizeof(char) + 1); // Still dependent on cstring. Fix?
    if (temp == NULL) {
        return NULL;
    } else {
        output = temp;
    }
    while (*input != '\0') {
        if ((('a' <= *input) && (*input <= 'z') ) ||
                (('A' <= *input) && (*input <= 'Z') ) ||
                (('0' <= *input) && ( *input <= '9')) ||
                ((*input == '-' ) || (*input == '_' )) ||
                ((*input == '.'))) {
            *temp = *input;
        } else if (*input == ' ') {
            *temp = '+'; // Or is it "%20"?
        } else {
            *temp = '%'; temp++;
            *temp = hex[*input >> 4]; temp++;
            *temp = hex[*input & 0x0f];
        }
        input++; temp++;
    }
    *temp = '\0';
    return output; // Better free() this up after use.
}

YahooWeather::YahooWeather(const char *app_id, const char *consumer_key, const char *consumer_secret, FS &fs)
{
    _app_id = app_id;
    _consumer_key = consumer_key;
    _consumer_secret = consumer_secret;
    _fs = &fs;
    _use_flag = _YAHOO_USE_CITYNAME;
}

void YahooWeather::setCityName(const char *location)
{
    _use_flag = _YAHOO_USE_CITYNAME;
    _location = location;
}

void YahooWeather::setLocationt(const char *lat, const char *lon)
{
    _use_flag = _YAHOO_USE_LOCTION;
    _lat = lat;
    _lon = lon;
}

void YahooWeather::setWoeid(const char *id)
{
    _use_flag = _YAHOO_USE_WOEID;
    _woeid = id;
}

bool YahooWeather::updateWeather(long time)
{
    long randValue = random(time);
    String oauth_nonce = String(randValue);
    String oauth_timestamp = String(time);

    struct oauth {
        const char *key;
        const char *val;
    } yahoo_oauth[] = {
        {"oauth_consumer_key", _consumer_key.c_str()},
        {"oauth_nonce", oauth_nonce.c_str()},
        {"oauth_signature_method", "HMAC-SHA1"},
        {"oauth_timestamp", oauth_timestamp.c_str()},
        {"oauth_version", "1.0"},
    };

    String params = "";
    params += "format=json";
    switch (_use_flag) {
    case _YAHOO_USE_CITYNAME:
        params += CONCAT;
        params += "location=";
        params += _location;
        break;
    case _YAHOO_USE_LOCTION:
        params += CONCAT;
        params += "lat=";
        params += _lat;
        params += CONCAT;
        params += "lon=";
        params += _lon;
        break;
    default:
        break;
    }

    params += CONCAT;

    for (int i = 0; i < sizeof(yahoo_oauth) / sizeof(yahoo_oauth[0]); i++) {
        params += yahoo_oauth[i].key;
        params += "=";
        params += yahoo_oauth[i].val;
        if (i != sizeof(yahoo_oauth) / sizeof(yahoo_oauth[0]) - 1) {
            params += CONCAT;
        }
    }

    if (!_isImperial) {
        params += CONCAT;
        params += "u=c";
    }

    if (_use_flag == _YAHOO_USE_WOEID) {
        params += CONCAT;
        params += "woeid=";
        params += _woeid;
    }

#ifdef YAHOO_DEBUG
    Serial.printf("params:%s\n", params.c_str());
#endif
    char *url_code = urlEncode(YAHOO_URL);
    char *opts = urlEncode((char *)params.c_str());

    if (url_code == NULL) {
        return false;
    }
    if (opts == NULL) {
        free(url_code);
        return false;
    }

    String signature_base_str = YAHOO_METHOD;
    signature_base_str += CONCAT;
    signature_base_str += url_code;
    signature_base_str += CONCAT;
    signature_base_str += opts;

    free(url_code);
    free(opts);

#ifdef YAHOO_DEBUG
    Serial.println();
    Serial.printf("signature_base_str:%s\n", signature_base_str.c_str());
    Serial.println();
    Serial.printf("oauth_nonce:%s\n", oauth_nonce.c_str());
#endif

    String composite_key = _consumer_secret;
    composite_key += CONCAT;

    uint8_t result[32] = {0};
#ifdef YAHOO_DEBUG
    Serial.println();
    Serial.printf("composite_key:%s\n", composite_key.c_str());
    Serial.println();
#endif

    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA1;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
    mbedtls_md_hmac_starts(&ctx, (const unsigned char *) composite_key.c_str(), composite_key.length());
    mbedtls_md_hmac_update(&ctx, (const unsigned char *) signature_base_str.c_str(), signature_base_str.length());
    mbedtls_md_hmac_finish(&ctx, result);
    uint8_t size = mbedtls_md_get_size(ctx.md_info);
    mbedtls_md_free(&ctx);

#ifdef YAHOO_DEBUG
    Serial.print("hmac sha1:");
    for (int i = 0; i < size; i++) {
        Serial.printf("%02x", result[i]);
    }
    Serial.println();
#endif

    String base64Code = base64::encode(result, size);

    String auth_header = "OAuth ";

    for (int i = 0; i < sizeof(yahoo_oauth) / sizeof(yahoo_oauth[0]); i++) {
        auth_header += yahoo_oauth[i].key;
        auth_header += "=";
        auth_header += yahoo_oauth[i].val;
        auth_header += ",";
    }
    auth_header += "oauth_signature=";
    auth_header += base64Code;

#ifdef YAHOO_DEBUG
    Serial.println();
    Serial.printf("base64Code:%s\n", base64Code.c_str());
    Serial.println();
    Serial.printf("auth_header:%s\n", auth_header.c_str());
    Serial.println();
#endif

    String url = YAHOO_URL;
    url += "?";
    if (!_isImperial) {
        url += "u=c";
        url += CONCAT;
    }
    switch (_use_flag) {
    case _YAHOO_USE_CITYNAME:
        url += "location=";
        url += _location;
        break;
    case _YAHOO_USE_LOCTION:
        url += "lat=";
        url += _lat;
        url += CONCAT;
        url += "lon=";
        url += _lon;
        break;
    case _YAHOO_USE_WOEID:
        url += "woeid=";
        url += _woeid;
        break;
    default:
        break;
    }

    url += CONCAT;
    url += "format=json";

#ifdef YAHOO_DEBUG
    Serial.printf("Request url:%s\n", url.c_str());
#endif

    HTTPClient client;
    client.begin(url);
    client.addHeader("Authorization", auth_header);
    client.addHeader("Yahoo-App-Id", _app_id);

    int httpCode = client.GET();
#ifdef YAHOO_DEBUG
    Serial.printf("httpCode:%d\n", httpCode);
#endif
    if (httpCode == HTTP_CODE_OK) {

        File f = _fs->open(YAHOO_JSON_FLIE, "w");
        if (!f) {
#ifdef YAHOO_DEBUG
            Serial.println("file open failed");
#endif
            client.end();
            return false;
        }

        uint8_t buff[512] = { 0 };
        int total = client.getSize();
        int len = total;
        WiFiClient *stream = client.getStreamPtr();
        while (client.connected() && (len > 0 || len == -1)) {
            size_t size = stream->available();
            if (size) {
                int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
                f.write(buff, c);
                if (len > 0) {
                    len -= c;
                }
            }
            delay(1);
        }
        Serial.println();
        f.close();
        client.end();
    } else {
#ifdef YAHOO_DEBUG
        Serial.printf("[HTTP] GET... failed, error: %s\n", client.errorToString(httpCode).c_str());
#endif
        client.end();
        return false;
    }
    return parseJson();
}


bool YahooWeather::parseJson()
{
    File file = _fs->open(YAHOO_JSON_FLIE);
    if (!file) {
#ifdef YAHOO_DEBUG
        Serial.println("Open Fial");
#endif
        return false;
    }

#ifndef ENABLE_CJSON
#if ARDUINOJSON_VERSION_MAJOR == 5
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.parseObject(file);
    if (!root.success()) {
#ifdef YAHOO_DEBUG
        Serial.println(F("Parse json failed"));
#endif
        file.close();
        return false;
    }
#ifdef YAHOO_DEBUG
    root.prettyPrintTo(Serial);
#endif
#elif ARDUINOJSON_VERSION_MAJOR == 6
    DynamicJsonDocument doc(5 * 1024);
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
#ifdef YAHOO_DEBUG
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
#endif
        return false;
    }
    JsonObject root = doc.as<JsonObject>();
#ifdef YAHOO_DEBUG
    serializeJson(doc, Serial);
    Serial.println();
#endif
#else
#error "ArduinoJson Verson is not set ,please see https://github.com/bblanchon/ArduinoJson"
#endif

    const char *city = (const char *)root["location"]["city"];
    if (city == NULL) {
        info.valid = false;
        return false;
    }

    info.city =  city;
    info.valid = true;
    info.timeZone = (const char *)root["location"]["timezone_id"];

    info.chill = root["current_observation"]["wind"]["chill"];
    info.direction = root["current_observation"]["wind"]["direction"];
    info.speed = root["current_observation"]["wind"]["speed"];

    info.humidity = root["current_observation"]["atmosphere"]["humidity"];
    info.visibility = root["current_observation"]["atmosphere"]["visibility"];
    info.pressure = root["current_observation"]["atmosphere"]["pressure"];
    info.rising = root["current_observation"]["atmosphere"]["rising"];

    info.sunrise =  (const char *)root["current_observation"]["astronomy"]["sunrise"];
    info.sunset = (const char *) root["current_observation"]["astronomy"]["sunset"];

    info.text =  (const char *)root["current_observation"]["condition"]["text"];
    info.code = root["current_observation"]["condition"]["code"];
    info.temperature = root["current_observation"]["condition"]["temperature"];

    info.pubDate = root["current_observation"]["pubDate"];

    JsonArray array = root["forecasts"];

    for (int i = 0; i < array.size(); i++) {
        info.forecasts[i].day =  (const char *)array[i]["day"];
        info.forecasts[i].date = array[i]["date"];
        info.forecasts[i].low = array[i]["low"];
        info.forecasts[i].high = array[i]["high"];
        info.forecasts[i].text =  (const char *)array[i]["text"];
        info.forecasts[i].code = array[i]["code"];
    }
#else

    cJSON *root = cJSON_Parse(file.readString().c_str());
    file.close();
    if (!root) {
#ifdef YAHOO_DEBUG
        Serial.println(F("cJSON_Parse() failed!"));
#endif
        info.valid = false;
        return false;
    }

    /*location*/
    cJSON *location = cJSON_GetObjectItem(root, "location");
    cJSON *city = cJSON_GetObjectItem(location, "city");
    cJSON *region = cJSON_GetObjectItem(location, "region");
    cJSON *woeid = cJSON_GetObjectItem(location, "woeid");
    cJSON *country = cJSON_GetObjectItem(location, "country");
    cJSON *lat = cJSON_GetObjectItem(location, "lat");
    cJSON *lon = cJSON_GetObjectItem(location, "long");
    cJSON *timezone_id = cJSON_GetObjectItem(location, "timezone_id");

    /*current_observation*/
    cJSON *current = cJSON_GetObjectItem(root, "current_observation");
    /**wind*/
    cJSON *wind = cJSON_GetObjectItem(current, "wind");
    cJSON *chill = cJSON_GetObjectItem(wind, "chill");
    cJSON *direction = cJSON_GetObjectItem(wind, "direction");
    cJSON *speed = cJSON_GetObjectItem(wind, "speed");
    /**atmosphere*/
    cJSON *atmosphere = cJSON_GetObjectItem(current, "atmosphere");
    cJSON *humidity = cJSON_GetObjectItem(atmosphere, "humidity");
    cJSON *visibility = cJSON_GetObjectItem(atmosphere, "visibility");
    cJSON *pressure = cJSON_GetObjectItem(atmosphere, "pressure");
    cJSON *rising = cJSON_GetObjectItem(atmosphere, "rising");
    /**astronomy*/
    cJSON *astronomy = cJSON_GetObjectItem(current, "astronomy");
    cJSON *sunrise = cJSON_GetObjectItem(astronomy, "sunrise");
    cJSON *sunset = cJSON_GetObjectItem(astronomy, "sunset");
    /**condition*/
    cJSON *condition = cJSON_GetObjectItem(current, "condition");
    cJSON *text = cJSON_GetObjectItem(condition, "text");
    cJSON *code = cJSON_GetObjectItem(condition, "code");
    cJSON *temperature = cJSON_GetObjectItem(condition, "temperature");
    /**pubDate*/
    cJSON *pubDate = cJSON_GetObjectItem(current, "pubDate");

    /*location*/
    info.timeZone = timezone_id->valuestring;
    info.region = region->valuestring;
    info.woeid = woeid->valueint;
    info.country = country->valuestring;
    info.lat = lat->valuedouble;
    info.lon = lon->valuedouble;
    info.timezone_id = timezone_id->valuestring;

    /**wind*/
    info.chill = chill->valueint;
    info.direction = direction->valueint;
    info.speed = speed->valuedouble;

    /**atmosphere*/
    info.humidity = humidity->valueint;
    info.visibility = visibility->valuedouble;
    info.pressure = pressure->valuedouble;
    info.rising = rising->valueint;

    /**astronomy*/
    info.sunrise = sunrise->valuestring;
    info.sunset = sunset->valuestring;

    /**condition*/
    info.text = text->valuestring;
    info.code = code->valueint;
    info.temperature = temperature->valueint;

    /**pubDate*/
    info.pubDate = pubDate->valueint;


    cJSON *forecasts = cJSON_GetObjectItem(root, "forecasts");
    int size =   cJSON_GetArraySize(forecasts);
    for (int i = 0; i < size; ++i) {
        cJSON *item = cJSON_GetArrayItem(forecasts, i);
        info.forecasts[i].day =  cJSON_GetObjectItem(item, "day")->valuestring;
        info.forecasts[i].date = cJSON_GetObjectItem(item, "date")->valueint;
        info.forecasts[i].low =  cJSON_GetObjectItem(item, "low")->valueint;
        info.forecasts[i].high =  cJSON_GetObjectItem(item, "high")->valueint;
        info.forecasts[i].text =   cJSON_GetObjectItem(item, "text")->valuestring;
        info.forecasts[i].code = cJSON_GetObjectItem(item, "code")->valueint;
    }
    cJSON_Delete(root);
#endif
    info.valid = true;
    return true;
}

void YahooWeather::setUnitFormat(bool isImperial)
{
    _isImperial = isImperial;
}

uint8_t YahooWeather::getUnit()
{
    return !_isImperial ? 0 : 1;
}
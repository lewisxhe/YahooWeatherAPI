YahooWeatherAPI
===============

1. Log in with yahoo mailbox, create a weather application, [click here to create](https://developer.yahoo.com/apps/create/)
2. Fill in the application name, Application Type choose Web Application, other parts can be left blank
3. After successful creation, you will get your`App ID`, `Consumer Key`, `Consumer Secret`, and fill in these three in [YahooWeahter.ino](https://github.com/lewisxhe/YahooWeatherAPI/blob/master/examples/YahooWeahter/YahooWeahter.ino) respectively.
4. Upload the **YahooWeahter.ino** to the board and you will see the weather information output by the serial port
5. For more usage, please see [YahooWeahter.ino](https://github.com/lewisxhe/YahooWeatherAPI/blob/master/examples/YahooWeahter/YahooWeahter.ino)
6. For more information, see [Yahoo! Developer](https://developer.yahoo.com/weather/)
7. By default, `cJSON` is used for parsing. No other dependencies need to be installed on the `ESP32` platform. If you need to change to `ArduinoJSON`, you need to comment `ENABLE_CJSON` in `YahooWeatherAPI.h`

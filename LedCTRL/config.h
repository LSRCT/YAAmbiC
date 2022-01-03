#define CONFIG_CTRL_PIN 5     // Pin 5 is D1 on esp8266 dev board
#define CONFIG_NUMPIXELS 144  // 35 right 74 top 35 left

#define CONFIG_WIFI_SSID "wifi"
#define CONFIG_WIFI_PWD "1234"
#define CONFIG_MQTT_BROKER "192.168.178.1"
#define CONFIG_MQTT_PORT 1883

#define CONFIG_UDP_PORT 4210  // local port to listen on for ambient mode

// MQTT Topics
#define CONFIG_MQTT_CLIENTID "ESP8266Client_ambilight"
#define CONFIG_MQTT_TOPIC_STATE "light/ambient1"
#define CONFIG_MQTT_TOPIC_SET "light/ambient1/set"

// default when turning on device
#define CONFIG_DEFAULT_R 255
#define CONFIG_DEFAULT_G 255
#define CONFIG_DEFAULT_B 255
#define CONFIG_DEFAULT_BRIGHTNESS 50

// calibration, set to 255 255 255 for default
#define CONFIG_RGBCALIB_R 255
#define CONFIG_RGBCALIB_G 230
#define CONFIG_RGBCALIB_B 100

#define CONFIG_DEBUG 0
// Arduino side for YAAmbiC
// Significant amount of code for json and mqtt adapted from
// https://github.com/corbanmailloux/esp-mqtt-rgb-led/
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>

#include "config.h"

const int BUFFER_SIZE = JSON_OBJECT_SIZE(20);

// bytes are convenient because rgb values go from 0->255 anyway
byte incomingPacket[CONFIG_NUMPIXELS * 3] = {0};  // buffer for incoming packets

// Maintained state for reporting to HA
byte red = 255;
byte green = 255;
byte blue = 255;
byte brightness = 255;

// Real values to write to the LEDs (ex. including brightness and calibration)
byte realRed = 0;
byte realGreen = 0;
byte realBlue = 0;
bool stateOn = false;
bool effect_ambient = false;

// UDP for ambient light mode
WiFiUDP Udp;
WiFiClient espClient;
// MQTT for normal usage
PubSubClient MQTTclient(espClient);
Adafruit_NeoPixel pixels =
    Adafruit_NeoPixel(CONFIG_NUMPIXELS, CONFIG_CTRL_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  Serial.println();
  // init the led pixels
  pixels.begin();
  pixels.clear();
  setup_rgb_default();
  setup_wifi();
  MQTTclient.setServer(CONFIG_MQTT_BROKER, CONFIG_MQTT_PORT);
  MQTTclient.setCallback(callback_MQTT);
}

void setup_rgb_default() {
  Serial.println("Setting RGB default values...");
  brightness = CONFIG_DEFAULT_BRIGHTNESS;
  set_real_rgb(CONFIG_DEFAULT_R, CONFIG_DEFAULT_G, CONFIG_DEFAULT_B);
  show_LED_uniform();
}

void setup_wifi() {
  Serial.printf("Connecting to %s ", CONFIG_WIFI_SSID);
  WiFi.begin(CONFIG_WIFI_SSID, CONFIG_WIFI_PWD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");
  Udp.begin(CONFIG_UDP_PORT);
  Serial.printf("Now listening at IP %s, UDP port %d\n",
                WiFi.localIP().toString().c_str(), CONFIG_UDP_PORT);
}

void reconnect_MQTT() {
  while (!MQTTclient.connected()) {
    Serial.println("Reconnecting MQTT...");
    if (!MQTTclient.connect(CONFIG_MQTT_CLIENTID)) {
      Serial.print("failed, rc=");
      Serial.print(MQTTclient.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    } else {
      MQTTclient.subscribe(CONFIG_MQTT_TOPIC_SET);
    }
  }
  Serial.println("MQTT Connected...");
}

void callback_MQTT(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char message[length + 1];
  for (unsigned int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);

  if (!processJson(message)) {
    return;
  }

  if (stateOn) {
    // Update lights
    set_real_rgb(red, green, blue);
  } else {
    realRed = 0;
    realGreen = 0;
    realBlue = 0;
  }
  show_LED_uniform();
  sendState();
}

bool processJson(char *message) {
  DynamicJsonDocument root(BUFFER_SIZE);
  DeserializationError error = deserializeJson(root, message);
  if (error) {
    Serial.println("parseObject() failed");
    return false;
  }

  if (root.containsKey("state")) {
    if (strcmp(root["state"], "ON") == 0) {
      stateOn = true;
    } else if (strcmp(root["state"], "OFF") == 0) {
      stateOn = false;
    }
  }
  if (root.containsKey("color")) {
    red = root["color"]["r"];
    green = root["color"]["g"];
    blue = root["color"]["b"];
  }

  if (root.containsKey("brightness")) {
    brightness = root["brightness"];
  }
  if (root.containsKey("effect")) {
    if (strcmp(root["effect"], "ambient") == 0) {
      effect_ambient = true;
    } else {
      effect_ambient = false;
    }
  }
  return true;
}

void sendState() {
  DynamicJsonDocument root(BUFFER_SIZE);
  root["state"] = (stateOn) ? "ON" : "OFF";
  JsonObject color = root.createNestedObject("color");
  color["r"] = red;
  color["g"] = green;
  color["b"] = blue;
  root["brightness"] = brightness;
  root["effect"] = (effect_ambient) ? "ambient" : "rgb";
  char buffer[measureJson(root) + 1];
  serializeJson(root, buffer, sizeof(buffer));
  MQTTclient.publish(CONFIG_MQTT_TOPIC_STATE, buffer, true);
}

void set_real_rgb(byte r, byte g, byte b) {
  // map rgb values to brightness and color calibration
  realRed = map(r, 0, 255, 0, map(brightness, 0, 255, 0, CONFIG_RGBCALIB_R));
  realGreen = map(g, 0, 255, 0, map(brightness, 0, 255, 0, CONFIG_RGBCALIB_G));
  realBlue = map(b, 0, 255, 0, map(brightness, 0, 255, 0, CONFIG_RGBCALIB_B));
}

void show_LED_uniform() {
  // show all LEDS in the current RGB color (realRed, realGreen, realBlue)
  for (int i = 0; i < CONFIG_NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(realRed, realGreen, realBlue));
  }
  pixels.show();
}

void loop() {
  if (!MQTTclient.connected()) {
    reconnect_MQTT();
  }
  MQTTclient.loop();

  // listen for UDP packages when using ambient mode
  if (effect_ambient && stateOn) {
    int packetSize = Udp.parsePacket();
    if (packetSize) {
      // receive incoming UDP packets
      int len = Udp.read(incomingPacket, CONFIG_NUMPIXELS * 3);
      if (CONFIG_DEBUG) {
        Serial.printf("Received %d bytes from %s, port %d\n", packetSize,
                      Udp.remoteIP().toString().c_str(), Udp.remotePort());
        // this is just for printing since we already know how long the packet
        // is (-> n_leds)
        if (len > 0) {
          incomingPacket[len] = 0;
        }
        Serial.printf("UDP packet contents: %s\n", incomingPacket);
      }
      // pixel loop
      for (int i = 0; i < CONFIG_NUMPIXELS; i++) {
        set_real_rgb(incomingPacket[0 + (i * 3)], incomingPacket[1 + (i * 3)],
                     incomingPacket[2 + (i * 3)]);
        pixels.setPixelColor(i, pixels.Color(realRed, realGreen, realBlue));
      }
      pixels.show();  // This sends the updated pixel color to the hardware.
    }
  } else {
    delay(100);
  }
}

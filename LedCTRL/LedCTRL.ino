#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define PIN            5  // Pin 5 is D1 on esp8266 dev board
#define NUMPIXELS      (18 + 32 + 18) // 18 right 32 top 18 left

// TODO somehow dont hardcore this
const char* ssid     = "LSRCT";
const char* password = "83067046472468411597";
unsigned int localUdpPort = 4210;  // local port to listen on
// chars are convenient because rgb values go from 0->255 anyway
unsigned char incomingPacket[255] = {0};  // buffer for incoming packets

WiFiUDP Udp;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  // init the led pixels
  pixels.begin();
  pixels.clear();

  Serial.begin(115200);
  Serial.println();

  // connect to wifi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");
  Udp.begin(localUdpPort);
  Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPort);
}

void loop(){
  int packetSize = Udp.parsePacket();
  if (packetSize)
  {
    // receive incoming UDP packets
    Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());
    // this is just for printing since we already know how long the packet is (-> n_leds)
    int len = Udp.read(incomingPacket, 255);
    if (len > 0)
    {
      incomingPacket[len] = 0;
    }
    Serial.printf("UDP packet contents: %s\n", incomingPacket);
    // pixel loop
    for(int i=0;i<NUMPIXELS;i++){
      pixels.setPixelColor(i, pixels.Color(incomingPacket[0 +(i * 3)], 
                                           incomingPacket[1 +(i * 3)], 
                                           incomingPacket[2 +(i * 3)]));
    }
    pixels.show(); // This sends the updated pixel color to the hardware.
  }
}

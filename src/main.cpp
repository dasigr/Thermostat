/***************************************************
  Adafruit MQTT Library ESP8266 Example

  Must use ESP8266 Arduino from:
    https://github.com/esp8266/Arduino

  Works great with Adafruit's Huzzah ESP board & Feather
  ----> https://www.adafruit.com/product/2471
  ----> https://www.adafruit.com/products/2821

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Tony DiCola for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <DHT.h>

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "STARGATE"
#define WLAN_PASS       "SPACELINK239"

/************************* MQTT Setup *********************************/

#define AIO_SERVER      "192.168.1.4"
#define AIO_SERVERPORT  1883                    // use 8883 for SSL
#define AIO_USERNAME    "admin"
#define AIO_KEY         "admin"

/****************************** DHT Setup ************************************/

#define DHTPIN 2
#define DHTTYPE DHT11

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiClientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Setup feeds for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish mqtt_device = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/room.temperature");

/****************************** DHT Sensor **********************************/

DHT dht(DHTPIN, DHTTYPE, 11);
float hum;  // Humidity
float temp; // Temperature
unsigned long previousMillis = 0;
const long interval = 2000;

/*************************** Sketch Code ************************************/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();

void setup() {
  Serial.begin(115200);
  delay(10);

  Serial.println(F("Adafruit MQTT demo"));

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

  // Setup DHT Sensor.
  dht.begin();
}

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();
  
  // Get current time in milliseconds.
  unsigned long currentMillis = millis();

  // Read temperature every 2 seconds.
  if (currentMillis - previousMillis >= interval) {
    // Save the last time we've read the temperature.
    previousMillis = currentMillis;

    // Read data and store it to variables hum and temp.
    hum = dht.readHumidity();
    temp = dht.readTemperature();

    // Publish sensor value.
    Serial.print(F("\nSending sensor val "));
    char payload[64] = "";
    strcat(payload, "20;02;DKW2012;");

    // Convert temperature and add to payload.
    char temp_value[32];
    snprintf(temp_value, sizeof temp_value, "%f", temp);
    strcat(payload, "TEMP=");
    strcat(payload, temp_value);
    strcat(payload, ";");

    // Convert humidity and add to payload.
    char hum_value[32];
    snprintf(hum_value, sizeof hum_value, "%f", hum);
    strcat(payload, "HUM=");
    strcat(payload, hum_value);
    strcat(payload, ";");

    Serial.print(payload);
    Serial.print("... ");
    if (! mqtt_device.publish(payload)) {
      Serial.println(F("Failed"));
    } else {
      Serial.println(F("OK!"));
    }
  }

  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  /*
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }
  */
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
    retries--;

    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while (1);
    }
  }
  Serial.println("MQTT Connected!");
}

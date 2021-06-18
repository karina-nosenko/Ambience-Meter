/***************************************************
  
  SHENKAR - SMART SYSTEMS
  By: Karina Nosenko
  DATE: May-2021
  
 ****************************************************/
 
#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <Wire.h>
#include "Adafruit_MCP9808.h"
#include <Adafruit_NeoPixel.h>

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "YOUR_WIFI_NAME"
#define WLAN_PASS       "YOUR_WIFI_PASSWORD"

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   
#define AIO_USERNAME    "karina_n"
#define AIO_KEY         "aio_nIjg57K4J6PPA91UlIwabYOWkJ6k"

/**************************** Global State **********************************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Setup a feed called 'sound_sensor' for publishing.
Adafruit_MQTT_Publish sound_sensor = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/sound");

// Setup a feed called 'temperature_sensor' for publishing.
Adafruit_MQTT_Publish temperature_sensor = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperature");

// Setup a feed called 'sound_sensor' for subscribing to changes.
Adafruit_MQTT_Subscribe soundSensor = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/sound");

// Setup a feed called 'temperature_sensor' for subscribing to changes.
Adafruit_MQTT_Subscribe temperatureSensor = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/temperature");

/*************************** Sketch Code ************************************/

#define soundPIN 37   // Pin of sound sensor
#define rgbPIN  10    // Pin of NeoPixels
#define NUMPIXELS 14  // Number of attached NeoPixels
#define DELAYVAL 100  // Time (in milliseconds) to pause between pixels


// When setting up the NeoPixel library, we tell it how many pixels,
// and which pin to use to send signals
Adafruit_NeoPixel pixels(NUMPIXELS, rgbPIN, NEO_GRB + NEO_KHZ800);

// Create the MCP9808 temperature sensor object
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();

void MQTT_connect();


void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("Starting..."));
  delay(1000);
  Serial.println(F("\n\n##################################"));
  Serial.println(F("Adafruit MQTT demo - SHENKAR"));
  Serial.println(F("##################################"));
  
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
  Serial.println();

  mqtt.subscribe(&soundSensor);           // Setup MQTT subscription for sound_sensor feed.
  mqtt.subscribe(&temperatureSensor);     // Setup MQTT subscription for temperature_sensor feed.

  //Setup the temperature sensor
  Wire.begin();
  if (!tempsensor.begin(0x18)) {              // 0x1C Address For MANTA Temperature Sensor
    Serial.println("Couldn't find MCP9808!");
    while (1);
  }

  //Setup the sound sensor
  pinMode (soundPIN, INPUT);

  //Setup NeoPixel strip object
  pixels.begin();

}

int red=0, green = 0, blue = 0;
int delaySound = 0;


void loop() {
  
  MQTT_connect();

  Adafruit_MQTT_Subscribe *subscription;

  // Read the value from the temperature sensor and publish it
  float tempValue = tempsensor.readTempC();
  
  Serial.print(F("\nTemperature value: "));
  Serial.print(tempValue);
  Serial.print("...");
  if (! temperature_sensor.publish(tempValue)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }

  // Read the last temperature value from Adafruit
  char* lastTemperatureValue;
  float lastTemperature;
  while ((subscription = mqtt.readSubscription(2000))) {
    if (subscription == &temperatureSensor) {
      Serial.print(F("Got temperature: "));
      lastTemperatureValue = (char *)temperatureSensor.lastread;
      Serial.println(lastTemperatureValue);
    }
  }
  lastTemperature = atof(lastTemperatureValue);

  delay(1000);

  // Read the value from the sound sensor and publish it
  float soundValue = analogRead(soundPIN);
  
  Serial.print(F("\nSound value: "));
  Serial.print(soundValue);
  Serial.print("...");
  if (! sound_sensor.publish(soundValue)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }

   // Read the last sound value from Adafruit
  char* lastSoundValue;
  float lastSound;
  while ((subscription = mqtt.readSubscription(2000))) {
    if (subscription == &soundSensor) {
      Serial.print(F("Got sound: "));
      lastSoundValue = (char *)soundSensor.lastread;
      Serial.println(lastSoundValue);
    }
  } 
  lastSound = atof(lastSoundValue);

  //delay(2000);

  pixels.clear(); // Set all pixel colors to 'off'

  //set the red color
  red = (lastTemperature*10-140);
  if(red > 255) red = 255;
  else if(red < 0) red = 0;

  //set the blue color
  blue = 0;
  if(lastTemperature*10 <= 250) {
    blue = 255 - red;
  }

  //set the green color
  green = 255-red;
  if(((red-blue)<10 && (red-blue)>=0) || ((blue-red)<10 && (blue-red)>=0)){ //if there is a small difference between blue and red - "normal" temperature - add green
    green = 255 - red;
    
  }

  //set the delay time - depends on the sound input
  if(lastSound<105) delaySound = 500;
  else  delaySound = 200;

  //define the way to show the pixels
  if(lastSound < 104) pulseColor(delaySound, red, green, blue);
  else if (lastSound>=104 && lastSound<=106)  runPixels(delaySound, red, green, blue);
  else if (lastSound>106) runPixels(50, red, green, blue);
}


//Display all the pixels one after another
void runPixels(uint8_t wait, int red, int green, int blue) {
    for(int i=0; i<NUMPIXELS; i++) {
    
    pixels.setPixelColor(i, pixels.Color(red, green, blue));
    pixels.show();

    delay(wait);
    
    digitalWrite(rgbPIN, HIGH);
  }
}


//Gradually fade in and fade out the leds
void pulseColor(uint8_t wait, int red, int green, int blue) {

  int range = 0;  //range defines the maximum value that will be sent to the setPixelColor()
  if (red>green) {
    if(red>blue) {
      range=red;
    }
    else {
      range=blue;
    }
  }
  else {
    if(green>blue) {
      range=green;
    }
    else {
      range=blue;
    }
  }

  //fade out
  for(int i=1; i<range; i++) {
    for(int j=0; j<NUMPIXELS; j++) {  //fill the whole strip
       pixels.setPixelColor(j, pixels.Color(red/i, green/i, blue/i));
       pixels.show(); 
       digitalWrite(rgbPIN, HIGH);
    }
    delay(10);  
  }

  //fade in
  for(int i=range; i>0; i--) {
    for(int j=0; j<NUMPIXELS; j++) {  //fill the whole strip
       pixels.setPixelColor(j, pixels.Color(red/i, green/i, blue/i));
       pixels.show(); 
       digitalWrite(rgbPIN, HIGH);
    }
    delay(10);  
  }

  pixels.clear(); // Set all pixel colors to 'off'

  //display all the pixels for a 3 seconds
  for(int j=0; j<NUMPIXELS; j++) {  //fill the whole strip
       pixels.setPixelColor(j, pixels.Color(red, green, blue));
       pixels.show(); 
       digitalWrite(rgbPIN, HIGH);
   }

  delay(3000);
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
       Serial.println("Retrying MQTT connection");
       mqtt.disconnect();
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}

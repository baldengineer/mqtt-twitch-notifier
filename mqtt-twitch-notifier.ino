// *************************************
// bald-hw-twitch-notifier v2
// mqtt-twitch-notifier
// created by James Lewis, 2021
//
// Notifier that plays MP3 sound effects based
// on MQTT messages received.
//
// Used during Bald Engineer Twitch live streams.
// https://twitch.tv/baldengineer
//
// Additional Attributions on the bottom

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include "SparkFun_Qwiic_MP3_Trigger_Arduino_Library.h" 
#include "config.h"  // edit config.sample and rename to config.h
#include <Wire.h>

#define LED_PIN    14
#define LED_COUNT 8

// ESP8266 WiFi
WiFiClientSecure espClient;    // oooo secure!

// PubSubClient
PubSubClient client(espClient);
unsigned long lastMsg = 0;
const byte MSG_BUFFER_SIZE = 50;
char msg[MSG_BUFFER_SIZE];
int value = 0;
int previous_mqtt_status = 0;

// NeoPixel
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Spark Fun Qwiic MP3
// http://librarymanager/All#SparkFun_MP3_Trigger
MP3TRIGGER mp3;
byte mp3_volume = 18;
const byte i2c_SCL = 5;
const byte i2c_SDA = 4;

// Countdown timer for resetting visual
unsigned long previous_indicator_millis = millis();
unsigned long indicator_interval = 2000;
bool arm_indicator_countdown = false;

// Other Variables
unsigned long previous_millis = 0;

void moar_random() {
  float crazy_seed = analogRead(0) * analogRead(0);
  delay(50);
  crazy_seed = crazy_seed * analogRead(0);
  unsigned long less_crazy_seed = crazy_seed; // truncate the heck out of it.
  randomSeed(less_crazy_seed);
}

void color_code_status(int status) {
  if (status != previous_mqtt_status) {
    switch (status) {

      case 1: //network disconnected
        colorWipe(strip.Color(0, 0, 0), 10);
        strip.setBrightness(10);
        colorWipe(strip.Color(255, 0, 0), 10);
        //Mytone.Tone_Down(buzzer_pin);
        break;

      case 11: // MQTT disconnected
        colorWipe(strip.Color(0, 0, 0), 10);
        strip.setBrightness(10);
        colorWipe(strip.Color(255, 255, 0), 10);
        //   Mytone.Ringer(buzzer_pin);
        break;

      case 21: // connected?
        colorWipe(strip.Color(0, 0, 0), 10);
        strip.setBrightness(10);
        colorWipe(strip.Color(0, 255, 0), 10);
        //      Mytone.Tone_Up(buzzer_pin);
        break;
    }
    previous_mqtt_status = status;

  }
}

// random pixels
void randomWipe(int wait) {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(random(256), random(256), random(256)));
    strip.show();
    delay(wait);
  }
}

// fills in the pixels
void colorWipe(uint32_t color, int wait) {
  for (int i = 0; i < strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
  }
}

void setup_wifi() {
  delay(10);
  Serial.print("\nConnecting to '"); Serial.print(WIFI_SSID); Serial.println("'");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// MQTT PubSub callback
void callback(char* topic, byte* payload, unsigned int length) {
  String topic_str = topic;

  Serial.print("Message arrived [");
  Serial.print(topic_str);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if (topic_str == "stream/twitch-attn-indi") {
    Serial.println("Processing stream/twitch-attn-indi");
    if ((char)payload[0] == '1') {
      mp3Play(2);
      arm_indicator_countdown = true;
      previous_indicator_millis = millis();
    }
  } else {
    mp3Play(3);
  }
}

/*
** Possible values for client.state()
  #define MQTT_CONNECTION_TIMEOUT     -4
  #define MQTT_CONNECTION_LOST        -3
  #define MQTT_CONNECT_FAILED         -2
  #define MQTT_DISCONNECTED           -1
  #define MQTT_CONNECTED               0
  #define MQTT_CONNECT_BAD_PROTOCOL    1
  #define MQTT_CONNECT_BAD_CLIENT_ID   2
  #define MQTT_CONNECT_UNAVAILABLE     3
  #define MQTT_CONNECT_BAD_CREDENTIALS 4
  #define MQTT_CONNECT_UNAUTHORIZED    5
**
*/

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "notifier-";
    clientId += String(random(0xffff), HEX);

    // Attempt to connect
    if (client.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("connected");

      // MQTT topics for subscription
      client.subscribe("stream/twitch-attn-indi");
      client.subscribe("stream/dispense-treat-toggle");
      client.subscribe("stream/channel-raid");
      client.subscribe("stream/channel-cheer");

      Serial.println("subscribed!");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void mp3Status() {
  Serial.print("Status: "); Serial.println(mp3.getStatus());
  int stat = mp3.getStatus();
  switch (stat) {
    case 0:
      Serial.println("OK");
      break;
    case 1:
      Serial.println("Fail");
      break;
    case 2:
      Serial.println("NO File");
      break;
    case 5:
      Serial.println("SD Error");
      break;
    case 255:
      Serial.println("255 ERROR!");
      break;
    default:
      Serial.print("Unknown MP3 error: ");
      Serial.println(stat);
  }
}

void mp3Play(int track) {
  /*
    F001.mp3 - Dialing
    F002.mp3 - Connecting
    F003.mp3 - AddOhms VTR
  */
  Serial.print("Playing track #"); Serial.println(track);
  mp3.playFile(track);
  mp3Status();
}

void setup() {
  // start the serial connection
  Serial.begin(115200);

  // Setup NeoPixel Strip
  strip.begin();
  strip.show();
  strip.setBrightness(10); // Set BRIGHTNESS (max = 255)
  colorWipe(strip.Color(255,  0,  0), 50);

  // For Sparkfun MP3
  Wire.begin(i2c_SDA, i2c_SCL);
  if (mp3.begin() == false) {
    Serial.println("\n\nQuiic MP3 failed.");
  } else {
    mp3.setVolume(mp3_volume);
    mp3Play(1);
  }

  // Connect to WiFi
  setup_wifi();

  // Connect to MQTT Broker
  espClient.setFingerprint(fingerprint); // when you do know it
  client.setServer(MQTT_SERVER, MQTT_SERVERPORT);
  client.setCallback(callback);
  mp3Play(2);
}

void handleSerial() {
  // double check there are characters ready
  if (Serial.available() == 0)
    return;

  byte incomingChar = Serial.read();

  switch (incomingChar) {
    case '+':
      mp3_volume++;
      if (mp3_volume >= 32)
        mp3_volume = 31;
      mp3.setVolume(mp3_volume);
      Serial.print("Volume: "); Serial.println(mp3_volume);
      break;

    case '-':
      mp3_volume --;
      if (mp3_volume <= 0)
        mp3_volume = 1;
      mp3.setVolume(mp3_volume);
      Serial.print("Volume: "); Serial.println(mp3_volume);
      break;

    case '1':
      mp3Play(1);
      break;

    case '2':
      mp3Play(2);
      break;

    case '3':
      mp3Play(3);
      break;
  }
}

void handleLEDs() {
  // Old code for lights, need to update
  if (arm_indicator_countdown) {
    //    colorWipe(strip.Color(random(256),random(256),random(256)),50);
    // randomWipe(50);
    colorWipe(strip.Color(0, 0, 255), 100);
    // int rnd = random(0,4);
    int rnd = 4;
    colorWipe(strip.Color(0, 255, 0), 100);
  }

  if ((arm_indicator_countdown) && (millis() - previous_indicator_millis >= indicator_interval)) {
    // we received a 1 and decided to do something for a while
    // but that time is over
    arm_indicator_countdown = false; // stop checking if it is time to clear
    //  indicator->save(0); // turn off the LEDs through MQTT
    snprintf(msg, MSG_BUFFER_SIZE, "0");
    client.publish("twitch-attn-indi", msg);
  }
}

void loop() {
  // Stay Connected
  if (!client.connected())
    reconnect();

  // Needed to keep MQTT alive
  client.loop();
  // color_code_status(io.status());

  // See if there are any serial commands
  while (Serial.available()) handleSerial();

  // Update LEDs, if necessary
  handleLEDs();
}


// MQTT based on PubSubclient Examples and self-hosted MQTT Broker
// LED Animations based Adafruit's NeoPixel Examples
//
// Hardware Used:
// - Adafruit Huzzah ESP8266 Feather
// - Sparkfun's Quiic MP3 Player Board

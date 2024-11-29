// Include necessary libraries
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <HTTPClient.h>
#include <SHT21.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>      // For parsing JSON data from Telegram
#include <Preferences.h>      // For storing settings

#include "config.h"           // Include settings from a separate file

// OLED display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1  // Default value
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// DS18B20 settings
#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);

// SHT21 settings
SHT21 sht21;

// PMS5003 settings
#define PMS_RX_PIN 19
HardwareSerial pmsSerial(2);  // Use the second UART port

// Pins for RGB LED
#define RED_PIN    16
#define GREEN_PIN  17
#define BLUE_PIN    5

// ThingSpeak settings
const char* thingSpeakServer = "api.thingspeak.com";  // Renamed variable to avoid conflict
String apiKey = THINGSPEAK_API_KEY;      // Use setting from config.h

// Telegram Bot settings
const char* telegramToken = TELEGRAM_TOKEN;  // Use setting from config.h
const char* chatId = TELEGRAM_CHAT_ID;       // Use setting from config.h
WiFiClientSecure client;
UniversalTelegramBot bot(telegramToken, client);

// Variables for Telegram Bot
unsigned long bot_lasttime = 0;             // Time of the last bot request
const unsigned long BOT_MTBS = 1000;        // Interval between message checks (in milliseconds)

// Variables for storing data
float ds18b20Temp = 0.0;
float sht21Temp = 0.0;
float sht21Humidity = 0.0;
int pm1_0 = 0;
int pm2_5 = 0;
int pm10 = 0;

// Time variables
unsigned long previousMillis = 0;
unsigned long interval = 300000;  // 5 minutes in milliseconds
const unsigned long MIN_INTERVAL = 300000; // Minimum interval of 5 minutes

// Variables for LED blinking
unsigned long previousLedMillis = 0;
int ledInterval = 0;

// PM2.5 threshold variables
int pm2_5_low = 12;
int pm2_5_medium = 35;

// Flag for displaying debug information on OLED
bool showDebugInfo = true;

// Preferences object for storing settings
Preferences preferences;

// Variable for storing the ESP32 IP address
String deviceIP = "Unknown";

void setup() {
  Serial.begin(115200);

  // Initialize DS18B20
  ds18b20.begin();

  // Initialize I2C
  Wire.begin();

  // Initialize SHT21
  sht21.begin();

  // Initialize PMS5003
  pmsSerial.begin(9600, SERIAL_8N1, PMS_RX_PIN, -1);
  delay(1000);  // Delay for stabilization
  setPMS5003PassiveMode();

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Display initialization error");
    while (true);
  }
  display.setRotation(2); // Rotate the display 180 degrees
  display.clearDisplay();
  display.display();

  // Initialize RGB LED pins
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  // Connect to Wi-Fi
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected");

  // Get and store IP address
  deviceIP = WiFi.localIP().toString();
  Serial.print("ESP32 IP Address: ");
  Serial.println(deviceIP);

  // Configure client for Telegram Bot
  client.setInsecure();

  // Initialize Preferences
  preferences.begin("settings", false); // Namespace "settings", read-write mode
  loadSettings();

  // Removed code related to OTA updates

  Serial.println("Setup completed");
}

void loop() {
  readDS18B20();
  readSHT21();
  readPMS5003();
  blinkRGBLed(pm2_5);
  displayData();

  // Send data to ThingSpeak every 'interval' milliseconds
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    Serial.println("Sending data to ThingSpeak. Current time: " + String(currentMillis));
    previousMillis = currentMillis;
    sendDataToThingSpeak();
  }

  // Check for new messages on Telegram
  if (WiFi.status() == WL_CONNECTED) {
    if (millis() - bot_lasttime > BOT_MTBS) {
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      while (numNewMessages) {
        handleNewMessages(numNewMessages);
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      }
      bot_lasttime = millis();
    }
  }
}

void readDS18B20() {
  ds18b20.requestTemperatures();
  ds18b20Temp = ds18b20.getTempCByIndex(0);
  Serial.print("DS18B20 Temperature: ");
  Serial.println(ds18b20Temp);
}

void readSHT21() {
  sht21Temp = sht21.getTemperature();
  sht21Humidity = sht21.getHumidity();
  Serial.print("SHT21 Temperature: ");
  Serial.println(sht21Temp);
  Serial.print("SHT21 Humidity: ");
  Serial.println(sht21Humidity);
}

void readPMS5003() {
  // Clear buffer before request
  while (pmsSerial.available()) {
    pmsSerial.read();
  }

  requestPMSData();

  // Wait for data with timeout
  unsigned long startTime = millis();
  while (pmsSerial.available() < 32) {
    if (millis() - startTime > 1000) { // Timeout 1 second
      Serial.println("PMS5003 read error: No data from PMS5003");
      return;
    }
    delay(10);
  }

  // Read data
  uint8_t buffer[32];
  pmsSerial.readBytes(buffer, 32);

  // Check start bytes
  if (buffer[0] == 0x42 && buffer[1] == 0x4D) {
    pm1_0 = buffer[10] << 8 | buffer[11];
    pm2_5 = buffer[12] << 8 | buffer[13];
    pm10 = buffer[14] << 8 | buffer[15];
    Serial.print("PM1.0: ");
    Serial.print(pm1_0);
    Serial.print(", PM2.5: ");
    Serial.print(pm2_5);
    Serial.print(", PM10: ");
    Serial.println(pm10);
  } else {
    Serial.println("PMS5003 read error: Incorrect start bytes");
  }
}

void blinkRGBLed(int pm2_5_value) {
  unsigned long currentMillis = millis();

  if (currentMillis - previousLedMillis >= ledInterval) {
    previousLedMillis = currentMillis;

    // Generate a random blink interval between 30 and 50 ms
    ledInterval = random(30, 50);

    // Toggle LED state
    static bool ledState = false;
    ledState = !ledState;

    if (pm2_5_value <= pm2_5_low) {
      // Green
      digitalWrite(RED_PIN, LOW);
      digitalWrite(GREEN_PIN, ledState ? HIGH : LOW);
      digitalWrite(BLUE_PIN, LOW);
    } else if (pm2_5_value > pm2_5_low && pm2_5_value <= pm2_5_medium) {
      // Blue
      digitalWrite(RED_PIN, LOW);
      digitalWrite(GREEN_PIN, LOW);
      digitalWrite(BLUE_PIN, ledState ? HIGH : LOW);
    } else {
      // Red
      digitalWrite(RED_PIN, ledState ? HIGH : LOW);
      digitalWrite(GREEN_PIN, LOW);
      digitalWrite(BLUE_PIN, LOW);
    }
  }
}

void sendDataToThingSpeak() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String("http://") + thingSpeakServer + "/update?api_key=" + apiKey +
                 "&field1=" + String(ds18b20Temp) +
                 "&field2=" + String(sht21Temp) +
                 "&field3=" + String(sht21Humidity) +
                 "&field4=" + String(pm1_0) +
                 "&field5=" + String(pm2_5) +
                 "&field6=" + String(pm10);
    http.begin(url);
    int httpCode = http.GET();
    Serial.print("ThingSpeak response: ");
    Serial.println(httpCode);
    http.end();
  } else {
    Serial.println("Error: Wi-Fi not connected");
  }
}

void displayData() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print("DS18B20 Temp: ");
  display.print(ds18b20Temp);
  display.println(" C");

  display.print("SHT21 Temp: ");
  display.print(sht21Temp);
  display.println(" C");

  display.print("SHT21 Hum: ");
  display.print(sht21Humidity);
  display.println(" %");

  display.print("PM1.0: ");
  display.print(pm1_0);
  display.println(" ug/m3");

  display.print("PM2.5: ");
  display.print(pm2_5);
  display.println(" ug/m3");

  display.print("PM10: ");
  display.print(pm10);
  display.println(" ug/m3");

  if (showDebugInfo) {
    // Display IP address
    display.print("IP: ");
    display.println(deviceIP);
  }

  display.display();
}

void setPMS5003PassiveMode() {
  uint8_t cmd[] = {0x42, 0x4D, 0xE1, 0x00, 0x00, 0x01, 0x70};
  pmsSerial.write(cmd, sizeof(cmd));
}

void requestPMSData() {
  uint8_t cmd[] = {0x42, 0x4D, 0xE2, 0x00, 0x00, 0x01, 0x71};
  pmsSerial.write(cmd, sizeof(cmd));
}

void handleNewMessages(int numNewMessages) {
  Serial.println("New messages received");
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;

    if (chat_id != String(chatId)) {
      bot.sendMessage(chat_id, "You do not have access to this bot.", "");
      continue;
    }

    if (text == "/start") {
      String welcome = "Hello, " + from_name + ".\n";
      welcome += "Use the /status command to get current data.";
      bot.sendMessage(chat_id, welcome, "");
    }
    else if (text == "/status") {
      String message = "Current air quality data:\n";
      message += "DS18B20 Temp: " + String(ds18b20Temp) + " C\n";
      message += "SHT21 Temp: " + String(sht21Temp) + " C\n";
      message += "SHT21 Hum: " + String(sht21Humidity) + " %\n";
      message += "PM1.0: " + String(pm1_0) + " ug/m3\n";
      message += "PM2.5: " + String(pm2_5) + " ug/m3\n";
      message += "PM10: " + String(pm10) + " ug/m3\n";
      bot.sendMessage(chat_id, message, "");
    }
    else if (text.startsWith("/setapikey ")) {
      String newApiKey = text.substring(11);
      apiKey = newApiKey;
      preferences.putString("apiKey", apiKey);
      bot.sendMessage(chat_id, "ThingSpeak API Key updated.", "");
      Serial.println("ThingSpeak API Key updated via bot");
    }
    else if (text.startsWith("/setinterval ")) {
      String intervalStr = text.substring(13);
      unsigned long newInterval = intervalStr.toInt() * 1000; // Convert seconds to milliseconds
      if (newInterval >= MIN_INTERVAL) {
        interval = newInterval;
        preferences.putULong("interval", interval);
        bot.sendMessage(chat_id, "Update interval set to " + String(interval / 1000) + " seconds.", "");
      } else {
        bot.sendMessage(chat_id, "Interval must be at least " + String(MIN_INTERVAL / 1000) + " seconds.", "");
      }
    }
    else if (text.startsWith("/setthreshold ")) {
      // Expected format: /setthreshold <low> <medium>
      int firstSpace = text.indexOf(' ');
      int secondSpace = text.indexOf(' ', firstSpace + 1);
      if (secondSpace != -1) {
        String lowStr = text.substring(firstSpace + 1, secondSpace);
        String mediumStr = text.substring(secondSpace + 1);
        int newLow = lowStr.toInt();
        int newMedium = mediumStr.toInt();
        if (newLow > 0 && newMedium > newLow) {
          pm2_5_low = newLow;
          pm2_5_medium = newMedium;
          preferences.putInt("pm2_5_low", pm2_5_low);
          preferences.putInt("pm2_5_medium", pm2_5_medium);
          bot.sendMessage(chat_id, "PM2.5 thresholds set: low = " + String(pm2_5_low) + ", medium = " + String(pm2_5_medium), "");
          Serial.println("PM2.5 thresholds updated via bot");
        } else {
          bot.sendMessage(chat_id, "Invalid threshold values. Ensure that medium threshold is greater than low and both are positive.", "");
        }
      } else {
        bot.sendMessage(chat_id, "Invalid command format. Use: /setthreshold <low> <medium>", "");
      }
    }
    else if (text == "/getsettings") {
      String settings = "Current settings:\n";
      settings += "ThingSpeak API Key: " + apiKey + "\n";
      settings += "Update interval: " + String(interval / 1000) + " seconds\n";
      settings += "PM2.5 Thresholds: low = " + String(pm2_5_low) + ", medium = " + String(pm2_5_medium) + "\n";
      settings += "Debug info on OLED: " + String(showDebugInfo ? "Enabled" : "Disabled") + "\n";
      bot.sendMessage(chat_id, settings, "");
    }
    else if (text == "/toggledebug") {
      showDebugInfo = !showDebugInfo;
      preferences.putBool("showDebugInfo", showDebugInfo);
      bot.sendMessage(chat_id, "Debug information on OLED " + String(showDebugInfo ? "enabled." : "disabled."), "");
      Serial.println("Debug information on OLED " + String(showDebugInfo ? "enabled via bot" : "disabled via bot"));
    }
    else {
      bot.sendMessage(chat_id, "Unknown command. Use /status, /setapikey, /setinterval, /setthreshold, /getsettings, or /toggledebug.", "");
    }
  }
}

void loadSettings() {
  apiKey = preferences.getString("apiKey", apiKey);
  interval = preferences.getULong("interval", interval);

  // Set minimum interval
  if (interval < MIN_INTERVAL) {
    interval = MIN_INTERVAL;
  }

  pm2_5_low = preferences.getInt("pm2_5_low", pm2_5_low);
  pm2_5_medium = preferences.getInt("pm2_5_medium", pm2_5_medium);
  showDebugInfo = preferences.getBool("showDebugInfo", showDebugInfo);

  Serial.println("Settings loaded:");
  Serial.println("API Key: " + apiKey);
  Serial.println("Interval: " + String(interval / 1000) + " seconds");
  Serial.println("PM2.5 Thresholds: low = " + String(pm2_5_low) + ", medium = " + String(pm2_5_medium));
}

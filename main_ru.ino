// Подключение необходимых библиотек
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
#include <ArduinoJson.h>      // Для парсинга JSON данных из Telegram
#include <Preferences.h>      // Для хранения настроек

#include "config.h"           // Включение настроек из отдельного файла

// Настройки OLED-дисплея
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1  // Используем по умолчанию
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Настройки DS18B20
#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);

// Настройки SHT21
SHT21 sht21;

// Настройки PMS5003
#define PMS_RX_PIN 19
HardwareSerial pmsSerial(2);  // Используем второй порт UART

// Пины для RGB-светодиода
#define RED_PIN    16
#define GREEN_PIN  17
#define BLUE_PIN    5

// Настройки ThingSpeak
const char* thingSpeakServer = "api.thingspeak.com";  // Переименовали переменную для избежания конфликта
String apiKey = THINGSPEAK_API_KEY;      // Используем настройку из config.h

// Настройки Telegram Bot
const char* telegramToken = TELEGRAM_TOKEN;  // Используем настройку из config.h
const char* chatId = TELEGRAM_CHAT_ID;        // Используем настройку из config.h
WiFiClientSecure client;
UniversalTelegramBot bot(telegramToken, client);

// Переменные для Telegram Bot
unsigned long bot_lasttime = 0;             // Время последнего запроса к боту
const unsigned long BOT_MTBS = 1000;        // Интервал между проверками сообщений (в миллисекундах)

// Переменные для хранения данных
float ds18b20Temp = 0.0;
float sht21Temp = 0.0;
float sht21Humidity = 0.0;
int pm1_0 = 0;
int pm2_5 = 0;
int pm10 = 0;

// Переменные времени
unsigned long previousMillis = 0;
unsigned long interval = 300000;  // 5 минут в миллисекундах
const unsigned long MIN_INTERVAL = 300000; // Минимальный интервал 5 минут

// Переменные для мигания светодиода
unsigned long previousLedMillis = 0;
int ledInterval = 0;

// Переменные порогов PM2.5
int pm2_5_low = 12;
int pm2_5_medium = 35;

// Флаг отображения отладочной информации на OLED
bool showDebugInfo = true;

// Объект Preferences для хранения настроек
Preferences preferences;

// Переменная для хранения IP-адреса ESP32
String deviceIP = "Неизвестно";

void setup() {
  Serial.begin(115200);

  // Инициализация DS18B20
  ds18b20.begin();

  // Инициализация I2C
  Wire.begin();

  // Инициализация SHT21
  sht21.begin();

  // Инициализация PMS5003
  pmsSerial.begin(9600, SERIAL_8N1, PMS_RX_PIN, -1);
  delay(1000);  // Задержка для стабилизации
  setPMS5003PassiveMode();

  // Инициализация OLED-дисплея
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Ошибка инициализации дисплея");
    while (true);
  }
  display.setRotation(2); // Переворачиваем дисплей на 180 градусов
  display.clearDisplay();
  display.display();

  // Инициализация пинов RGB-светодиода
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  // Подключение к Wi-Fi
  Serial.print("Подключение к Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Подключено");

  // Получение и сохранение IP-адреса
  deviceIP = WiFi.localIP().toString();
  Serial.print("IP-адрес ESP32: ");
  Serial.println(deviceIP);

  // Настройка клиента для Telegram Bot
  client.setInsecure();

  // Инициализация Preferences
  preferences.begin("settings", false); // Неймспейс "settings", режим чтение-запись
  loadSettings();

  // Удалили код, связанный с OTA обновлением

  Serial.println("Настройка завершена");
}

void loop() {
  readDS18B20();
  readSHT21();
  readPMS5003();
  blinkRGBLed(pm2_5);
  displayData();

  // Отправка данных на ThingSpeak каждые interval миллисекунд
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    Serial.println("Отправка данных на ThingSpeak. Текущее время: " + String(currentMillis));
    previousMillis = currentMillis;
    sendDataToThingSpeak();
  }

  // Проверка новых сообщений в Telegram
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
  Serial.print("DS18B20 Температура: ");
  Serial.println(ds18b20Temp);
}

void readSHT21() {
  sht21Temp = sht21.getTemperature();
  sht21Humidity = sht21.getHumidity();
  Serial.print("SHT21 Температура: ");
  Serial.println(sht21Temp);
  Serial.print("SHT21 Влажность: ");
  Serial.println(sht21Humidity);
}

void readPMS5003() {
  // Очищаем буфер перед запросом
  while (pmsSerial.available()) {
    pmsSerial.read();
  }

  requestPMSData();

  // Ожидаем данные с таймаутом
  unsigned long startTime = millis();
  while (pmsSerial.available() < 32) {
    if (millis() - startTime > 1000) { // Таймаут 1 секунда
      Serial.println("Ошибка чтения PMS5003: Нет данных от PMS5003");
      return;
    }
    delay(10);
  }

  // Считываем данные
  uint8_t buffer[32];
  pmsSerial.readBytes(buffer, 32);

  // Проверяем начальные байты
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
    Serial.println("Ошибка чтения PMS5003: Некорректные начальные байты");
  }
}

void blinkRGBLed(int pm2_5_value) {
  unsigned long currentMillis = millis();

  if (currentMillis - previousLedMillis >= ledInterval) {
    previousLedMillis = currentMillis;

    // Генерируем случайный интервал мигания от 30 до 50 мс
    ledInterval = random(30, 50);

    // Переключаем состояние светодиода
    static bool ledState = false;
    ledState = !ledState;

    if (pm2_5_value <= pm2_5_low) {
      // Зеленый
      digitalWrite(RED_PIN, LOW);
      digitalWrite(GREEN_PIN, ledState ? HIGH : LOW);
      digitalWrite(BLUE_PIN, LOW);
    } else if (pm2_5_value > pm2_5_low && pm2_5_value <= pm2_5_medium) {
      // Синий
      digitalWrite(RED_PIN, LOW);
      digitalWrite(GREEN_PIN, LOW);
      digitalWrite(BLUE_PIN, ledState ? HIGH : LOW);
    } else {
      // Красный
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
    Serial.print("Ответ ThingSpeak: ");
    Serial.println(httpCode);
    http.end();
  } else {
    Serial.println("Ошибка: Wi-Fi не подключен");
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
    // Отображение IP-адреса
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
  Serial.println("Получены новые сообщения");
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;

    if (chat_id != String(chatId)) {
      bot.sendMessage(chat_id, "У вас нет доступа к этому боту.", "");
      continue;
    }

    if (text == "/start") {
      String welcome = "Здравствуйте, " + from_name + ".\n";
      welcome += "Используйте команду /status для получения текущих данных.";
      bot.sendMessage(chat_id, welcome, "");
    }
    else if (text == "/status") {
      String message = "Текущие данные качества воздуха:\n";
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
      bot.sendMessage(chat_id, "ThingSpeak API Key обновлен.", "");
      Serial.println("ThingSpeak API Key обновлен через бот");
    }
    else if (text.startsWith("/setinterval ")) {
      String intervalStr = text.substring(13);
      unsigned long newInterval = intervalStr.toInt() * 1000; // Переводим секунды в миллисекунды
      if (newInterval >= MIN_INTERVAL) {
        interval = newInterval;
        preferences.putULong("interval", interval);
        bot.sendMessage(chat_id, "Интервал обновления установлен на " + String(interval / 1000) + " секунд.", "");
      } else {
        bot.sendMessage(chat_id, "Интервал должен быть не менее " + String(MIN_INTERVAL / 1000) + " секунд.", "");
      }
    }
    else if (text.startsWith("/setthreshold ")) {
      // Ожидаем формат: /setthreshold <низкий> <средний>
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
          bot.sendMessage(chat_id, "Пороговые значения PM2.5 установлены: низкий = " + String(pm2_5_low) + ", средний = " + String(pm2_5_medium), "");
          Serial.println("Пороговые значения PM2.5 обновлены через бот");
        } else {
          bot.sendMessage(chat_id, "Неверные пороговые значения. Убедитесь, что средний порог больше низкого и оба положительные.", "");
        }
      } else {
        bot.sendMessage(chat_id, "Неверный формат команды. Используйте: /setthreshold <низкий> <средний>", "");
      }
    }
    else if (text == "/getsettings") {
      String settings = "Текущие настройки:\n";
      settings += "ThingSpeak API Key: " + apiKey + "\n";
      settings += "Интервал обновления: " + String(interval / 1000) + " секунд\n";
      settings += "Порог PM2.5: низкий = " + String(pm2_5_low) + ", средний = " + String(pm2_5_medium) + "\n";
      settings += "Отладочная информация на OLED: " + String(showDebugInfo ? "Включена" : "Выключена") + "\n";
      bot.sendMessage(chat_id, settings, "");
    }
    else if (text == "/toggledebug") {
      showDebugInfo = !showDebugInfo;
      preferences.putBool("showDebugInfo", showDebugInfo);
      bot.sendMessage(chat_id, "Отладочная информация на OLED " + String(showDebugInfo ? "включена." : "выключена."), "");
      Serial.println("Отладочная информация на OLED " + String(showDebugInfo ? "включена через бот" : "выключена через бот"));
    }
    else {
      bot.sendMessage(chat_id, "Неизвестная команда. Используйте /status, /setapikey, /setinterval, /setthreshold, /getsettings или /toggledebug.", "");
    }
  }
}

void loadSettings() {
  apiKey = preferences.getString("apiKey", apiKey);
  interval = preferences.getULong("interval", interval);

  // Устанавливаем минимальный интервал
  if (interval < MIN_INTERVAL) {
    interval = MIN_INTERVAL;
  }

  pm2_5_low = preferences.getInt("pm2_5_low", pm2_5_low);
  pm2_5_medium = preferences.getInt("pm2_5_medium", pm2_5_medium);
  showDebugInfo = preferences.getBool("showDebugInfo", showDebugInfo);

  Serial.println("Настройки загружены:");
  Serial.println("API Key: " + apiKey);
  Serial.println("Интервал: " + String(interval / 1000) + " секунд");
  Serial.println("Порог PM2.5: низкий = " + String(pm2_5_low) + ", средний = " + String(pm2_5_medium));
}

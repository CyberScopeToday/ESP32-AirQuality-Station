# 🌿 ESP32-Based Air Quality Monitoring Station 🌿

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/c5382b64bc5b4619a6779507659a8db3)](https://app.codacy.com/gh/CyberScopeToday/ESP32-AirQuality-Station?utm_source=github.com&utm_medium=referral&utm_content=CyberScopeToday/ESP32-AirQuality-Station&utm_campaign=Badge_Grade)

<img src="https://github.com/user-attachments/assets/8db90330-ebba-4bf1-82b8-e2c0cf1ace0f" width="33%">

> **An IoT project for monitoring temperature, humidity, and air quality with a user-friendly interface, RGB indication, and control via a Telegram bot.** 🎉

---

**[🇷🇺 Click here for the Russian version of the README (Нажмите здесь для русской версии README)](readmeru.md)**

---

## 📋 Description

This air quality monitoring station measures:

- 🌡️ **Temperature** (sensors **DS18B20** and **SHT21**)
- 💧 **Humidity** (sensor **SHT21**)
- 🌫️ **Particle concentration** PM1.0, PM2.5, PM10 (sensor **PMS5003**)

### 📊 Acquired Data:

- **Displayed** on an OLED SSD1306 display 📺
- **Indicated** using an RGB LED 💡
- **Sent** to the **ThingSpeak** platform 🌐
- **Accessible** via a **Telegram bot** 🤖

---

## 🛠 Used Technologies

| Technology          | Description                                                |
|---------------------|------------------------------------------------------------|
| ESP32               | Main controller of the project                             |
| ![Arduino IDE](https://img.icons8.com/color/48/000000/arduino.png) **Arduino IDE** | Development environment for programming the ESP32 |
| ![ThingSpeak](https://img.icons8.com/color/48/000000/api.png) **ThingSpeak** | Platform for data analysis and storage             |
| ![Telegram](https://img.icons8.com/color/48/000000/telegram-app.png) **Telegram Bot** | Device management and data retrieval            |
| OLED Display        | Displays temperature, humidity, and air quality data        |

---

## 🎥 Device Demonstration

### 🚦 Air Quality Color Indication

🌈 Air quality indication via LED:

- 🟢 Green: Low pollution level (PM2.5 ≤ 12)
- 🔵 Blue: Medium pollution level (12 < PM2.5 ≤ 35)
- 🔴 Red: High pollution level (PM2.5 > 35)

| Pollution Level | Indicator Color  | GIF                                    |
|-----------------|------------------|----------------------------------------|
| **Low**         | 🟢 Green         | <img src="https://github.com/user-attachments/assets/2f5cfba2-cc02-4ce2-a702-51e87d47cd74" width="33%"> |
| **Medium**      | 🔵 Blue          | <img src="https://github.com/user-attachments/assets/b189bf3d-bf9e-4071-bd53-0382c53eed21" width="33%"> |
| **High**        | 🔴 Red           | <img src="https://github.com/user-attachments/assets/97edd4c7-efd2-48bd-ba6a-446bf991b99d" width="33%"> |

### 📟 Example of Data Display on the OLED Screen

<img src="https://github.com/user-attachments/assets/b03e0ae0-c027-4d38-8eed-2e1c358e847b" width="20%">

---

## ⚙️ Components

- 🪣 **ESP32 DevKit**
- 🌡️ **DS18B20** temperature sensor
- 🌡️ **SHT21** temperature and humidity sensor
- 🌫️ **PMS5003** air quality sensor
- 📺 **OLED SSD1306 display**
- 💡 **RGB LED**
- 🔗 Resistors, wires, and other components

---

## 🚀 Installation and Setup

### 1. Hardware Preparation

Assemble the circuit according to the code and schematics. Ensure all connections are secure.

### 2. Software Preparation

- Clone the repository:

  ```bash
  git clone https://github.com/CyberScopeToday/ESP32-AirQuality-Station.git
  ```

- Install **Arduino IDE** and the necessary libraries (see below).

### 3. Configuration Setup

#### Installing Libraries

Ensure the following libraries are installed in the Arduino IDE:

- `WiFi.h`  
- `Wire.h`  
- `Adafruit GFX Library`  
- `Adafruit SSD1306`  
- `OneWire`  
- `DallasTemperature`  
- `HTTPClient`  
- `SHT21`  
- `WiFiClientSecure`  
- `UniversalTelegramBot`  
- `ArduinoJson`  
- `Preferences`

#### Configuring `config.h`

Create a file named **`config.h`**:

```cpp
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"
#define THINGSPEAK_API_KEY "your_thingspeak_api_key"
#define TELEGRAM_TOKEN "your_telegram_bot_token"
#define TELEGRAM_CHAT_ID "your_telegram_chat_id"
```

Replace the placeholder values with your actual credentials:

- **`your_wifi_ssid`** — Your Wi-Fi network name
- **`your_wifi_password`** — Your Wi-Fi password
- **`your_thingspeak_api_key`** — Your ThingSpeak channel API key
- **`your_telegram_bot_token`** — Your Telegram bot token
- **`your_telegram_chat_id`** — Your Telegram chat ID

### 4. Uploading the Sketch

1. Connect the ESP32 via USB.
2. In **Arduino IDE**, select the **ESP32 DevKit** board.
3. Upload the code to the device.

---

## 📱 Telegram Bot Commands

| Command                       | Description                                                          |
|-------------------------------|----------------------------------------------------------------------|
| `/start`                      | Welcome message and help                                             |
| `/status`                     | Retrieve current data from the device                                |
| `/setapikey <api_key>`        | Set a new ThingSpeak API key                                         |
| `/setinterval <seconds>`      | Set data sending interval (minimum 300 seconds)                      |
| `/setthreshold <low> <medium>`| Set PM2.5 threshold values                                           |
| `/getsettings`                | View current settings                                                |
| `/toggledebug`                | Enable/disable debug information on the OLED display                 |

---

## 📸 Device Photos

<img src="https://github.com/user-attachments/assets/daf3410a-e842-46be-a4cd-a2c22c1daa27" width="40%">

---

## 📈 Data Visualization on ThingSpeak

All data collected by the ESP32 Air Quality Monitoring Station is sent to **ThingSpeak**, a cloud-based platform for IoT data visualization and analysis.

### 🌟 Live Dashboard Example

Explore a real-time example of air quality data visualization on ThingSpeak:  
**[ThingSpeak Channel - Air Quality Monitor](https://thingspeak.mathworks.com/channels/991348)**

### 📊 Charts Available:

- 🌡️ **Temperature**: Visualize ambient temperature from **DS18B20** and **SHT21** sensors.
- 💧 **Humidity**: Monitor real-time humidity levels measured by the **SHT21** sensor.
- 🌫️ **Particulate Matter (PM)**: Track air quality with PM1.0, PM2.5, and PM10 data from **PMS5003**:
  - **PM1.0**: Fine particles, diameter ≤ 1.0 µm
  - **PM2.5**: Particles, diameter ≤ 2.5 µm
  - **PM10**: Coarse particles, diameter ≤ 10 µm

---

### 🔗 How to Access

1. Open the **[ThingSpeak Channel](https://thingspeak.mathworks.com/channels/991348)**.
2. Use the **charts** to view real-time air quality data updated every 5 minutes.
3. Analyze trends, download datasets, or integrate the data into your own applications using ThingSpeak's API.

---

## 📄 License

This project is licensed under the **MIT** License. See [LICENSE](LICENSE) for details.

---

## 🎯 Useful Links

- 📕 [ESP32 Documentation](https://www.espressif.com/en/products/socs/esp32/resources)
- 🌐 [ThingSpeak API](https://thingspeak.com)
- 🤖 [Creating a Telegram Bot](https://core.telegram.org/bots)

---

# 🇷🇺 Русская версия README

You can find the Russian version of this README [here](readmeru.md).

---

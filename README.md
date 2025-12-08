# 🌱 BLOOM - Basic Local Observation & Optimized Moisture Control

[![ESP32](https://img.shields.io/badge/Platform-ESP32-blue.svg)](https://www.espressif.com/en/products/socs/esp32)
[![FreeRTOS](https://img.shields.io/badge/RTOS-FreeRTOS-green.svg)](https://www.freertos.org/)
[![MQTT](https://img.shields.io/badge/Protocol-MQTT-orange.svg)](https://mqtt.org/)
[![Blynk](https://img.shields.io/badge/IoT-Blynk-blueviolet.svg)](https://blynk.io/)

<div style="text-align: justify">

> An intelligent plant care system using dual ESP32 nodes with FreeRTOS, MQTT communication, and Blynk IoT dashboard for real-time monitoring and remote control.

## **👥 Team Members - Group 16**

- **Isyana Trevia Pohaci** (2306250592)
- **Arsinta Kirana Nisa** (2306215980)
- **Jonathan Frederick Kosasih** (2306225981)
- **Laura Fawzia Sambowo** (2306260145)

---

## **📋 Table of Contents**

- [Project Report](#-project-report)
  - [Introduction](#introduction)
  - [Implementation](#implementation)
  - [Testing & Evaluation](#testing--evaluation)
  - [Conclusion](#conclusion)
- [Features](#-features)
- [System Architecture](#-system-architecture)
- [Quick Start](#-quick-start)
- [Requirements](#-requirements)
  - [Hardware Requirements](#-hardware-requirements)
  - [Software Requirements](#-software-requirements)
- [Pin Configuration](#-pin-configuration)
- [Setup](#-setup)
  - [Installation](#-installation)
  - [Configuration](#-configuration)
- [Usage](#-usage)
- [Technical Details](#-technical-details)
  - [MQTT Topics](#-mqtt-topics)
  - [Blynk Dashboard Setup](#-blynk-dashboard-setup)
  - [FreeRTOS Implementation](#-freertos-implementation)
- [License](#-license)

---

## **📖 Project Report**

This section summarizes the formal project report for BLOOM (Real Time System and Internet of Things Final Project).

### **💡 Introduction**

BLOOM solves the plant care problem by implementing an IoT-based smart irrigation system using dual ESP32 nodes with FreeRTOS and MQTT. Unlike ineffective manual methods or non-adaptive timer systems, BLOOM uses real-time soil moisture data for automatic watering, provides remote monitoring via Blynk, and enables manual control from anywhere. All acceptance criteria have been met: stable sensor readings, automatic pump activation at <30% soil moisture for 5s, stable WiFi/MQTT connection, real-time Blynk dashboard updates, and remote watering capability.

### **⚙️ Implementation**

**Hardware Architecture:**
- **Node 1:** Capacitive soil moisture sensor (GPIO 34), DHT11 (GPIO 4), MOSFET-controlled pump (GPIO 27), LEDs, and buttons
- **Node 2:** Pure software gateway (no local hardware)

**Software Architecture:**
- **Node 1:** 6 FreeRTOS tasks for sensor reading, pump control, MQTT publishing, and button handling with Queues/Mutexes for synchronization
- **Node 2:** 2 tasks for MQTT bridging and Blynk cloud updates

**Communication & Infrastructure:**
- MQTT publishes sensor data to `plant/sensor/*` with 10-second intervals
- Watering triggers automatically at <30% soil moisture threshold
- MQTT Broker: HiveMQ Cloud | Cloud Platform: Blynk | WiFi: Configured via credentials

### **✅ Testing & Evaluation**

Both nodes successfully connected to mobile hotspot WiFi with stable communication verified via Serial console and Blynk dashboard. Network connectivity, real-time data transmission, automatic watering at <30% soil moisture, and Node 2 gateway stability all passed. Open-air sensor testing (0% constant) caused expected repeated watering cycles demonstrating correct system behavior. Limitations include fixed thresholds/duration, no long-term testing, and no outdoor testing, but the system demonstrates reliable IoT communication and distributed control architecture.

### **🎓 Conclusion**

BLOOM successfully demonstrates a complete IoT plant care solution combining dual-node ESP32 architecture, FreeRTOS multitasking, MQTT inter-node communication, and Blynk remote access. The system provides reliable sensor-based automatic watering, real-time monitoring, and remote control capabilities. Future enhancements could include adaptive thresholds per plant type, historical analytics, weather integration, and power optimization for long-term deployment.


## **✨ Features**

### **🌿 Node 1 - Sensor & Control**
- Real-time soil moisture monitoring
- Temperature & humidity sensing (DHT11)
- Automatic watering when soil < 30% moisture
- Manual watering via physical button
- Instant sensor reading via button press
- LED indicators (Green: Wet, Yellow: Dry)
- MQTT data publishing every 10 seconds
- Remote control via MQTT commands

### **🌐 Node 2 - Gateway & Dashboard**
- Pure software gateway (no hardware components)
- MQTT to Blynk bridge
- Real-time data forwarding
- Remote pump control via Blynk
- Auto-reconnection for WiFi/MQTT/Blynk
- Connection health monitoring
- System statistics logging

### **📱 Blynk Dashboard**
- Live sensor data visualization
- Soil moisture gauge (0-100%)
- Temperature & humidity display
- Pump status LED indicator
- Remote watering button
- Historical data charts
- Last update timestamp

---

## **🏗️ System Architecture**

```
┌─────────────────────────────────────────┐
│         ESP32 Node 1                    │
│    (Sensor + Control Node)              │
│                                         │
│  ┌──────────┐  ┌──────────┐             │
│  │  Soil    │  │  DHT11   │             │
│  │ Moisture │  │  Sensor  │             │
│  └──────────┘  └──────────┘             │
│        ↓            ↓                   │
│  ┌──────────────────────────────┐       │
│  │   FreeRTOS Task Manager      │       │
│  │  • taskReadSoil              │       │
│  │  • taskReadDHT               │       │
│  │  • taskPumpController        │       │
│  │  • taskMQTTPublish           │       │
│  │  • taskButtonHandler         │       │
│  │  • taskLEDIndicator          │       │
│  └──────────────────────────────┘       │
│        ↓                                │
│  [Pump Control] [LED Status]            │
└─────────────────┬───────────────────────┘
                  │
                  │ MQTT Protocol
                  ↓
        ┌─────────────────────┐
        │   MQTT Broker       │
        │   (HiveMQ Cloud)    │
        └─────────┬───────────┘
                  │
                  ↓
┌─────────────────────────────────────────┐
│         ESP32 Node 2                    │
│       (Gateway Node)                    │
│                                         │
│  ┌──────────────────────────────┐       │
│  │   FreeRTOS Gateway Tasks     │       │
│  │  • taskMQTTHandler           │       │
│  │  • taskBlynkUpdate           │       │
│  └──────────────────────────────┘       │
│        ↓                                │
│   [Blynk Client]                        │
└─────────────────┬───────────────────────┘
                  │
                  │ HTTPS
                  ↓
        ┌─────────────────────┐
        │   Blynk Cloud       │
        │   Dashboard         │
        └─────────────────────┘
                  │
                  ↓
        Mobile/Web App
```

---

## **🚀 Quick Start**

1. **Install PlatformIO** or Arduino IDE with ESP32 support
2. **Configure WiFi credentials** in code:
   ```cpp
   const char *WIFI_SSID = "YourWiFiName";
   const char *WIFI_PASS = "YourWiFiPassword";
   ```
3. **Upload** node1.ino to first ESP32
4. **Upload** node2.ino to second ESP32
5. **Configure Blynk** with auth token
6. **Monitor** via Serial + Blynk Dashboard

---

**Last Updated:** December 8, 2025  
**Status:** ✅ Project Complete

---

## **📊 Requirements**

### **🔧 Hardware Requirements**

#### **Node 1 Components:**

| Component | Quantity | Specifications |
|-----------|----------|----------------|
| ESP32 DevKit V1 | 1 | 38-pin version |
| Soil Moisture Sensor | 1 | Analog capacitive |
| DHT11 Temperature & Humidity Sensor | 1 | 3.3V compatible |
| Water Pump | 1 | DC 5V mini pump |
| MOSFET Module | 1 | D4184 or similar |
| LED Green | 1 | 3mm/5mm |
| LED Yellow | 1 | 3mm/5mm |
| Push Button | 2 | Tactile switch |
| Resistor 220Ω | 2 | For LEDs |
| Resistor 10kΩ | 2 | For buttons (pull-up) |
| Breadboard | 1 | 830 points |
| Jumper Wires | - | Male-to-male, male-to-female |
| USB Cable | 1 | Micro USB for ESP32 |
| Water Container | 1 | For pump reservoir |



#### **Node 2 Components:**
| Component | Quantity | Specifications |
|-----------|----------|----------------|
| ESP32 DevKit V1 | 1 | 38-pin version |
| USB Cable | 1 | Micro USB for ESP32 |

**Total Cost Estimate:** ~$25-30 USD

### **💻 Software Requirements**

#### **🛠️ Development Environment:**
- [Arduino IDE](https://www.arduino.cc/en/software) 1.8.19+ or [PlatformIO](https://platformio.org/)
- [ESP32 Board Support](https://github.com/espressif/arduino-esp32) (v2.0.0+)

#### **📚 Required Libraries:**
```cpp
// For both nodes:
- WiFi (built-in)
- PubSubClient (MQTT client)

// Node 1 only:
- DHT sensor library (Adafruit)
- Adafruit Unified Sensor
- Preferences (built-in)

// Node 2 only:
- Blynk (v1.3.2+)
```

#### **☁️ External Services:**
- [HiveMQ Cloud](https://www.hivemq.com/mqtt-cloud-broker/) (Free tier)
- [Blynk IoT](https://blynk.cloud/) (Free tier)

---

## **📌 Pin Configuration**

### **ESP32 Node 1:**

| Component | GPIO Pin | Type | Notes |
|-----------|----------|------|-------|
| Soil Moisture Sensor | GPIO 34 | Analog Input | ADC1_CH6 |
| DHT11 Data | GPIO 4 | Digital I/O | DHT11 signal |
| Water Pump (MOSFET) | GPIO 27 | Digital Output | PWM capable |
| LED Green | GPIO 18 | Digital Output | Normal status |
| LED Yellow | GPIO 19 | Digital Output | Dry status |
| Button Read | GPIO 32 | Digital Input | Pull-up enabled |
| Button Water | GPIO 33 | Digital Input | Pull-up enabled |

### **ESP32 Node 2:**
No GPIO pins used (pure software gateway)

---

## **🛠️ Setup**

### **🚀 Installation**

### **Step 1: Hardware Assembly**

#### **Node 1 Wiring:**

**Soil Moisture Sensor:**
```
VCC  → 3.3V
GND  → GND
AO   → GPIO 34
```

**DHT11 Sensor:**
```
VCC  → 3.3V
GND  → GND
DATA → GPIO 4
```

**Water Pump via MOSFET:**
```
MOSFET Gate     → GPIO 27
MOSFET Source   → GND (shared with ESP32)
MOSFET Drain    → Pump (-)
Pump (+)        → 5V External Power
```

**LEDs:**
```
LED Green Anode  → GPIO 18 → 220Ω → LED → GND
LED Yellow Anode → GPIO 19 → 220Ω → LED → GND
```

**Buttons:**
```
Button Read:  GPIO 32 → Button → GND (internal pull-up)
Button Water: GPIO 33 → Button → GND (internal pull-up)
```

#### **Node 2 Setup:**
Simply connect ESP32 to computer via USB. No additional wiring needed.

### **Step 2: Software Installation**

#### **Option A: Using Arduino IDE**

1. **Install ESP32 Board Support:**
   - Open Arduino IDE
   - Go to `File → Preferences`
   - Add to "Additional Board Manager URLs":
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Go to `Tools → Board → Board Manager`
   - Search "ESP32" and install

2. **Install Libraries:**
   - Go to `Sketch → Include Library → Manage Libraries`
   - Install:
     - `PubSubClient` by Nick O'Leary
     - `DHT sensor library` by Adafruit
     - `Adafruit Unified Sensor`
     - `Blynk` by Volodymyr Shymanskyy

3. **Upload Code:**
   - Open `node1.ino` for Node 1
   - Select Board: `ESP32 Dev Module`
   - Select Port
   - Click Upload
   - Repeat for `node2.ino` on Node 2

#### **🚀 Option B: Using PlatformIO**

1. **Install PlatformIO Extension** in VS Code

2. **Create Project:**
   ```bash
   pio init --board esp32dev
   ```

3. **Copy provided code** to `src/main.cpp`

4. **Upload:**
   ```bash
   # For Node 1
   pio run -e node1 -t upload
   
   # For Node 2
   pio run -e node2 -t upload
   ```

---

### **⚙️ Configuration**

### **WiFi Configuration (Both Nodes):**

```cpp
const char* WIFI_SSID = "YourWiFiName";
const char* WIFI_PASS = "YourWiFiPassword";
```

### **MQTT Configuration (Both Nodes):**

```cpp
const char* MQTT_BROKER = "broker.hivemq.com";
const int MQTT_PORT = 1883;
```

For private broker, replace with your broker URL.

### **Blynk Configuration (Node 2 Only):**

1. Create account at [blynk.cloud](https://blynk.cloud)
2. Create new template
3. Add datastreams (V0-V6)
4. Create device and copy credentials:

```cpp
#define BLYNK_TEMPLATE_ID "TMPLxxxxxx"
#define BLYNK_TEMPLATE_NAME "BLOOM"
#define BLYNK_AUTH_TOKEN "YourAuthTokenHere"
```

### **Sensor Thresholds (Node 1):**

Adjust in code if needed:
```cpp
#define SOIL_DRY_THRESHOLD 30    // Trigger watering below this
#define SOIL_WET_THRESHOLD 60    // Normal above this
#define PUMP_DURATION 5000       // Watering duration (ms)
```

---

## **📖 Usage**

### **Starting the System:**

1. **Power on Node 1:**
   - System initializes sensors
   - Connects to WiFi and MQTT
   - Begins monitoring soil moisture
   - Publishes data every 10 seconds

2. **Power on Node 2:**
   - Connects to WiFi, MQTT, and Blynk
   - Subscribes to Node 1 data topics
   - Forwards data to Blynk dashboard
   - Ready to receive remote commands

### **Physical Controls (Node 1):**

- **Button 1 (GPIO 32) - "Read Sensor":**
  - Press to get instant sensor readings
  - Forces immediate MQTT publish
  - Useful for testing or quick checks

- **Button 2 (GPIO 33) - "Manual Water":**
  - Press to water plants manually
  - Activates pump for 5 seconds
  - Bypasses automatic watering logic

### **💡 LED Indicators (Node 1):**

| LED Status | Meaning |
|------------|---------|
| Green ON | Soil moisture ≥ 60% (Healthy) |
| Yellow ON | Soil moisture ≤ 30% (Needs water) |
| Both OFF | Soil moisture 31-59% (Normal) |
| Both BLINKING | Pump is active (watering) |

### **Remote Control (via Blynk):**

1. Open Blynk app/web dashboard
2. View real-time sensor data
3. Press "Manual Water" button (V5)
4. Pump activates for 5 seconds
5. Status updates automatically

### **Automatic Watering:**

- System checks soil moisture every 5 seconds
- When moisture drops below 30%:
  - Pump activates automatically
  - Waters for 5 seconds
  - Yellow LED blinks during watering
  - Status updates to dashboard
  - Returns to monitoring mode

---

## **🔧📚 Technical Details**

### **📡 MQTT Topics**

### **Published by Node 1:**

| Topic | Data Type | Example | Update Rate |
|-------|-----------|---------|-------------|
| `plant/sensor/moisture` | Integer (0-100) | `45` | 10 seconds |
| `plant/sensor/temp` | Float (°C) | `28.5` | 10 seconds |
| `plant/sensor/humidity` | Float (%) | `65.0` | 10 seconds |
| `plant/control/status` | String | `ON` / `OFF` | On change |

### **Subscribed by Node 1:**

| Topic | Data Type | Command | Action |
|-------|-----------|---------|--------|
| `plant/control/pump` | String | `ON` | Activate pump |
| `plant/control/pump` | String | `OFF` | Deactivate pump |

### **Data Flow:**
```
Node 1 → MQTT Broker → Node 2 → Blynk Cloud → User Dashboard
User Dashboard → Blynk Cloud → Node 2 → MQTT Broker → Node 1
```

---

### **📱 Blynk Dashboard Setup**

#### **Step 1: Create Template**

1. Login to [blynk.cloud](https://blynk.cloud)
2. Go to `Developer Zone` → `My Templates`
3. Click `+ New Template`
4. Configure:
   - **Name:** BLOOM Smart Plant
   - **Hardware:** ESP32
   - **Connection:** WiFi

#### **Step 2: Create Datastreams**

| Virtual Pin | Name | Type | Min | Max | Default | Unit |
|-------------|------|------|-----|-----|---------|------|
| V0 | Soil Moisture | Integer | 0 | 100 | 0 | % |
| V1 | Temperature | Double | -10 | 50 | 0 | °C |
| V2 | Humidity | Double | 0 | 100 | 0 | % |
| V3 | Pump Status | Integer | 0 | 1 | 0 | - |
| V4 | Status Text | String | - | - | Idle | - |
| V5 | Remote Button | Integer | 0 | 1 | 0 | - |
| V6 | Last Update | String | - | - | Never | - |

#### **Step 3: Design Dashboard**

#### **Web Dashboard Layout:**

![Blynk Dashboard](https://i.ibb.co.com/2YKxLw63/blynk.jpg)

#### **Widget Configuration:**

1. **Soil Moisture Gauge (V0):**
   - Min: 0, Max: 100
   - Color: White-Green gradient
   - Label: "Soil Moisture %"

2. **Temperature Gauge (V1):**
   - Min: -10, Max: 50
   - Color: Blue-Red gradient
   - Label: "Temperature °C"

3. **Humidity Gauge (V2):**
   - Min: 0, Max: 100
   - Color: White-Green gradient
   - Label: "Air Humidity %"

4. **Pump LED (V3):**
   - ON Color: Green
   - OFF Color: Gray
   - Label: "Pump Status"

5. **Status Label (V4):**
   - Display raw value
   - Label: "Current Status"

6. **Remote Water Control Switch (V5):**
   - ON Label: "WATERING"
   - OFF Label: "START WATERING"

7. **Last Update Label (V6):**
   - Display raw value
   - Label: "Data Age"

8. **Soil Moisture Chart (V0):**
   - Display historical data
   - Label: "Soil Moisture Chart"

9. **Temperature Chart (V1):**
   - Display historical data
   - Label: "Temperature Chart"

10. **Humidity Chart (V2):**
    - Display historical data
    - Label: "Humidity Chart"

#### **Step 4: Create Device**

1. Go to `Devices` → `+ New Device`
2. Choose `From Template` → Select BLOOM template
3. Give device a name
4. Copy the `Auth Token` for Node 2 code

---

### **🧵 FreeRTOS Implementation**

#### **Node 1 - Tasks:**

| Task Name | Priority | Stack | Core | Purpose |
|-----------|----------|-------|------|---------|
| `taskReadSoil` | 2 | 4096 | 0 | Read soil moisture every 5s |
| `taskReadDHT` | 2 | 4096 | 0 | Read temp/humidity every 5s |
| `taskPumpController` | 3 | 4096 | 0 | Auto-watering logic |
| `taskMQTTPublish` | 2 | 8192 | 1 | Publish data every 10s |
| `taskButtonHandler` | 2 | 4096 | 1 | Handle button presses |
| `taskLEDIndicator` | 1 | 2048 | 0 | Control LED status |

#### **Node 1 - Synchronization:**

- **Queue:** `sensorQueue` - Transfer sensor data between tasks
- **Mutex:** `pumpMutex` - Protect pump control from race conditions
- **Semaphore:** `mqttSemaphore` - Serialize MQTT publishing
- **Timer:** `sensorTimer` - Trigger periodic sensor reads

#### **Node 2 - Tasks:**

| Task Name | Priority | Stack | Core | Purpose |
|-----------|----------|-------|------|---------|
| `taskMQTTHandler` | 2 | 8192 | 1 | Process MQTT messages |
| `taskBlynkUpdate` | 2 | 8192 | 1 | Update Blynk dashboard |

#### **Node 2 - Synchronization:**

- **Queue:** `mqttDataQueue` - Buffer incoming MQTT messages
- **Mutex:** `blynkMutex` - Protect Blynk API calls
- **Timer:** `connectionTimer` - Periodic connection health check

---

## **📝 License**

Group Project - Universitas Indonesia  
Department of Electrical Engineering  
Real Time System and Internet of Things Course

---

<p align="center">
  Made with 💚 by Group 16<br>
  <strong>BLOOM - Because every plant deserves smart care</strong>
</p>

</div>

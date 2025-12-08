/*
 * ESP32-Node1: BLOOM (Sensor + Control)
 *
 * Group 16:
 * Isyana Trevia Pohaci (2306250592)
 * Arsinta Kirana Nisa (2306215980)
 * Jonathan Frederick Kosasih (2306225981)
 * Laura Fawzia Sambowo (2306260145)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Preferences.h>

// pin definitions
#define SOIL_MOISTURE_PIN 34 // ADC Pin for Soil Moisture Sensor
#define DHT_PIN 4            // DHT11 Data Pin
#define PUMP_PIN 27          // MOSFET Control Pin
#define LED_GREEN_PIN 18     // Green LED
#define LED_YELLOW_PIN 19    // Yellow LED
#define BTN_READ_PIN 32      // Push Button Read
#define BTN_WATER_PIN 33     // Push Button Water

// sensor configuration
#define DHT_TYPE DHT11
#define SOIL_DRY_THRESHOLD 30 // under this = dry
#define SOIL_WET_THRESHOLD 60 // over this = wet
#define PUMP_DURATION 5000    // 5 seconds watering

// wifi & mqtt configuration
const char *WIFI_SSID = "kel16";
const char *WIFI_PASS = "pojolaran"; // poci joko laura kiran

const char *MQTT_BROKER = "broker.hivemq.com"; // broker public
const int MQTT_PORT = 1883;
const char *MQTT_CLIENT_ID = "ESP32_Node1_SmartPlant";

// MQTT Topics - send data to Node2
const char *TOPIC_SOIL = "plant/sensor/moisture";
const char *TOPIC_TEMP = "plant/sensor/temp";
const char *TOPIC_HUMIDITY = "plant/sensor/humidity";
const char *TOPIC_PUMP_STATUS = "plant/control/status";

// MQTT Topics - receive command from Node2
const char *TOPIC_PUMP_COMMAND = "plant/control/pump";

// global objects
DHT dht(DHT_PIN, DHT_TYPE);
WiFiClient espClient;
PubSubClient mqtt(espClient);
Preferences prefs;

// FreeRTOS handles
QueueHandle_t sensorQueue;
SemaphoreHandle_t pumpMutex;
SemaphoreHandle_t mqttSemaphore;
TimerHandle_t sensorTimer;

// data structures
typedef struct
{
    int soil;
    float temp;
    float hum;
    unsigned long timestamp;
} SensorData_t;

// global variables
volatile bool pumpActive = false;
volatile bool manualOverride = false;
SensorData_t latestData = {0, 0.0, 0.0, 0};

// function prototypes
void setupWiFi();
void setupMQTT();
void reconnectMQTT();
void mqttCallback(char *topic, byte *payload, unsigned int length);
int readSoilMoisture();
void controlPump(bool state);
void updateLEDIndicators(int soilPercent);

// FreeRTOS tasks
void taskReadSoil(void *parameter);
void taskReadDHT(void *parameter);
void taskPumpController(void *parameter);
void taskMQTTPublish(void *parameter);
void taskButtonHandler(void *parameter);
void taskLEDIndicator(void *parameter);
void sensorTimerCallback(TimerHandle_t xTimer);

// setup function
void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== ESP32-Node1 Smart Plant System ===");
    Serial.println("Mode: Sensor + Control (MQTT Only)");

    // pin setup
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(LED_GREEN_PIN, OUTPUT);
    pinMode(LED_YELLOW_PIN, OUTPUT);
    pinMode(BTN_READ_PIN, INPUT_PULLUP);
    pinMode(BTN_WATER_PIN, INPUT_PULLUP);
    digitalWrite(PUMP_PIN, LOW);

    dht.begin();

    setupWiFi();
    setupMQTT();

    // FreeRTOS setup
    sensorQueue = xQueueCreate(10, sizeof(SensorData_t));
    pumpMutex = xSemaphoreCreateMutex();
    mqttSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(mqttSemaphore);

    sensorTimer = xTimerCreate("SensorTimer", pdMS_TO_TICKS(5000),
                               pdTRUE, (void *)0, sensorTimerCallback);
    xTimerStart(sensorTimer, 0);

    // create tasks
    xTaskCreatePinnedToCore(taskReadSoil, "ReadSoil", 4096, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(taskReadDHT, "ReadDHT", 4096, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(taskPumpController, "PumpCtrl", 4096, NULL, 3, NULL, 0);
    xTaskCreatePinnedToCore(taskMQTTPublish, "MQTT", 8192, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(taskButtonHandler, "Buttons", 4096, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(taskLEDIndicator, "LED", 2048, NULL, 1, NULL, 0);

    Serial.println("System Ready! Sending data to MQTT...");
}

// main loop
void loop()
{
    if (!mqtt.connected())
    {
        reconnectMQTT();
    }
    mqtt.loop();
    delay(10);
}

// setup WiFi connection
void setupWiFi()
{
    Serial.print("Connecting to WiFi");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi Connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}

// setup MQTT connection
void setupMQTT()
{
    mqtt.setServer(MQTT_BROKER, MQTT_PORT);
    mqtt.setCallback(mqttCallback);
    reconnectMQTT();
}

// reconnect MQTT
void reconnectMQTT()
{
    while (!mqtt.connected())
    {
        Serial.print("Connecting to MQTT...");
        if (mqtt.connect(MQTT_CLIENT_ID))
        {
            Serial.println("Connected!");
            mqtt.subscribe(TOPIC_PUMP_COMMAND);
            Serial.println("Subscribed to: plant/control/pump");
        }
        else
        {
            Serial.print("Failed, rc=");
            Serial.println(mqtt.state());
            delay(2000);
        }
    }
}

// MQTT message callback
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    String message = "";
    for (unsigned int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }

    Serial.printf("MQTT Received [%s]: %s\n", topic, message.c_str());

    if (String(topic) == TOPIC_PUMP_COMMAND)
    {
        if (message == "ON")
        {
            manualOverride = true;
            controlPump(true);
            Serial.println("Remote Pump ON (from Node2)");
        }
        else if (message == "OFF")
        {
            manualOverride = false;
            controlPump(false);
            Serial.println("Remote Pump OFF (from Node2)");
        }
    }
}

// sensor reading func
int readSoilMoisture()
{
    int rawValue = analogRead(SOIL_MOISTURE_PIN);
    int percent = map(rawValue, 4095, 0, 0, 100);
    percent = constrain(percent, 0, 100);
    return percent;
}

// control pump func
void controlPump(bool state)
{
    if (xSemaphoreTake(pumpMutex, portMAX_DELAY))
    {
        digitalWrite(PUMP_PIN, state ? HIGH : LOW);
        pumpActive = state;

        // update MQTT status
        mqtt.publish(TOPIC_PUMP_STATUS, state ? "ON" : "OFF");
        Serial.printf("Pump Status Published: %s\n", state ? "ON" : "OFF");

        xSemaphoreGive(pumpMutex);
    }
}

// update LED based on soil moisture
void updateLEDIndicators(int soilPercent)
{
    if (soilPercent >= SOIL_WET_THRESHOLD)
    {
        // soil wet - LED Green On (GPIO 18)
        digitalWrite(LED_GREEN_PIN, HIGH);
        digitalWrite(LED_YELLOW_PIN, LOW);
    }
    else if (soilPercent <= SOIL_DRY_THRESHOLD)
    {
        // soil dry - LED Yellow On (GPIO 19)
        digitalWrite(LED_GREEN_PIN, LOW);
        digitalWrite(LED_YELLOW_PIN, HIGH);
    }
    else
    {
        // Moderate - Both LEDs Off
        digitalWrite(LED_GREEN_PIN, LOW);
        digitalWrite(LED_YELLOW_PIN, LOW);
    }
}

// FreeRTOS Tasks
// timer callback
void sensorTimerCallback(TimerHandle_t xTimer)
{
    static bool trigger = true;
    trigger = !trigger;
}

// task 1: read soil moisture
void taskReadSoil(void *parameter)
{
    SensorData_t data;

    while (1)
    {
        data.soil = readSoilMoisture();
        data.timestamp = millis();

        if (xQueueSend(sensorQueue, &data, pdMS_TO_TICKS(100)) != pdPASS)
        {
            Serial.println("Queue full - Soil data dropped");
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// task 2: read DHT11
void taskReadDHT(void *parameter)
{
    SensorData_t data;

    while (1)
    {
        float temp = dht.readTemperature();
        float hum = dht.readHumidity();

        if (!isnan(temp) && !isnan(hum))
        {
            data.temp = temp;
            data.hum = hum;
            data.timestamp = millis();

            latestData.temp = temp;
            latestData.hum = hum;

            Serial.printf("DHT: Temp=%.1f°C, Hum=%.1f%%\n", temp, hum);
        }
        else
        {
            Serial.println("DHT Read Failed!");
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// task 3: pump controller (auto watering logic)
void taskPumpController(void *parameter)
{
    SensorData_t data;

    while (1)
    {
        if (xQueueReceive(sensorQueue, &data, portMAX_DELAY))
        {
            latestData.soil = data.soil;

            Serial.printf("Soil Moisture: %d%%\n", data.soil);

            // auto watering logic (if not manual override)
            if (!manualOverride)
            {
                if (data.soil <= SOIL_DRY_THRESHOLD && !pumpActive)
                {
                    Serial.println("AUTO: Soil too dry! Starting pump...");
                    controlPump(true);
                    vTaskDelay(pdMS_TO_TICKS(PUMP_DURATION));
                    controlPump(false);
                    Serial.println("AUTO: Pump stopped");
                }
            }

            updateLEDIndicators(data.soil);
        }
    }
}

// task 4: MQTT Publisher
void taskMQTTPublish(void *parameter)
{
    char buffer[50];

    while (1)
    {
        if (xSemaphoreTake(mqttSemaphore, pdMS_TO_TICKS(100)))
        {
            // publish soil moisture
            sprintf(buffer, "%d", latestData.soil);
            mqtt.publish(TOPIC_SOIL, buffer);
            Serial.printf("Published Soil: %d%%\n", latestData.soil);

            // publish temperature
            dtostrf(latestData.temp, 4, 1, buffer);
            mqtt.publish(TOPIC_TEMP, buffer);
            Serial.printf("Published Temp: %.1f°C\n", latestData.temp);

            // publish humidity
            dtostrf(latestData.hum, 4, 1, buffer);
            mqtt.publish(TOPIC_HUMIDITY, buffer);
            Serial.printf("Published Humidity: %.1f%%\n", latestData.hum);

            Serial.println("--- Data sent to MQTT ---\n");

            xSemaphoreGive(mqttSemaphore);
        }

        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

// task 5: button handler
void taskButtonHandler(void *parameter)
{
    static unsigned long lastPressRead = 0;
    static unsigned long lastPressWater = 0;
    const unsigned long debounceDelay = 300;

    while (1)
    {
        // button read sensor (immediate read) - GPIO 32
        if (digitalRead(BTN_READ_PIN) == LOW)
        {
            if (millis() - lastPressRead > debounceDelay)
            {
                lastPressRead = millis();
                Serial.println("\n=== BTN: Immediate sensor read ===");

                // force read sensors
                int soil = readSoilMoisture();
                float temp = dht.readTemperature();
                float hum = dht.readHumidity();

                Serial.printf("Soil: %d%% | Temp: %.1f°C | Hum: %.1f%%\n", soil, temp, hum);

                // update immediately
                latestData.soil = soil;
                latestData.temp = temp;
                latestData.hum = hum;

                // force publish to MQTT
                char buffer[50];
                sprintf(buffer, "%d", soil);
                mqtt.publish(TOPIC_SOIL, buffer);
                dtostrf(temp, 4, 1, buffer);
                mqtt.publish(TOPIC_TEMP, buffer);
                dtostrf(hum, 4, 1, buffer);
                mqtt.publish(TOPIC_HUMIDITY, buffer);
                Serial.println("Data force-published to MQTT\n");
            }
        }

        // button manual water - GPIO 33
        if (digitalRead(BTN_WATER_PIN) == LOW)
        {
            if (millis() - lastPressWater > debounceDelay)
            {
                lastPressWater = millis();
                Serial.println("\n=== BTN: Manual watering ===");

                manualOverride = true;
                controlPump(true);
                vTaskDelay(pdMS_TO_TICKS(PUMP_DURATION));
                controlPump(false);
                manualOverride = false;

                Serial.println("Manual watering completed\n");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// task 6: LED indicator
void taskLEDIndicator(void *parameter)
{
    while (1)
    {
        if (pumpActive)
        {
            // blink both LEDs when pump active
            digitalWrite(LED_GREEN_PIN, !digitalRead(LED_GREEN_PIN));
            digitalWrite(LED_YELLOW_PIN, !digitalRead(LED_YELLOW_PIN));
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}
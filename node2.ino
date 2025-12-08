/*
 * ESP32-Node2: BLOOM (Coordinator + Blynk)

 * Group 16 :
 * Isyana Trevia Pohaci (2306250592)
 * Arsinta Kirana Nisa (2306215980)
 * Jonathan Frederick Kosasih (2306225981)
 * Laura Fawzia Sambowo (2306260145)
*/ 

// Blynk configuration
#define BLYNK_TEMPLATE_ID "TMPL6fKKyzWyk"
#define BLYNK_TEMPLATE_NAME "FinProIOT"
#define BLYNK_AUTH_TOKEN "Twae2c887b529mUXhd3thzjzqNEmMAtm"
#define BLYNK_PRINT Serial

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <BlynkSimpleEsp32.h>

// wifi & mqtt configuration
const char *WIFI_SSID = "kel16";
const char *WIFI_PASS = "pojolaran"; // poci joko laura kiran

const char *MQTT_BROKER = "broker.hivemq.com";
const int MQTT_PORT = 1883;
const char *MQTT_CLIENT_ID = "ESP32_Node2_Coord"; // unique client ID

// MQTT topics
const char *TOPIC_SOIL = "plant/sensor/moisture";
const char *TOPIC_TEMP = "plant/sensor/temp";
const char *TOPIC_HUMIDITY = "plant/sensor/humidity";
const char *TOPIC_PUMP_STATUS = "plant/control/status";
const char *TOPIC_PUMP_COMMAND = "plant/control/pump";

// Blynk virtual pins
#define VPIN_SOIL V0
#define VPIN_TEMP V1
#define VPIN_HUMIDITY V2
#define VPIN_PUMP V3
#define VPIN_STATUS V4
#define VPIN_REMOTE_BTN V5
#define VPIN_LAST_UPDATE V6

// global objects
WiFiClient espClient;
PubSubClient mqtt(espClient);

// FreeRTOS handles
QueueHandle_t mqttDataQueue;
SemaphoreHandle_t blynkMutex;
TimerHandle_t connectionTimer;

// data structures
typedef struct
{
    String topic;
    String payload;
    unsigned long timestamp;
} MQTTMessage_t;

typedef struct
{
    int soilMoisture;
    float temperature;
    float humidity;
    String pumpStatus;
    unsigned long lastUpdate;
} PlantData_t;

// global variables
PlantData_t plantData = {0, 0.0, 0.0, "Idle", 0};
volatile bool wifiConnected = false;
volatile bool mqttConnected = false;
volatile bool blynkConnected = false;

// function prototypes
void setupWiFi();
void setupMQTT();
void reconnectMQTT();
void mqttCallback(char *topic, byte *payload, unsigned int length);
void sendPumpCommand(bool state);
void updateBlynkDashboard();

// FreeRTOS tasks
void taskMQTTHandler(void *parameter);
void taskBlynkUpdate(void *parameter);
void connectionTimerCallback(TimerHandle_t xTimer);

// setup func
void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== ESP32-Node2 Coordinator System ===");
    Serial.println("Mode: MQTT -> Blynk Gateway (No Local Hardware)");

    // create Queue & Mutex
    mqttDataQueue = xQueueCreate(20, sizeof(MQTTMessage_t));
    if (mqttDataQueue == NULL)
    {
        Serial.println("Error creating Queue!");
        while (1)
            ; // halt if failed
    }

    blynkMutex = xSemaphoreCreateMutex();
    if (blynkMutex == NULL)
    {
        Serial.println("Error creating Mutex!");
        while (1)
            ; // halt if failed
    }

    // timer setup
    connectionTimer = xTimerCreate("ConnCheck", pdMS_TO_TICKS(30000),
                                   pdTRUE, (void *)0, connectionTimerCallback);
    if (connectionTimer != NULL)
    {
        xTimerStart(connectionTimer, 0);
    }

    // after creating Queue & Mutex, start WiFi, MQTT, Blynk
    // WiFi & MQTT
    setupWiFi();
    setupMQTT();

    // Blynk setup
    Serial.println("Connecting to Blynk...");
    Blynk.config(BLYNK_AUTH_TOKEN);

    // connect Blynk
    int attempts = 0;
    while (!Blynk.connect() && attempts < 5)
    {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (Blynk.connected())
    {
        blynkConnected = true;
        Serial.println("\nBlynk Connected!");
    }
    else
    {
        Serial.println("\nBlynk Connection Failed! Will retry in background...");
    }

    // create tasks 
    xTaskCreatePinnedToCore(taskMQTTHandler, "MQTT", 8192, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(taskBlynkUpdate, "Blynk", 8192, NULL, 2, NULL, 1);

    Serial.println("Coordinator Ready!");
    Serial.println("Waiting for data from Node1...\n");
}

// main loop
void loop()
{
    // MQTT reconnect if needed
    if (!mqtt.connected())
    {
        reconnectMQTT();
    }
    mqtt.loop();

    if (blynkConnected)
    {
        Blynk.run();
    }

    delay(10);
}

// WiFi setup
void setupWiFi()
{
    Serial.print("Connecting to WiFi");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20)
    {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        wifiConnected = true;
        Serial.println("\nWiFi Connected!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("\nWiFi Connection Failed!");
        wifiConnected = false;
    }
}

// MQTT setup
void setupMQTT()
{
    mqtt.setServer(MQTT_BROKER, MQTT_PORT);
    mqtt.setCallback(mqttCallback);
    mqtt.setBufferSize(1024); 
    reconnectMQTT();
}

// MQTT reconnect
void reconnectMQTT()
{
    // make sure WiFi is connected
    if (WiFi.status() != WL_CONNECTED)
        return;

    int attempts = 0;
    // try to connect max 2 times to avoid blocking too long in loop
    while (!mqtt.connected() && attempts < 2)
    {
        Serial.print("Connecting to MQTT...");
        if (mqtt.connect(MQTT_CLIENT_ID))
        {
            mqttConnected = true;
            Serial.println("Connected!");

            mqtt.subscribe(TOPIC_SOIL);
            mqtt.subscribe(TOPIC_TEMP);
            mqtt.subscribe(TOPIC_HUMIDITY);
            mqtt.subscribe(TOPIC_PUMP_STATUS);

            Serial.println("Subscribed to topics");
        }
        else
        {
            mqttConnected = false;
            Serial.print("Failed, rc=");
            Serial.println(mqtt.state());
            attempts++;
            delay(1000);
        }
    }
}

// MQTT message callback
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    // make sure Queue exists before sending
    if (mqttDataQueue == NULL)
        return;

    MQTTMessage_t msg;
    msg.topic = String(topic);
    msg.payload = "";

    for (unsigned int i = 0; i < length; i++)
    {
        msg.payload += (char)payload[i];
    }
    msg.timestamp = millis();

    if (xQueueSend(mqttDataQueue, &msg, pdMS_TO_TICKS(10)) != pdPASS)
    {
        Serial.println("Queue full - Message dropped");
    }
}

// send pump command to Node1
void sendPumpCommand(bool state)
{
    if (mqtt.connected())
    {
        const char *cmd = state ? "ON" : "OFF";
        mqtt.publish(TOPIC_PUMP_COMMAND, cmd);
        Serial.printf("Sent pump command to Node1: %s\n", cmd);

        plantData.pumpStatus = state ? "Watering..." : "Idle";

        if (blynkConnected)
        {
            Blynk.virtualWrite(VPIN_STATUS, plantData.pumpStatus);
            Blynk.virtualWrite(VPIN_PUMP, state ? 1 : 0);
        }
    }
    else
    {
        Serial.println("MQTT not connected! Cannot send command.");
    }
}

// update Blynk dashboard
void updateBlynkDashboard()
{
    if (!blynkConnected || blynkMutex == NULL)
        return;

    if (xSemaphoreTake(blynkMutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        Blynk.virtualWrite(VPIN_SOIL, plantData.soilMoisture);
        Blynk.virtualWrite(VPIN_TEMP, plantData.temperature);
        Blynk.virtualWrite(VPIN_HUMIDITY, plantData.humidity);
        Blynk.virtualWrite(VPIN_STATUS, plantData.pumpStatus);

        int pumpValue = (plantData.pumpStatus == "Watering...") ? 1 : 0;
        Blynk.virtualWrite(VPIN_PUMP, pumpValue);

        unsigned long secAgo = (millis() - plantData.lastUpdate) / 1000;
        String timeText = String(secAgo) + "s ago";
        Blynk.virtualWrite(VPIN_LAST_UPDATE, timeText);

        xSemaphoreGive(blynkMutex);
    }
}

// FreeRTOS Tasks
// task 1: MQTT message handler
void taskMQTTHandler(void *parameter)
{
    MQTTMessage_t msg;
    while (1)
    {
        if (xQueueReceive(mqttDataQueue, &msg, portMAX_DELAY))
        {
            Serial.printf("Processing [%s]: %s\n", msg.topic.c_str(), msg.payload.c_str());

            if (msg.topic == TOPIC_SOIL)
            {
                plantData.soilMoisture = msg.payload.toInt();
                plantData.lastUpdate = millis();
                Serial.printf("  -> Soil Moisture: %d%%\n", plantData.soilMoisture);
            }
            else if (msg.topic == TOPIC_TEMP)
            {
                plantData.temperature = msg.payload.toFloat();
                plantData.lastUpdate = millis();
                Serial.printf("  -> Temperature: %.1f°C\n", plantData.temperature);
            }
            else if (msg.topic == TOPIC_HUMIDITY)
            {
                plantData.humidity = msg.payload.toFloat();
                plantData.lastUpdate = millis();
                Serial.printf("  -> Humidity: %.1f%%\n", plantData.humidity);
            }
            else if (msg.topic == TOPIC_PUMP_STATUS)
            {
                plantData.pumpStatus = msg.payload; // "ON" or "OFF"
                if (plantData.pumpStatus == "ON")
                    plantData.pumpStatus = "Watering...";
                else
                    plantData.pumpStatus = "Idle";

                plantData.lastUpdate = millis();
                Serial.printf("  -> Pump Status: %s\n", plantData.pumpStatus.c_str());
            }
            Serial.println();
        }
    }
}

// task 2: Blynk update
void taskBlynkUpdate(void *parameter)
{
    while (1)
    {
        if (blynkConnected)
        {
            updateBlynkDashboard();
        }
        else
        {
            if (Blynk.connect())
            {
                blynkConnected = true;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// task 3: connection checker
void connectionTimerCallback(TimerHandle_t xTimer)
{
    // only print status, reconnect handled in loop/task
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[Check] WiFi Lost");
        setupWiFi();
    }
    if (!mqtt.connected())
    {
        Serial.println("[Check] MQTT Lost");
    }
    if (!Blynk.connected())
    {
        Serial.println("[Check] Blynk Lost");
    }
}

// Blynk write handlers
BLYNK_WRITE(VPIN_REMOTE_BTN)
{
    int value = param.asInt();
    Serial.printf("\n=== Blynk Remote Control: %s ===\n", value ? "ON" : "OFF");

    sendPumpCommand(value == 1);

    if (value == 1)
    {
        Serial.println("Auto-off timer started...");
        // wait for 5 seconds while keeping Blynk and MQTT running
        unsigned long start = millis();
        while (millis() - start < 5000)
        {
            Blynk.run(); // keep running Blynk to avoid disconnect
            mqtt.loop();
        }

        sendPumpCommand(false);
        Blynk.virtualWrite(VPIN_REMOTE_BTN, 0);
        Serial.println("Auto-off pump completed\n");
    }
}

// Blynk connected handler
BLYNK_CONNECTED()
{
    Serial.println(">>> Blynk Connected!");
    blynkConnected = true;
    updateBlynkDashboard(); // now safe because Mutex was created at the beginning of setup
    Blynk.syncVirtual(VPIN_REMOTE_BTN);
}

// Blynk disconnected handler
BLYNK_DISCONNECTED()
{
    Serial.println(">>> Blynk Disconnected!");
    blynkConnected = false;
}
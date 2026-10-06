/*
  Internet-Based Remote Control of Home Appliances Using Flask
  ESP32 + MQTT + Relay/LED prototype

  MQTT commands received:
    home/light  -> ON / OFF
    home/fan    -> ON / OFF
    home/geyser -> ON / OFF

  MQTT status published:
    home/status -> JSON

  Prototype safety:
    Use LEDs/resistors or low-voltage loads only.
    Do NOT connect this prototype directly to mains voltage.
*/

#include <WiFi.h>
#include <PubSubClient.h>

// ---------- Wi-Fi ----------
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// ---------- MQTT ----------
const char* MQTT_SERVER = "192.168.1.100";   // Change to broker IP/hostname
const int MQTT_PORT = 1883;
const char* MQTT_USER = "";                  // Optional
const char* MQTT_PASSWORD = "";              // Optional

const char* TOPIC_LIGHT  = "home/light";
const char* TOPIC_FAN    = "home/fan";
const char* TOPIC_GEYSER = "home/geyser";
const char* TOPIC_STATUS = "home/status";

// ---------- GPIO ----------
// Change these pins to match your wiring.
const int LIGHT_PIN  = 25;
const int FAN_PIN    = 26;
const int GEYSER_PIN = 27;

// Most relay modules are active LOW.
// For LED prototype, set this to false if your LEDs behave normally.
const bool RELAY_ACTIVE_LOW = true;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

bool lightState = false;
bool fanState = false;
bool geyserState = false;

unsigned long lastStatusPublish = 0;
const unsigned long STATUS_INTERVAL_MS = 5000;

void writeDevice(int pin, bool state) {
  if (RELAY_ACTIVE_LOW) {
    digitalWrite(pin, state ? LOW : HIGH);
  } else {
    digitalWrite(pin, state ? HIGH : LOW);
  }
}

void publishStatus() {
  char payload[300];

  snprintf(
    payload,
    sizeof(payload),
    "{\"devices\":{\"light\":\"%s\",\"fan\":\"%s\",\"geyser\":\"%s\"},\"sensors\":{\"temperature\":null,\"humidity\":null,\"gas\":\"NORMAL\"}}",
    lightState ? "ON" : "OFF",
    fanState ? "ON" : "OFF",
    geyserState ? "ON" : "OFF"
  );

  mqttClient.publish(TOPIC_STATUS, payload, true);
  Serial.print("Published status: ");
  Serial.println(payload);
}

void handleCommand(const char* topic, const String& command) {
  bool state;

  if (command == "ON") {
    state = true;
  } else if (command == "OFF") {
    state = false;
  } else {
    Serial.println("Unknown command");
    return;
  }

  if (strcmp(topic, TOPIC_LIGHT) == 0) {
    lightState = state;
    writeDevice(LIGHT_PIN, lightState);
  } 
  else if (strcmp(topic, TOPIC_FAN) == 0) {
    fanState = state;
    writeDevice(FAN_PIN, fanState);
  } 
  else if (strcmp(topic, TOPIC_GEYSER) == 0) {
    geyserState = state;
    writeDevice(GEYSER_PIN, geyserState);
  }

  publishStatus();
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String command;

  for (unsigned int i = 0; i < length; i++) {
    command += (char)payload[i];
  }

  command.trim();
  command.toUpperCase();

  Serial.print("MQTT command [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(command);

  handleCommand(topic, command);
}

void connectWiFi() {
  Serial.print("Connecting to Wi-Fi");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Wi-Fi connected. ESP32 IP: ");
  Serial.println(WiFi.localIP());
}

void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT...");

    String clientId = "ESP32-Home-" + String((uint32_t)ESP.getEfuseMac(), HEX);

    bool connected;

    if (strlen(MQTT_USER) > 0) {
      connected = mqttClient.connect(
        clientId.c_str(),
        MQTT_USER,
        MQTT_PASSWORD
      );
    } else {
      connected = mqttClient.connect(clientId.c_str());
    }

    if (connected) {
      Serial.println("connected");

      mqttClient.subscribe(TOPIC_LIGHT);
      mqttClient.subscribe(TOPIC_FAN);
      mqttClient.subscribe(TOPIC_GEYSER);

      publishStatus();
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying in 2 seconds");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(GEYSER_PIN, OUTPUT);

  // Start safely with all outputs OFF.
  lightState = false;
  fanState = false;
  geyserState = false;

  writeDevice(LIGHT_PIN, false);
  writeDevice(FAN_PIN, false);
  writeDevice(GEYSER_PIN, false);

  connectWiFi();

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(512);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!mqttClient.connected()) {
    connectMQTT();
  }

  mqttClient.loop();

  if (millis() - lastStatusPublish >= STATUS_INTERVAL_MS) {
    lastStatusPublish = millis();
    publishStatus();
  }
}

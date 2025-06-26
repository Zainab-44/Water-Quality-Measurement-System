#include <WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DFRobot_ESP_EC.h>
#include <EEPROM.h>

// === WiFi & MQTT Configuration ===
#define WIFI_SSID "NTU FSD"
#define WIFI_PASSWORD ""
#define MQTT_SERVER "10.13.40.21"
#define MQTT_PORT 1883

WiFiClient espClient;
PubSubClient client(espClient);

// === Pin Definitions ===
#define TDS_PIN 4             // Analog pin for TDS sensor
#define ONE_WIRE_BUS 13       // DS18B20 temperature sensor
#define TURBIDITY_PIN 5       // Analog turbidity sensor
#define LED_PIN 2             // LED alert if water is dirty

// === Sensor Libraries Setup ===
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DFRobot_ESP_EC ec;

// === Global Variables ===
float voltage = 0.0;
float ecValue = 0.0;
float temperature = 25.0;
float turbidityVoltage = 0.0;
int turbidityPercent = 0;
unsigned long lastMsg = 0;
const long interval = 5000;

void setup_wifi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected");
  Serial.print("📡 IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("✅ MQTT connected");
    } else {
      Serial.print("❌ Failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(32);
  sensors.begin();
  ec.begin();
  pinMode(LED_PIN, OUTPUT);
  
  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  Serial.println("📡 MQTT Water Quality Monitoring System Started");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > interval) {
    lastMsg = now;

    // === Read Temperature ===
    sensors.requestTemperatures();
    temperature = sensors.getTempCByIndex(0);
    if (temperature == DEVICE_DISCONNECTED_C || temperature < -55 || temperature > 125) {
      Serial.println("⚠️ Temp sensor failed");
      temperature = -1;
    }

    // === Read TDS Voltage & EC ===
    voltage = analogRead(TDS_PIN) / 4095.0 * 3.3;
    ecValue = ec.readEC(voltage, temperature);
    ec.calibration(voltage, temperature);

    // === Read Turbidity Voltage ===
    turbidityVoltage = analogRead(TURBIDITY_PIN) / 4095.0 * 3.3;

    // === Calibrate Turbidity % ===
    float cleanVoltage = 2.5;  // <- Update with your real clean water value
    float dirtyVoltage = 1.2;  // <- Update with dirty value

    if (turbidityVoltage >= cleanVoltage) {
      turbidityPercent = 100;
    } else if (turbidityVoltage <= dirtyVoltage) {
      turbidityPercent = 0;
    } else {
      turbidityPercent = (int)(((turbidityVoltage - dirtyVoltage) / (cleanVoltage - dirtyVoltage)) * 100);
    }

    digitalWrite(LED_PIN, (turbidityPercent < 40) ? HIGH : LOW);

    // === Publish via MQTT ===
    client.publish("esp32/temp", String(temperature, 2).c_str());
    client.publish("esp32/tds_voltage", String(voltage, 2).c_str());
    client.publish("esp32/ec", String(ecValue, 2).c_str());
    client.publish("esp32/turbidity_voltage", String(turbidityVoltage, 2).c_str());
    client.publish("esp32/turbidity_percent", String(turbidityPercent).c_str());

    // === Debug ===
    Serial.println("\n===== 📤 MQTT PUBLISHED DATA =====");
    Serial.print("🌡 Temp: "); Serial.println(temperature);
    Serial.print("🔋 TDS Voltage: "); Serial.println(voltage);
    Serial.print("🌐 EC: "); Serial.println(ecValue);
    Serial.print("🧪 Turbidity Voltage: "); Serial.println(turbidityVoltage);
    Serial.print("✅ Turbidity %: "); Serial.println(turbidityPercent);
    Serial.println("==================================");
  }
}


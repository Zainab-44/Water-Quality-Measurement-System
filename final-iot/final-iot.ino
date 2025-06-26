#include <OneWire.h>
#include <DallasTemperature.h>
#include <DFRobot_ESP_EC.h>
#include <EEPROM.h>

// === Pin Definitions ===
#define TDS_PIN 4             // Analog pin for TDS sensor
#define ONE_WIRE_BUS 13       // DS18B20 temperature sensor
#define TURBIDITY_PIN 5       // Analog turbidity sensor pin
#define LED_PIN 2             // LED lights up if dirty

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

void setup() {
  Serial.begin(115200);
  EEPROM.begin(32);
  sensors.begin();
  ec.begin();
  pinMode(LED_PIN, OUTPUT);

  Serial.println("📡 Water Quality Monitoring Calibration Mode Started...");

  // Confirm DS18B20 Detection
  int devicesFound = sensors.getDeviceCount();
  Serial.print("🔎 DS18B20 sensors found: ");
  Serial.println(devicesFound);

  if (devicesFound == 0) {
    Serial.println("⚠️ DS18B20 NOT DETECTED. Check wiring & pull-up resistor!");
  }
}

void loop() {
  // === Read Temperature ===
  sensors.requestTemperatures();
  temperature = sensors.getTempCByIndex(0);
  if (temperature == DEVICE_DISCONNECTED_C || temperature < -55 || temperature > 125) {
    Serial.println("⚠️ Temperature sensor reading failed!");
    temperature = -1;
  }

  // === Read TDS Voltage & EC ===
  voltage = analogRead(TDS_PIN) / 4095.0 * 3.3;
  ecValue = ec.readEC(voltage, temperature);
  ec.calibration(voltage, temperature);

  // === Read Turbidity Voltage ===
  turbidityVoltage = analogRead(TURBIDITY_PIN) / 4095.0 * 3.3;

  // === Calibrate Turbidity % (Adjust clean/dirty thresholds based on testing) ===
  float cleanVoltage = 2.5;  // <<< Replace with your clean water reading
  float dirtyVoltage = 1.2;  // <<< Replace with your dirty water reading

  if (turbidityVoltage >= cleanVoltage) {
    turbidityPercent = 100;
  } else if (turbidityVoltage <= dirtyVoltage) {
    turbidityPercent = 0;
  } else {
    turbidityPercent = (int)(((turbidityVoltage - dirtyVoltage) / (cleanVoltage - dirtyVoltage)) * 100);
  }

  // === LED Indicator ===
  digitalWrite(LED_PIN, (turbidityPercent < 40) ? HIGH : LOW);

  // === Output All Readings for Calibration ===
  Serial.println("\n===== 🔬 SENSOR CALIBRATION REPORT =====");
  Serial.print("🌡️ Temperature: ");
  Serial.print(temperature, 2);
  Serial.println(" °C");

  Serial.print("🔋 TDS Voltage: ");
  Serial.print(voltage, 2);
  Serial.println(" V");

  Serial.print("🌐 EC Value: ");
  Serial.print(ecValue, 2);
  Serial.println(" ms/cm");

  Serial.print("🧪 Turbidity Voltage: ");
  Serial.print(turbidityVoltage, 2);
  Serial.println(" V");

  Serial.print("✅ Cleanliness: ");
  Serial.print(turbidityPercent);
  Serial.println(" %");

  Serial.print("❌ Dirtiness: ");
  Serial.print(100 - turbidityPercent);
  Serial.println(" %");

  Serial.println("========================================");

  delay(1500);
}

#include <WiFi.h>
#include <coap-simple.h>
#include <DHT.h>
#include <ESP32Servo.h>

// ==== WiFi Credentials ====
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// ==== Pin Configuration ====
#define DHT_PIN 15
#define SOIL_SENSOR_PIN 34
#define LDR_PIN 35
#define FAN_PIN 25
#define PUMP_PIN 26
#define VENT_SERVO_PIN 14

// ==== Constants ====
#define DHTTYPE DHT22
#define SOIL_THRESHOLD 2500
#define TEMP_THRESHOLD 30
#define HUMIDITY_THRESHOLD 80
#define WATER_DURATION 4000

#define VENT_OPEN_ANGLE 90
#define VENT_CLOSED_ANGLE 0

// ==== Object Initialization ====
WiFiUDP udp;
Coap coap(udp);
DHT dht(DHT_PIN, DHTTYPE);
Servo ventServo;

// ==== System State ====
struct GreenhouseState {
  float temperature;
  float humidity;
  int soilMoisture;
  int lightLevel;
  bool fanOn;
  bool pumpOn;
  bool autoMode;
  int irrigationCount;
  unsigned long lastIrrigation;
} state = {0, 0, 0, 0, false, false, true, 0, 0};

// ==== Actuator Functions ====
void startFan() {
  digitalWrite(FAN_PIN, HIGH);
  state.fanOn = true;
  Serial.println("Fan turned ON");
}

void stopFan() {
  digitalWrite(FAN_PIN, LOW);
  state.fanOn = false;
  Serial.println("Fan turned OFF");
}

void startPump() {
  if (!state.pumpOn) {
    digitalWrite(PUMP_PIN, HIGH);
    state.pumpOn = true;
    state.irrigationCount++;
    state.lastIrrigation = millis();
    Serial.println("Pump started");
  }
}

void stopPump() {
  digitalWrite(PUMP_PIN, LOW);
  state.pumpOn = false;
  Serial.println("Pump stopped");
}

// ==== Vent Control ====
void openVent() {
  ventServo.write(VENT_OPEN_ANGLE);
  Serial.println("Vent opened");
}

void closeVent() {
  ventServo.write(VENT_CLOSED_ANGLE);
  Serial.println("Vent closed");
}

// ==== Sensor Reading ====
void readSensors() {
  state.temperature = dht.readTemperature();
  state.humidity = dht.readHumidity();
  state.soilMoisture = analogRead(SOIL_SENSOR_PIN);
  state.lightLevel = analogRead(LDR_PIN);

  if (isnan(state.temperature) || isnan(state.humidity)) {
    Serial.println("ERROR: Failed to read DHT sensor!");
    state.temperature = 0;
    state.humidity = 0;
  }

  Serial.printf("Temperature: %.1f°C | Humidity: %.1f%% | Soil Moisture: %d | Light Level: %d\n",
                state.temperature, state.humidity,
                state.soilMoisture, state.lightLevel);
}

// ==== Automatic Control Logic ====
void autoControl() {
  if (!state.autoMode) return;

  // Fan Control
  if (state.temperature > TEMP_THRESHOLD || state.humidity > HUMIDITY_THRESHOLD) {
    if (!state.fanOn) startFan();
  } else if (state.fanOn) {
    stopFan();
  }

  // Pump Control (soil dry)
  if (state.soilMoisture > SOIL_THRESHOLD && !state.pumpOn) {
    Serial.println("Soil is dry — starting irrigation");
    startPump();
  }

  if (state.pumpOn && millis() - state.lastIrrigation > WATER_DURATION) {
    Serial.println("Irrigation complete — stopping pump");
    stopPump();
  }

  // Vent Control (with hysteresis)
  if (state.temperature > 32) {
    openVent();
  } else if (state.temperature < 28) {
    closeVent();
  }
}

// ==== CoAP Endpoint Handlers ====

// /sensors - GET
void callback_sensors(CoapPacket &packet, IPAddress ip, int port) {
  char response[256];
  snprintf(response, sizeof(response),
           "{\"temp\":%.1f,\"humid\":%.1f,\"soil\":%d,\"light\":%d,"
           "\"fan\":\"%s\",\"pump\":\"%s\",\"mode\":\"%s\",\"count\":%d}",
           state.temperature, state.humidity, state.soilMoisture, state.lightLevel,
           state.fanOn ? "ON" : "OFF",
           state.pumpOn ? "ON" : "OFF",
           state.autoMode ? "AUTO" : "MANUAL",
           state.irrigationCount);

  coap.sendResponse(ip, port, packet.messageid, response, strlen(response),
                    COAP_CONTENT, COAP_APPLICATION_JSON, packet.token, packet.tokenlen);

  Serial.println("[CoAP] Sent sensor data");
}

// /fan - POST
void callback_fan(CoapPacket &packet, IPAddress ip, int port) {
  String payload = String((char*)packet.payload);
  payload.trim();

  if (payload == "ON" || payload == "1") {
    startFan();
    coap.sendResponse(ip, port, packet.messageid, "Fan ON", 6, COAP_CHANGED, COAP_TEXT_PLAIN, packet.token, packet.tokenlen);
  } else if (payload == "OFF" || payload == "0") {
    stopFan();
    coap.sendResponse(ip, port, packet.messageid, "Fan OFF", 7, COAP_CHANGED, COAP_TEXT_PLAIN, packet.token, packet.tokenlen);
  } else {
    coap.sendResponse(ip, port, packet.messageid, "Invalid", 7, COAP_BAD_REQUEST, COAP_TEXT_PLAIN, packet.token, packet.tokenlen);
  }
}

// /pump - POST
void callback_pump(CoapPacket &packet, IPAddress ip, int port) {
  String payload = String((char*)packet.payload);
  payload.trim();

  if (payload == "ON" || payload == "1") {
    startPump();
    coap.sendResponse(ip, port, packet.messageid, "Pump ON", 7, COAP_CHANGED, COAP_TEXT_PLAIN, packet.token, packet.tokenlen);
  } else if (payload == "OFF" || payload == "0") {
    stopPump();
    coap.sendResponse(ip, port, packet.messageid, "Pump OFF", 8, COAP_CHANGED, COAP_TEXT_PLAIN, packet.token, packet.tokenlen);
  } else {
    coap.sendResponse(ip, port, packet.messageid, "Invalid", 7, COAP_BAD_REQUEST, COAP_TEXT_PLAIN, packet.token, packet.tokenlen);
  }
}

// /mode - POST
void callback_mode(CoapPacket &packet, IPAddress ip, int port) {
  String payload = String((char*)packet.payload);
  payload.trim();

  if (payload == "AUTO" || payload == "1") {
    state.autoMode = true;
    coap.sendResponse(ip, port, packet.messageid, "Auto Mode ON", 12, COAP_CHANGED, COAP_TEXT_PLAIN, packet.token, packet.tokenlen);
  } else if (payload == "MANUAL" || payload == "0") {
    state.autoMode = false;
    stopPump();
    stopFan();
    closeVent();
    coap.sendResponse(ip, port, packet.messageid, "Manual Mode ON", 14, COAP_CHANGED, COAP_TEXT_PLAIN, packet.token, packet.tokenlen);
  } else {
    coap.sendResponse(ip, port, packet.messageid, "Invalid", 7, COAP_BAD_REQUEST, COAP_TEXT_PLAIN, packet.token, packet.tokenlen);
  }
}

// ==== Setup ====
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== ESP32 Greenhouse Automation System ===");

  pinMode(FAN_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);
  digitalWrite(PUMP_PIN, LOW);

  dht.begin();

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

ventServo.setPeriodHertz(50);  // Standard servo refresh rate
ventServo.attach(VENT_SERVO_PIN, 500, 2400);  // Standard pulse range
ventServo.write(VENT_CLOSED_ANGLE);

Serial.println("Vent initialized (closed)");

  // WiFi Setup
  Serial.printf("Connecting to WiFi: %s", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // CoAP Setup
  coap.server(callback_sensors, "sensors");
  coap.server(callback_fan, "fan");
  coap.server(callback_pump, "pump");
  coap.server(callback_mode, "mode");
  coap.start();

  Serial.println("\nCoAP Server Online (port 5683)");
  Serial.println("System Ready!");
}

// ==== Main Loop ====
void loop() {
  coap.loop();

  static unsigned long lastRead = 0;
  if (millis() - lastRead > 3000) {
    readSensors();
    autoControl();
    lastRead = millis();
  }
}

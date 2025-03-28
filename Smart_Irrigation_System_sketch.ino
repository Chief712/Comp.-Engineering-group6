#define BLYNK_TEMPLATE_ID "TMPL206TCp1Em"
#define BLYNK_TEMPLATE_NAME "Smart irrigation system"
#define BLYNK_AUTH_TOKEN "ag1rYVQKl2uEU8wm-iqwr1cHLNj3PDJg"

#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <ESP32Servo.h>



char auth[] = "ag1rYVQKl2uEU8wm-iqwr1cHLNj3PDJg";  
char ssid[] = "Wokwi-GUEST";         
char pass[] = "";     


// Pin Definitions
#define MOISTURE_SENSOR 34
#define DHT_PIN 4
#define LED_PIN 2
#define BUTTON_PIN 15


#define servoPin 18

// Thresholds
#define MOISTURE_THRESHOLD 2000
#define TEMPERATURE_THRESHOLD 40
#define HUMIDITY_THRESHOLD 50
#define IRRIGATION_DURATION 10000 // 10 seconds

// Initialize DHT sensor
DHT dht(DHT_PIN, DHT22);
Servo servo1;
// Initialize LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Global variables
float humidity = 0;
float temperature = 0;
int moistureValue = 0;
bool irrigationActive = false;
unsigned long irrigationStartTime = 0;
bool manualOverride = false;

#define VIRTUAL_MOISTURE V1
#define VIRTUAL_TEMPERATURE V2
#define VIRTUAL_HUMIDITY V3
#define VIRTUAL_IRRIGATION V4
#define VIRTUAL_BUTTON V5

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  Blynk.begin("ag1rYVQKl2uEU8wm-iqwr1cHLNj3PDJg", "Wokwi-GUEST", "");

  
  Serial.println("\nWiFi connected!");
 

  // Initialize pins
  pinMode(MOISTURE_SENSOR, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Turn off the LED initially
  digitalWrite(LED_PIN, LOW);

  // Initialize DHT sensor
  dht.begin();

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("IoT Irrigation");
  lcd.setCursor(0, 1);
  lcd.print("System Starting");
  delay(2000);
  servo1.attach(servoPin);
  Serial.println("IoT Irrigation System Started");
}

void loop() {

  Blynk.run();
  
  // Read sensor values
  readSensors();

  // Check button for manual override
  checkButton();

  // Display sensor values on LCD
  updateDisplay();

  // Check if irrigation is needed
  checkIrrigation();

  // Manage irrigation system
  manageIrrigation();

  // Print to serial monitor
  serialOutput();

  // Wait for a while
  delay(1000);
}

void readSensors() {
  // Read moisture value (0-4095 on ESP32)
  moistureValue = analogRead(MOISTURE_SENSOR);

  // Read temperature and humidity
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();

  // Check if the readings are valid
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    humidity = 0.0;
    temperature = 0.0;
  }

  Blynk.virtualWrite(VIRTUAL_MOISTURE, moistureValue);
  Blynk.virtualWrite(VIRTUAL_TEMPERATURE, temperature);
  Blynk.virtualWrite(VIRTUAL_HUMIDITY, humidity);
}

void updateDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Moist:");
  lcd.print(moistureValue);

  lcd.setCursor(0, 1);
  lcd.print("T:");
  lcd.print(temperature, 1);
  lcd.print("C H:");
  lcd.print(humidity, 0);
  lcd.print("%");
}

void checkButton() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    manualOverride = !manualOverride;
    Blynk.virtualWrite(VIRTUAL_BUTTON, manualOverride ? 1 : 0);
    Serial.println("Manual override: " + String(manualOverride ? "ON" : "OFF"));
    delay(300); // Debounce
  }
}

void checkIrrigation() {
  // Check if irrigation is needed based on sensor values or manual override
  if (!irrigationActive && (moistureValue < MOISTURE_THRESHOLD || manualOverride || temperature > TEMPERATURE_THRESHOLD || humidity < HUMIDITY_THRESHOLD )) {
    // Soil is dry or manual override, start irrigation
    irrigationActive = true;
    irrigationStartTime = millis();
    digitalWrite(LED_PIN, HIGH);
    Blynk.virtualWrite(VIRTUAL_IRRIGATION, 1);
    OpenWater();
    Serial.println("Irrigation started");
  }

  // Check if irrigation should be stopped after 5 seconds
  if (irrigationActive && (millis() - irrigationStartTime >= 5000)) {
    // Stop irrigation after 10 seconds
    irrigationActive = false;
    digitalWrite(LED_PIN, LOW); // Turn off the pump LED
    Blynk.virtualWrite(VIRTUAL_IRRIGATION, 0);
    CloseWater();
    Serial.println("Irrigation stopped after 5 seconds");
  }
}

void manageIrrigation() {
  // Check if irrigation should be stopped
  if (irrigationActive && (millis() - irrigationStartTime >= IRRIGATION_DURATION) && !manualOverride && moistureValue > MOISTURE_THRESHOLD && temperature < TEMPERATURE_THRESHOLD && humidity < HUMIDITY_THRESHOLD) {
    // Irrigation time is over and not in manual mode, stop irrigation
    irrigationActive = false;
    digitalWrite(LED_PIN, LOW);
    CloseWater();
    Blynk.virtualWrite(VIRTUAL_IRRIGATION, 0);
    Serial.println("Irrigation stopped");
  }

  // Keep irrigation active if in manual mode
  if (manualOverride) {
    digitalWrite(LED_PIN, HIGH);
    ManualOverrideWater();
  }
}

void serialOutput() {
  Serial.println("--------------------");
  Serial.println("Moisture: " + String(moistureValue));
  Serial.println("Temperature: " + String(temperature) + "°C");
  Serial.println("Humidity: " + String(humidity) + "%");
  Serial.println("Irrigation: " + String(irrigationActive ? "ON" : "OFF"));
  Serial.println("--------------------");
}

BLYNK_WRITE(VIRTUAL_BUTTON) {
  manualOverride = param.asInt();
  Serial.println("Manual Override from Blynk: " + String(manualOverride ? "ON" : "OFF"));
}

void OpenWater() {
  servo1.write(0);   // Fully open the valve
  Serial.println("Servo at 0° - Water is flowing");
}

void CloseWater() {
  servo1.write(90);  // Neutral position (closed)
  Serial.println("Servo at 90° - Water flow stopped");
}

void ManualOverrideWater() {
  servo1.write(180); // Fully open in manual mode
  Serial.println("Servo at 180° - Manual Water Control Active");
}


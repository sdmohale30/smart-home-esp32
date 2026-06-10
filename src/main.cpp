#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

// ==========================================
// PIN DEFINITIONS (Matched to your wiring map)
// ==========================================
#define DHTPIN 4
#define DHTTYPE DHT22
#define RELAY1_PIN 26 // Fan (Temp control)
#define RELAY2_PIN 27 // Humidifier (Humidity control)
#define BUTTON_PIN 0  // Manual override button

// ==========================================
// OLED CONFIGURATION
// ==========================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ==========================================
// STATE MACHINE DEFINITIONS
// ==========================================
enum SystemState {
  STATE_IDLE,
  STATE_AUTO,
  STATE_MANUAL,
  STATE_ALARM
};
SystemState currentState = STATE_AUTO; // Start in auto mode
int displayPage = 0; // 0 = Sensor Dashboard, 1 = Relay Status

// ==========================================
// GLOBAL VARIABLES
// ==========================================
DHT dht(DHTPIN, DHTTYPE);

// Non-blocking timers
unsigned long previousMillis = 0;
const long interval = 2000; // 2 seconds for DHT22

// Sensor data struct
struct SensorData {
  float temp;
  float humidity;
} currentData;

// Button interrupt variables
volatile bool buttonPressed = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

// ==========================================
// INTERRUPT SERVICE ROUTINE (ISR)
// ==========================================
void IRAM_ATTR handleButtonPress() {
  unsigned long currentTime = millis();
  if ((currentTime - lastDebounceTime) > debounceDelay) {
    buttonPressed = true;
    lastDebounceTime = currentTime;
  }
}

// ==========================================
// SETUP
// ==========================================
void setup() {
  Serial.begin(115200);

  // Initialize Relays (Active-LOW as per your notes)
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, HIGH); // HIGH = OFF
  digitalWrite(RELAY2_PIN, HIGH); // HIGH = OFF

  // Initialize Button with internal pull-up
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonPress, FALLING);

  // Initialize DHT
  dht.begin();

  // Initialize OLED (I2C pins SDA=21, SCL=22 are default on ESP32)
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Loop forever if display fails
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,20);
  display.println("System Booting...");
  display.display();
  delay(1000);
}

// ==========================================
// MAIN LOOP
// ==========================================
void loop() {
  readSensors();
  handleSerialCommands();
  
  // Handle Button Press (Toggle Display Page or State)
  if (buttonPressed) {
    buttonPressed = false;
    displayPage = !displayPage; // Toggle between page 0 and 1
    updateDisplay(); // Refresh immediately on state change
  }

  runStateMachine();
}

// ==========================================
// FUNCTIONS
// ==========================================

void readSensors() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float newTemp = dht.readTemperature();
    float newHum = dht.readHumidity();

    if (!isnan(newTemp) && !isnan(newHum)) {
      currentData.temp = newTemp;
      currentData.humidity = newHum;
      updateDisplay(); // Update screen when new data arrives
    } else {
      Serial.println("Failed to read from DHT sensor!");
    }
  }
}

void runStateMachine() {
  switch (currentState) {
    case STATE_IDLE:
      // Do nothing, wait for commands
      break;

    case STATE_AUTO:
      // Automation rules with hysteresis
      // Relay 1 (Fan) - turn on if temp > 28, off if temp < 26
      if (currentData.temp > 28.0) {
        digitalWrite(RELAY1_PIN, LOW); // ON
      } else if (currentData.temp < 26.0) {
        digitalWrite(RELAY1_PIN, HIGH); // OFF
      }

      // Relay 2 (Humidifier) - turn on if humidity < 40, off if > 45
      if (currentData.humidity < 40.0) {
        digitalWrite(RELAY2_PIN, LOW); // ON
      } else if (currentData.humidity > 45.0) {
        digitalWrite(RELAY2_PIN, HIGH); // OFF
      }
      break;

    case STATE_MANUAL:
      // Relays are controlled purely via Serial commands
      break;

    case STATE_ALARM:
      // Safety shutdown
      digitalWrite(RELAY1_PIN, HIGH);
      digitalWrite(RELAY2_PIN, HIGH);
      break;
  }
}

void updateDisplay() {
  display.clearDisplay();
  display.setCursor(0, 0);

  if (displayPage == 0) {
    // Page 0: Sensor Dashboard
    display.setTextSize(1);
    display.println("--- SENSOR DATA ---");
    display.println("");
    display.setTextSize(2);
    display.print("T: "); display.print(currentData.temp, 1); display.println("C");
    display.print("H: "); display.print(currentData.humidity, 1); display.println("%");
  } else {
    // Page 1: Relay Status & Mode
    display.setTextSize(1);
    display.println("--- SYSTEM STATUS ---");
    display.println("");
    
    display.print("Mode: ");
    if (currentState == STATE_AUTO) display.println("AUTO");
    else if (currentState == STATE_MANUAL) display.println("MANUAL");
    else if (currentState == STATE_IDLE) display.println("IDLE");
    
    display.println("");
    display.print("Relay 1 (Fan): ");
    display.println(digitalRead(RELAY1_PIN) == LOW ? "ON" : "OFF");
    display.print("Relay 2 (Hum): ");
    display.println(digitalRead(RELAY2_PIN) == LOW ? "ON" : "OFF");
  }
  display.display();
}

void handleSerialCommands() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim(); // Remove whitespace/newlines

    if (command == "MODE AUTO") {
      currentState = STATE_AUTO;
      Serial.println("{\"status\":\"Mode set to AUTO\"}");
    } 
    else if (command == "MODE MANUAL") {
      currentState = STATE_MANUAL;
      Serial.println("{\"status\":\"Mode set to MANUAL\"}");
    }
    else if (currentState == STATE_MANUAL && command == "RELAY1 ON") {
      digitalWrite(RELAY1_PIN, LOW);
      Serial.println("{\"relay1\":\"1\"}");
    }
    else if (currentState == STATE_MANUAL && command == "RELAY1 OFF") {
      digitalWrite(RELAY1_PIN, HIGH);
      Serial.println("{\"relay1\":\"0\"}");
    }
    else if (command == "GET TEMP") {
      // JSON Response as requested in your notes
      Serial.print("{\"temp\":");
      Serial.print(currentData.temp);
      Serial.print(",\"rh\":");
      Serial.print(currentData.humidity);
      Serial.print(",\"relay1\":");
      Serial.print(digitalRead(RELAY1_PIN) == LOW ? "1" : "0");
      Serial.println("}");
    }
    updateDisplay(); // Refresh UI in case mode or relays changed
  }
}

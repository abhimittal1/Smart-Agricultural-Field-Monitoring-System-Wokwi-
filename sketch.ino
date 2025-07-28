// Pin Definitions
#define SOIL_PIN 36          // Soil moisture sensor
#define LIGHT_PIN 39         // Light sensor (LDR)
#define TRIG_PIN 18          // Ultrasonic trigger
#define ECHO_PIN 19          // Ultrasonic echo
#define SERVO_PIN 13         // Servo motor
#define RED_LED 25           // Red status LED
#define GREEN_LED 26         // Green status LED
#define BUZZER_PIN 14        // Buzzer
#define BUTTON_PIN 12        // Manual button
#define THRESHOLD_PIN 34     // Potentiometer

// Variables
int soilMoisture = 0;
int lightLevel = 0;
int waterDistance = 0;
int threshold = 30;
bool irrigating = false;
bool buttonState = false;
unsigned long lastRead = 0;
unsigned long irrigationStart = 0;

void setup() {
  Serial.begin(115200);
  
  // Setup pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Setup servo pin for basic PWM
  pinMode(SERVO_PIN, OUTPUT);
  
  // Initialize - servo closed, green LED on
  servoWrite(0);  // Valve closed
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, HIGH);
  
  // Startup beep
  digitalWrite(BUZZER_PIN, HIGH);
  delay(200);
  digitalWrite(BUZZER_PIN, LOW);
  
  Serial.println("==================================");
  Serial.println("  SMART FARM MONITORING SYSTEM");
  Serial.println("     Remote Sensing Ready");
  Serial.println("==================================");
  Serial.println("Time,Soil%,Light%,Water_cm,Status");
  Serial.println("----------------------------------");
}

void loop() {
  // Read sensors every 1 second
  if (millis() - lastRead >= 1000) {
    readAllSensors();
    checkConditions();
    displayData();
    lastRead = millis();
  }
  
  // Check manual button
  if (digitalRead(BUTTON_PIN) == LOW && !buttonState) {
    buttonState = true;
    manualWatering();
    delay(200);
  } else if (digitalRead(BUTTON_PIN) == HIGH) {
    buttonState = false;
  }
  
  // Stop irrigation after 3 seconds
  if (irrigating && (millis() - irrigationStart > 3000)) {
    stopWatering();
  }
  
  delay(100);
}

void readAllSensors() {
  // Read soil moisture (0-100%)
  int soilRaw = analogRead(SOIL_PIN);
  soilMoisture = map(soilRaw, 0, 4095, 100, 0); // Higher reading = more moisture
  soilMoisture = constrain(soilMoisture, 0, 100);
  
  // Read light level (0-100%)
  int lightRaw = analogRead(LIGHT_PIN);
  lightLevel = map(lightRaw, 0, 4095, 0, 100);
  lightLevel = constrain(lightLevel, 0, 100);
  
  // Read water tank level using ultrasonic
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH, 20000);
  if (duration > 0) {
    waterDistance = duration * 0.034 / 2;
    waterDistance = constrain(waterDistance, 2, 100);
  }
  
  // Read threshold setting
  int thresholdRaw = analogRead(THRESHOLD_PIN);
  threshold = map(thresholdRaw, 0, 4095, 20, 70);
}

void checkConditions() {
  // Auto irrigation: dry soil + water available
  if (soilMoisture < threshold && waterDistance < 40 && !irrigating) {
    startWatering();
    Serial.println(">>> AUTO IRRIGATION STARTED <<<");
  }
  
  // Update status LEDs
  if (irrigating) {
    // Blink red when watering
    digitalWrite(RED_LED, (millis() / 500) % 2);
    digitalWrite(GREEN_LED, LOW);
  } else if (soilMoisture < threshold) {
    // Red = needs water
    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);
    // Warning beep every 5 seconds
    if (millis() % 5000 < 100) {
      tone(BUZZER_PIN, 800, 100);
    }
  } else if (waterDistance > 40) {
    // Blink both = low water tank
    digitalWrite(RED_LED, (millis() / 1000) % 2);
    digitalWrite(GREEN_LED, (millis() / 1000) % 2);
  } else {
    // Green = all good
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, HIGH);
  }
}

void displayData() {
  // Beautiful console output
  Serial.print(millis() / 1000);
  Serial.print("s,");
  Serial.print(soilMoisture);
  Serial.print("%,");
  Serial.print(lightLevel);
  Serial.print("%,");
  Serial.print(waterDistance);
  Serial.print("cm,");
  
  // Status
  if (irrigating) {
    Serial.println("WATERING");
  } else if (soilMoisture < threshold) {
    Serial.println("DRY_SOIL");
  } else if (waterDistance > 40) {
    Serial.println("LOW_TANK");
  } else {
    Serial.println("NORMAL");
  }
  
  // Dashboard every 10 readings
  if ((millis() / 1000) % 10 == 0) {
    printDashboard();
  }
}

void printDashboard() {
  Serial.println();
  Serial.println("┌─────────────────────────────────┐");
  Serial.println("│       FARM SENSOR DASHBOARD    │");
  Serial.println("├─────────────────────────────────┤");
  Serial.print("│ 💧 Soil Moisture: ");
  Serial.print(soilMoisture);
  Serial.print("%");
  if (soilMoisture < 30) Serial.println(" [LOW] 🔴│");
  else if (soilMoisture > 70) Serial.println(" [HIGH] 🔵│");
  else Serial.println(" [OK] 🟢 │");
  
  Serial.print("│ ☀️  Light Level: ");
  Serial.print(lightLevel);
  Serial.print("%");
  if (lightLevel > 80) Serial.println(" [BRIGHT]│");
  else if (lightLevel < 20) Serial.println(" [DARK]  │");
  else Serial.println(" [NORMAL]│");
  
  Serial.print("│ 🚰 Water Tank: ");
  Serial.print(waterDistance);
  Serial.print("cm");
  if (waterDistance > 40) Serial.println(" [LOW] ⚠️ │");
  else Serial.println(" [OK] ✅  │");
  
  Serial.print("│ 🎛️  Threshold: ");
  Serial.print(threshold);
  Serial.println("%           │");
  
  Serial.print("│ 🤖 System: ");
  if (irrigating) {
    Serial.println("WATERING 💦    │");
  } else {
    Serial.println("MONITORING 👁️   │");
  }
  Serial.println("└─────────────────────────────────┘");
  Serial.println("Controls: [BUTTON] Manual | [POT] Threshold");
  Serial.println();
}

void startWatering() {
  if (waterDistance < 40) {  // Only if water available
    irrigating = true;
    irrigationStart = millis();
    servoWrite(90);  // Open valve
    tone(BUZZER_PIN, 1200, 300);
    Serial.println("🚿 IRRIGATION VALVE OPENED");
  }
}

void stopWatering() {
  irrigating = false;
  servoWrite(0);   // Close valve
  tone(BUZZER_PIN, 800, 200);
  Serial.println("🛑 IRRIGATION VALVE CLOSED");
}

void manualWatering() {
  Serial.println("👆 MANUAL OVERRIDE PRESSED!");
  if (waterDistance < 40) {
    startWatering();
    tone(BUZZER_PIN, 1500, 200);
  } else {
    Serial.println("❌ Cannot irrigate - Water tank too low!");
    tone(BUZZER_PIN, 400, 800);
  }
}

// Simple servo control using basic PWM
void servoWrite(int angle) {
  int pulseWidth = map(angle, 0, 180, 544, 2400);
  for (int i = 0; i < 10; i++) {
    digitalWrite(SERVO_PIN, HIGH);
    delayMicroseconds(pulseWidth);
    digitalWrite(SERVO_PIN, LOW);
    delay(20 - pulseWidth / 1000);
  }
}

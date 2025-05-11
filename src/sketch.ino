const int DOOR_BUTTON_PIN = 2;
const int PIR_PIN = 3;
const int ALARM_LED_PIN = 4;
const int ALARM_BUZZER_PIN = 5;
const int TEMP_SENSOR_PIN = A1;

// System states
bool systemArmed = false;
bool doorState = false;
bool doorHasOpened = false;
bool armingPhaseActive = false;
bool alarmTriggered = false;

// Timers
unsigned long doorOpenTime = 0;
unsigned long doorClosedTime = 0;
unsigned long lastMotionTime = 0;
unsigned long lastPirChange = 0;
unsigned long lastStatusUpdate = 0;

// Configuration
const unsigned long motionTimeout = 5000;  // 5s for testing
const unsigned long pirDebounce = 200;     // 200ms debounce
bool currentPirState = LOW;

float readTemperature() {
  int rawValue = analogRead(TEMP_SENSOR_PIN);
  float voltage = rawValue * (5.0 / 1023.0);
  return voltage * 100;
}

void printStatus() {
  Serial.print(F("[STATUS] Door:"));
  Serial.print(doorState ? F("OPEN") : F("CLOSED"));
  Serial.print(F("|System:"));
  Serial.print(systemArmed ? F("ARMED") : F("DISARMED"));
  Serial.print(F("|ArmingPhase:"));
  Serial.print(armingPhaseActive ? F("ACTIVE") : F("INACTIVE"));
  Serial.print(F("|Motion:"));
  Serial.print(currentPirState ? F("DETECTED") : F("NONE"));
  Serial.print(F("|Alarm:"));
  Serial.print(alarmTriggered ? F("TRIGGERED") : F("OK"));
  Serial.print(F("|LED:"));
  Serial.print(digitalRead(ALARM_LED_PIN) ? F("ON") : F("OFF"));
  Serial.print(F("|Temp:"));
  Serial.print(readTemperature());
  Serial.println(F("°C"));
}

void setup() {
  pinMode(DOOR_BUTTON_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(ALARM_LED_PIN, OUTPUT);
  pinMode(ALARM_BUZZER_PIN, OUTPUT);
  pinMode(TEMP_SENSOR_PIN, INPUT);
  
  Serial.begin(9600);
  Serial.println(F("[SYSTEM] Security System Initialised"));
  Serial.println(F("[SYSTEM] Waiting for door to open..."));
  printStatus();
  digitalWrite(ALARM_BUZZER_PIN, LOW);
}

void loop() {
  unsigned long currentMillis = millis();
  
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "RESET_SYSTEM") {
      Serial.println(F("[SYSTEM] Received reset command from edge server"));
      // Simulate door opening to reset the system
      doorState = true;
      doorHasOpened = true;
      doorOpenTime = currentMillis;
      systemArmed = false;
      armingPhaseActive = false;
      alarmTriggered = false;
      digitalWrite(ALARM_LED_PIN, LOW);
      digitalWrite(ALARM_BUZZER_PIN, LOW);
      Serial.println(F("[EVENT] System reset by temperature alert! System DISARMED."));
      printStatus();
    }
  }
  
  // Debounced PIR reading
  bool newPirState = digitalRead(PIR_PIN);
  if (newPirState != currentPirState && 
     (currentMillis - lastPirChange > pirDebounce)) {
    currentPirState = newPirState;
    lastPirChange = currentMillis;
    
    if (currentPirState == HIGH) {
      Serial.println(F("[EVENT] Motion detected"));
    } else {
      Serial.println(F("[EVENT] Motion stopped"));
    }
  }

  // Door button handling
  if (digitalRead(DOOR_BUTTON_PIN) == HIGH && !doorState) {
    doorState = true;
    doorHasOpened = true;
    doorOpenTime = currentMillis;
    systemArmed = false;
    armingPhaseActive = false;
    alarmTriggered = false;
    digitalWrite(ALARM_LED_PIN, LOW);
    digitalWrite(ALARM_BUZZER_PIN, LOW);
    Serial.println(F("[EVENT] Door opened! System DISARMED."));
    printStatus();
  }

  // Auto-close door
  if (doorState && (currentMillis - doorOpenTime > 5000)) {
    doorState = false;
    doorClosedTime = currentMillis;
    armingPhaseActive = true;
    lastMotionTime = currentMillis;
    Serial.println(F("[EVENT] Door closed. 5-second arming phase started."));
    printStatus();
  }

  // Arming logic
  if (doorHasOpened && armingPhaseActive && !systemArmed) {
    if (currentMillis - doorClosedTime <= 5000) {
      if (currentPirState == HIGH) {
        systemArmed = true;
        armingPhaseActive = false;
        lastMotionTime = currentMillis;
        digitalWrite(ALARM_LED_PIN, HIGH);
        Serial.println(F("[EVENT] Motion detected! System ARMED."));
        printStatus();
      }
    } 
    else {
      armingPhaseActive = false;
      Serial.println(F("[EVENT] Arming phase ended (no motion). System remains DISARMED."));
      printStatus();
    }
  }

  // Motion timeout handling
  if (systemArmed && !alarmTriggered) {
    if (currentPirState == HIGH) {
      lastMotionTime = currentMillis;
      Serial.println(F("[ALARM] Motion detected - timer reset"));
    }
    else if (currentMillis - lastMotionTime > motionTimeout) {
      alarmTriggered = true;
      digitalWrite(ALARM_BUZZER_PIN, HIGH);
      Serial.println(F("[ALARM] WARNING! No motion detected - ALARM TRIGGERED!"));
      printStatus();

      // Alarm loop
      while (alarmTriggered) {
        digitalWrite(ALARM_LED_PIN, !digitalRead(ALARM_LED_PIN));
        Serial.println(F("[ALARM] ACTIVE"));
        
        if (digitalRead(DOOR_BUTTON_PIN) == HIGH) {
          doorState = true;
          alarmTriggered = false;
          digitalWrite(ALARM_LED_PIN, LOW);
          digitalWrite(ALARM_BUZZER_PIN, LOW);
          Serial.println(F("[EVENT] Door opened! Alarm stopped."));
          printStatus();
          break;
        }
        delay(200);
      }
    }
  }

  // Status updates
  if (currentMillis - lastStatusUpdate > 1000) {
    if (!alarmTriggered) printStatus();
    lastStatusUpdate = currentMillis;
  }
  
  delay(100);
}
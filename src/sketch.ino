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
unsigned long lastTempCheck = 0;

// Configuration
const unsigned long motionTimeout = 5000;  // 5s for testing
const unsigned long pirDebounce = 200;     // 200ms debounce
bool currentPirState = LOW;
const float TEMP_THRESHOLD = 27.0;

float readTemperature() {
	int rawValue = analogRead(TEMP_SENSOR_PIN);
	float resistance = 10000.0 * ((1023.0 / rawValue) - 1.0);
	float steinhart = resistance / 10000.0;
	steinhart = log(steinhart);
	steinhart /= 3950.0;
	steinhart += 1.0 / (25.0 + 273.15);
	steinhart = 1.0 / steinhart;
	return steinhart - 273.15;
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

void resetAlarm() {
  alarmTriggered = false;
  digitalWrite(ALARM_LED_PIN, LOW);
  digitalWrite(ALARM_BUZZER_PIN, LOW);
  printStatus();
}

void setup() {
  pinMode(DOOR_BUTTON_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(ALARM_LED_PIN, OUTPUT);
  pinMode(ALARM_BUZZER_PIN, OUTPUT);
  pinMode(TEMP_SENSOR_PIN, INPUT);
  
  Serial.begin(9600);
  printStatus();
  digitalWrite(ALARM_BUZZER_PIN, LOW);
}

void loop() {
  unsigned long currentMillis = millis();
  
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "RESET_SYSTEM") {
      doorState = true;
      doorHasOpened = true;
      doorOpenTime = currentMillis;
      systemArmed = false;
      armingPhaseActive = false;
      
      // Reset alarm state if triggered
      if (alarmTriggered) {
        Serial.println(F("[EVENT] Alarm reset by system command"));
        resetAlarm();
      } else {
        alarmTriggered = false;
        digitalWrite(ALARM_LED_PIN, LOW);
        digitalWrite(ALARM_BUZZER_PIN, LOW);
        printStatus();
      }
    }
  }
  
  // Check temperature every second
  if (currentMillis - lastTempCheck > 1000) {
    float currentTemp = readTemperature();
    lastTempCheck = currentMillis;
    
    // If temperature exceeds threshold and alarm is triggered, reset the alarm
    if (currentTemp > TEMP_THRESHOLD && alarmTriggered) {
      Serial.print(F("[EVENT] Alarm reset by high temperature: "));
      Serial.print(currentTemp);
      Serial.println(F("°C"));
      resetAlarm();
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
        printStatus();
      }
    } 
    else {
      armingPhaseActive = false;
      printStatus();
    }
  }

  // Motion timeout handling
  if (systemArmed && !alarmTriggered) {
    if (currentPirState == HIGH) {
      lastMotionTime = currentMillis;
    }
    else if (currentMillis - lastMotionTime > motionTimeout) {
      alarmTriggered = true;
      digitalWrite(ALARM_BUZZER_PIN, HIGH);
      printStatus();

      // Alarm loop
      while (alarmTriggered) {
        digitalWrite(ALARM_LED_PIN, !digitalRead(ALARM_LED_PIN));
        
        // Check for door button press
        if (digitalRead(DOOR_BUTTON_PIN) == HIGH) {
          doorState = true;
          alarmTriggered = false;
          digitalWrite(ALARM_LED_PIN, LOW);
          digitalWrite(ALARM_BUZZER_PIN, LOW);
          printStatus();
          break;
        }
        
        // Check for serial commands during alarm state
        if (Serial.available() > 0) {
          String command = Serial.readStringUntil('\n');
          command.trim();
          
          if (command == "RESET_SYSTEM") {
            Serial.println(F("[EVENT] Alarm reset by system command"));
            doorState = true;
            doorHasOpened = true;
            doorOpenTime = millis();
            systemArmed = false;
            armingPhaseActive = false;
            alarmTriggered = false;
            digitalWrite(ALARM_LED_PIN, LOW);
            digitalWrite(ALARM_BUZZER_PIN, LOW);
            printStatus();
            break;
          }
        }
        
        // Check temperature during alarm state
        float currentTemp = readTemperature();
        if (currentTemp > TEMP_THRESHOLD) {
          Serial.print(F("[EVENT] Alarm reset by high temperature: "));
          Serial.print(currentTemp);
          Serial.println(F("°C"));
          alarmTriggered = false;
          digitalWrite(ALARM_LED_PIN, LOW);
          digitalWrite(ALARM_BUZZER_PIN, LOW);
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
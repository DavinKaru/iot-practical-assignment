const int DOOR_BUTTON_PIN = 2;
const int PIR_PIN = 3;
const int ALARM_LED_PIN = 4;

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

void printStatus() {
  Serial.print("[STATUS] Door: ");
  Serial.print(doorState ? "OPEN" : "CLOSED");
  Serial.print(" | System: ");
  Serial.print(systemArmed ? "ARMED" : "DISARMED");
  Serial.print(" | Arming Phase: ");
  Serial.print(armingPhaseActive ? "ACTIVE" : "INACTIVE");
  Serial.print(" | Motion: ");
  Serial.print(currentPirState ? "DETECTED" : "NONE");
  Serial.print(" | Alarm: ");
  Serial.print(alarmTriggered ? "TRIGGERED" : "OK");
  Serial.print(" | LED: ");
  Serial.println(digitalRead(ALARM_LED_PIN) ? "ON" : "OFF");
}

void setup() {
  pinMode(DOOR_BUTTON_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(ALARM_LED_PIN, OUTPUT);
  
  Serial.begin(9600);
  Serial.println("[SYSTEM] Security System Initialised");
  Serial.println("[SYSTEM] Waiting for door to open...");
  printStatus();
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Debounced PIR reading
  bool newPirState = digitalRead(PIR_PIN);
  if (newPirState != currentPirState && 
     (currentMillis - lastPirChange > pirDebounce)) {
    currentPirState = newPirState;
    lastPirChange = currentMillis;
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
    Serial.println("[EVENT] Door opened! System DISARMED.");
    printStatus();
  }

  // Auto-close door
  if (doorState && (currentMillis - doorOpenTime > 5000)) {
    doorState = false;
    doorClosedTime = currentMillis;
    armingPhaseActive = true;
    lastMotionTime = currentMillis;
    Serial.println("[EVENT] Door closed. 5-second arming phase started.");
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
        Serial.println("[EVENT] Motion detected! System ARMED.");
        printStatus();
      }
    } 
    else {
      armingPhaseActive = false;
      Serial.println("[EVENT] Arming phase ended (no motion). System remains DISARMED.");
      printStatus();
    }
  }

  // Motion timeout handling
  if (systemArmed && !alarmTriggered) {
    if (currentPirState == HIGH) {
      lastMotionTime = currentMillis;
      Serial.println("[ALARM] Motion detected - timer reset");
    }
    else if (currentMillis - lastMotionTime > motionTimeout) {
      alarmTriggered = true;
      Serial.println("[ALARM] WARNING! No motion detected - ALARM TRIGGERED!");
      printStatus();

      // Alarm loop
      while (alarmTriggered) {
        digitalWrite(ALARM_LED_PIN, !digitalRead(ALARM_LED_PIN));
        Serial.println("[ALARM] ACTIVE ");
        
        if (digitalRead(DOOR_BUTTON_PIN) == HIGH) {
          doorState = true;
          alarmTriggered = false;
          digitalWrite(ALARM_LED_PIN, LOW);
          Serial.println("[EVENT] Door opened! Alarm stopped.");
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
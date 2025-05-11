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
const unsigned long motionTimeout = 5000;  // 10s for testing (should be 5 minutes / 300,000)

void setup() {
  pinMode(DOOR_BUTTON_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(ALARM_LED_PIN, OUTPUT);
  
  Serial.begin(9600);
  Serial.println("Security System Initialized");
  Serial.println("Waiting for door to open...");
}

void loop() {
  if (digitalRead(DOOR_BUTTON_PIN) == HIGH && !doorState) {
    doorState = true;
    doorHasOpened = true;
    doorOpenTime = millis();
    systemArmed = false;
    armingPhaseActive = false;
    alarmTriggered = false;
    digitalWrite(ALARM_LED_PIN, LOW);
    Serial.println("Door opened! System DISARMED.");
  }

  // Auto-close door after 5 seconds
  if (doorState && (millis() - doorOpenTime > 5000)) {
    doorState = false;
    doorClosedTime = millis();
    armingPhaseActive = true;
    lastMotionTime = millis();
    Serial.println("Door closed. 5-second arming phase started.");
  }

  // Arming phase logic
  if (doorHasOpened && armingPhaseActive && !systemArmed) {
    if (millis() - doorClosedTime <= 5000) {
      if (digitalRead(PIR_PIN) == HIGH) {
        systemArmed = true;
        armingPhaseActive = false;
        lastMotionTime = millis();  // Start safety motion monitoring
        Serial.println("MOTION DETECTED! System ARMED.");
      }
    } 
    else {
      armingPhaseActive = false;
      Serial.println("No motion detected. System remains DISARMED.");
    }
  }

  // Armed state monitoring
  if (systemArmed && !alarmTriggered) {
    if (digitalRead(PIR_PIN) == HIGH) {
      lastMotionTime = millis();
    }
    
    // Check for motion timeout
    if (millis() - lastMotionTime > motionTimeout) {
      alarmTriggered = true;
      digitalWrite(ALARM_LED_PIN, HIGH);
      Serial.println("ALARM TRIGGERED! No motion detected for 5 minutes.");
    }
  }

  // Status reporting
  Serial.print("Door: ");
  Serial.print(doorState ? "OPEN" : "CLOSED");
  Serial.print(" | System: ");
  Serial.print(systemArmed ? "ARMED" : "DISARMED");
  Serial.print(" | Motion: ");
  Serial.print(digitalRead(PIR_PIN) ? "DETECTED" : "NONE");
  Serial.print(" | Alarm: ");
  Serial.println(alarmTriggered ? "ACTIVE" : "INACTIVE");
  
  delay(1000);
}
const int DOOR_BUTTON_PIN = 2;
const int PIR_PIN = 3;

bool systemArmed = false;
bool doorState = false;
bool doorHasOpened = false;
bool armingPhaseActive = false;
unsigned long doorOpenTime = 0;
unsigned long doorClosedTime = 0;

void setup() {
  pinMode(DOOR_BUTTON_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  Serial.begin(9600);
  Serial.println("System started. Waiting for door to open...");
}

void loop() {
  if (digitalRead(DOOR_BUTTON_PIN) == HIGH && !doorState) {
    doorState = true;
    doorHasOpened = true;
    doorOpenTime = millis();
    systemArmed = false;
    armingPhaseActive = false;
    Serial.println("Door opened!");
  }

  if (doorState && (millis() - doorOpenTime > 5000)) {
    doorState = false;
    doorClosedTime = millis();
    armingPhaseActive = true;
    Serial.println("Door closed. 5-second arming phase starts.");
  }

  if (doorHasOpened && armingPhaseActive && !systemArmed) {
    if (millis() - doorClosedTime <= 5000) {
      if (digitalRead(PIR_PIN) == HIGH) {
        systemArmed = true;
        armingPhaseActive = false;
        Serial.println("MOTION DETECTED! System ARMED.");
      }
    } 
    else {
      armingPhaseActive = false;
      Serial.println("No motion. System REMAINS DISARMED.");
    }
  }

  Serial.print("Door: ");
  Serial.print(doorState ? "OPEN | " : "CLOSED | ");
  Serial.print("System: ");
  Serial.print(systemArmed ? "ARMED" : "DISARMED");
  Serial.print(" | Arming Phase: ");
  Serial.println(armingPhaseActive ? "ACTIVE" : "INACTIVE");
  
  delay(500);
}
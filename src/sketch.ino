const int DOOR_BUTTON_PIN = 2;
unsigned long doorOpenTime = 0;
bool doorState = false;

void setup() {
  pinMode(DOOR_BUTTON_PIN, INPUT);
  Serial.begin(9600);
}

void loop() {
  if (digitalRead(DOOR_BUTTON_PIN) == HIGH && !doorState) {
    doorState = true;
    doorOpenTime = millis();
    Serial.println("Door opened!");
  }

  if (doorState && (millis() - doorOpenTime > 5000)) {
    doorState = false;
    Serial.println("Door closed automatically after 5 seconds");
  }
  
  Serial.print("Door is ");
  if (doorState) {
    Serial.print("open (");
    Serial.print((millis() - doorOpenTime) / 1000);
    Serial.println(" seconds)");
  } else {
    Serial.println("closed");
  }
  
  delay(1000);
}
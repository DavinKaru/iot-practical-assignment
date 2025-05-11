const int DOOR_BUTTON_PIN = 2;
unsigned long doorOpenTime = 0;
bool doorState = false;

void setup() {
  pinMode(DOOR_BUTTON_PIN, INPUT);
  Serial.begin(9600);
}

void loop() {
  if (digitalRead(DOOR_BUTTON_PIN) == HIGH) { // Door opened
    doorState = true;
    doorOpenTime = millis();
    Serial.println("Door opened!");
  }

  if (doorState && (millis() - doorOpenTime > 5000)) {
    doorState = false;
    Serial.println("Door closed automatically after 5 seconds");
  } else {
    Serial.println("Door is ");
    Serial.println(doorState);
  }
  delay(1000);
}
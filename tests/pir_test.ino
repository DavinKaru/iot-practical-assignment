const int PIR_PIN = 3;

void setup() {
  pinMode(PIR_PIN, INPUT);
  Serial.begin(9600);
  Serial.println("PIR Sensor Test - Starting...");
  delay(60 * 1000); //A minute is recommended to stabilise the sensor
}

void loop() {
  int pirState = digitalRead(PIR_PIN);
  Serial.println("It's running");
  if (pirState == HIGH) {
    Serial.println("MOTION DETECTED (HIGH)");
  } else {
    Serial.println("no motion (LOW)");
  }
  delay(500);
}
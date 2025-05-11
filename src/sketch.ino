int btnState = LOW;

void setup() {
  pinMode(5, OUTPUT);
  pinMode(3, INPUT);
}

void loop() {
  btnState = digitalRead(3);
  if(btnState == HIGH) {  // Fixed: use == instead of =
    digitalWrite(5, HIGH);
  } else {
    digitalWrite(5, LOW);
  }
}
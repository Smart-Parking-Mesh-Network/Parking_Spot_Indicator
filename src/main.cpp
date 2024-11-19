#include <Arduino.h>

// Defines
#define BUTTON_PIN 2         
#define DEBOUNCE_DELAY 100   

// Variables
unsigned long lastDebounceTime = 0;
bool lastButtonState = HIGH;

// put function declarations here:
void checkButton();

void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT_PULLUP); 

}

void loop() {
  checkButton();
}

// put function definitions here:
void checkButton() {
  bool currentButtonState = digitalRead(BUTTON_PIN); // Read the current button state
  if (currentButtonState != lastButtonState) {
    lastDebounceTime = millis(); // Update last change time
  }
  // If the button state is stable for the debounce period
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (currentButtonState == LOW) { // Button is pressed
      Serial.println("Button Pressed");
    }
  }
  lastButtonState = currentButtonState; // Update last button state
}
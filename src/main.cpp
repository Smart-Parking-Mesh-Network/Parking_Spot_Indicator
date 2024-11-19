#include <Arduino.h>
#include <LiquidCrystal.h>
#include <SoftwareSerial.h>

// Defines
#define BUTTON_PIN 2         
#define DEBOUNCE_DELAY 100
#define TRIGGER_PIN 6
#define MAX_SECTIONS 10
#define UART_TIMEOUT 2000
#define RX_PIN 3
#define TX_PIN 4             
   
// Variables
unsigned long lastDebounceTime = 0;
bool lastButtonState = HIGH;
SoftwareSerial softSerial(RX_PIN, TX_PIN);

// LCD pin configuration
const int rs = 7, en = 8, d4 = 9, d5 = 10, d6 = 11, d7 = 12;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

struct ParkingSection {
  char section[16];
  int spots;
};
ParkingSection parkingList[MAX_SECTIONS];
int parkingCount = 0;                     
int currentDisplayIndex = 0;              
bool flag = false; 

// Function declarations
void checkButton();
void fetchDataFromNetwork();
void displayNextParking();

void setup() {
  Serial.begin(9600);
  softSerial.begin(9600);
  pinMode(BUTTON_PIN, INPUT_PULLUP); 
  pinMode(TRIGGER_PIN, OUTPUT);        
  digitalWrite(TRIGGER_PIN, HIGH);
  lcd.begin(16, 2);                    
  lcd.print("Press Button");
}

void loop() {
  checkButton();
}

// Function definitions
void checkButton() {
  bool currentButtonState = digitalRead(BUTTON_PIN); 
  if (currentButtonState != lastButtonState) {
    lastDebounceTime = millis(); // Update last change time
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (currentButtonState == LOW) { // Button is pressed
      fetchDataFromNetwork();
    }
  }
  if (flag) {
    Serial.println("Updating Display");
    displayNextParking(); 
    flag = false;
  }
  lastButtonState = currentButtonState; 
}

// Function to fetch data from ESP
void fetchDataFromNetwork() {
  parkingCount = 0; 
  flag = false;
  
  // Trigger the ESP to send data
  digitalWrite(TRIGGER_PIN, LOW); 
  delay(10);                      
  digitalWrite(TRIGGER_PIN, HIGH); 
  lcd.clear();
  lcd.print("Wait...");

  unsigned long startTime = millis();

  while (millis() - startTime < UART_TIMEOUT) {
    if (softSerial.available()) {
      char buffer[32];
      softSerial.readBytesUntil('\n', buffer, sizeof(buffer)); 
      Serial.print(buffer); 

      if (strcmp(buffer, "END") == 0) break;

      // store the received data : A 10
      sscanf(buffer, "%s %d", parkingList[parkingCount].section, &parkingList[parkingCount].spots);
      parkingCount++;
      Serial.println("Data Added");
    }
  }
  lcd.clear();
  if (parkingCount > 0) {
    lcd.print("Data Received");
    flag = true;
  } else {
    lcd.print("No Data Found");
  }
}

// Function to display the next parking section
void displayNextParking() {
  if (parkingCount > 0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sec: ");
    lcd.print(parkingList[currentDisplayIndex].section); 
    lcd.setCursor(0, 1);
    lcd.print("Spots: ");
    lcd.print(parkingList[currentDisplayIndex].spots); 

    // Move to the next section
    currentDisplayIndex = (currentDisplayIndex + 1) % parkingCount;
  } else {
    lcd.clear();
    lcd.print("No Data");
  }
}
#include <Arduino.h>
#include <LiquidCrystal.h>
#include <SoftwareSerial.h>

// Defines
#define BUTTON_PIN 2         
#define DEBOUNCE_DELAY 150
#define TRIGGER_PIN 6
#define MAX_SECTIONS 4
#define UART_TIMEOUT 2000
#define RX_PIN 3
#define TX_PIN 4             
   
// Variables
unsigned long lastDebounceTime = 0;
int lastButtonState = HIGH;
int buttonState;
SoftwareSerial softSerial(RX_PIN, TX_PIN);

// LCD pin configuration
const int rs = 7, en = 8, d4 = 9, d5 = 10, d6 = 11, d7 = 12;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

struct ParkingSection {
  char section[16];
  int spots;
  int entranceScore;
};
ParkingSection parkingList[MAX_SECTIONS];
int parkingCount = 0;                     
int currentDisplayIndex = 0;              
bool flag = false; 

// Function declarations
void checkButton();
void fetchDataFromNetwork();
void displayNextParking();
void clearParkingList();

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
  int currentButtonState = digitalRead(BUTTON_PIN); 
  if (currentButtonState != lastButtonState) {
    lastDebounceTime = millis(); // Update last change time
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if ( currentButtonState != buttonState ) {
      buttonState = currentButtonState;
      if (buttonState == LOW) { // Button is pressed
        lcd.clear();
        lcd.print("Wait...");
        clearParkingList();
        fetchDataFromNetwork();
      }
      }
    }
  if (flag) {
    Serial.println("Updating Display");
    delay(1000);
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


  unsigned long startTime = millis();

  while (millis() - startTime < UART_TIMEOUT) {
    if (softSerial.available()) {
      char buffer[32];
      softSerial.readBytesUntil('\n', buffer, sizeof(buffer)); 
      Serial.println(buffer); 
      if (strcmp(buffer, "NO") > 0) break;
      if (strcmp(buffer, "END") > 0) break;

      // store the received data : A 10
      sscanf(buffer, "%s %d %d", parkingList[parkingCount].section, &parkingList[parkingCount].spots, &parkingList[parkingCount].entranceScore);
      parkingCount++;
      Serial.println("Data Added");
    }
  }
  if (parkingCount > 0) {
    flag = true;
  } else {
    delay(1000);
    lcd.clear();
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
    lcd.print(" Score: ");
    lcd.print(parkingList[currentDisplayIndex].entranceScore);
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

void clearParkingList() {
  for (int i = 0; i < MAX_SECTIONS; i++) {
    memset(parkingList[i].section, 0, sizeof(parkingList[i].section)); // Clear section name
    parkingList[i].spots = 0;         // Reset spots
    parkingList[i].entranceScore = 0; // Reset entrance score
  }
  parkingCount = 0;           // Reset parking count
}
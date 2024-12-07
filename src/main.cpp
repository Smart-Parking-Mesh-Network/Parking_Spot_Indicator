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
#define MAX_RESERVATIONS 10
#define RESERVATION_TIMEOUT 30000
#define SHOW_RESULT 3000

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
  int score;
};
ParkingSection parkingList[MAX_SECTIONS];
struct Reservation {
  char section[16];       
  int spotID;             
  unsigned long expiry;   
};
Reservation reservationList[MAX_RESERVATIONS];
int parkingCount = 0;
int reservationCount = 0;                      
int currentDisplayIndex = 0;              
bool flag = false; 
int selectiveScore = 0; // 1: sort by entranceScore

// Function declarations
void checkButton();
void fetchDataFromNetwork();
void displayParking();
void clearParkingList();
void quickSort(ParkingSection arr[], int low, int high);
void addReservation(const char* sectionName, int spotID);
void manageReservations();
int getReservedCount(const char* sectionName);

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
  manageReservations();
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
        selectiveScore = 1;
        lcd.clear();
        lcd.print("Wait...");
        clearParkingList();
        fetchDataFromNetwork();
      }
      }
    }
  if (flag) {
    Serial.println("Updating Display");
    quickSort(parkingList, 0, parkingCount - 1);
    delay(1000);
    displayParking();
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

      // store the received data : A 10 2
      sscanf(buffer, "%s %d %d", parkingList[parkingCount].section, &parkingList[parkingCount].spots, &parkingList[parkingCount].entranceScore);
      // store the selective score 
      if(selectiveScore == 1) {
        parkingList[parkingCount].score = parkingList[parkingCount].entranceScore;
      }
      parkingCount++;
      Serial.println("Data Added");
    }
  }
  selectiveScore = 0;
  if (parkingCount > 0) {
    flag = true;
  } else {
    delay(1000);
    lcd.clear();
    lcd.print("No Data Found");
    delay(SHOW_RESULT); // time for watching driver
    lcd.clear();
    lcd.print("Press Button");
  }
}

// Function to display the parking section
void displayParking() {
  for (int i = 0; i < parkingCount; i++) {
    int reservedCount = getReservedCount(parkingList[i].section); // تعداد رزروها
    int remainingSpots = parkingList[i].spots - reservedCount;    // تعداد جای خالی باقی‌مانده

    if (remainingSpots > 0) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Sec: ");
      lcd.print(parkingList[i].section);
      lcd.print(" Score: ");
      lcd.print(parkingList[currentDisplayIndex].score);
      lcd.setCursor(0, 1);
      lcd.print("Spot: ");
      lcd.print(remainingSpots);

      addReservation(parkingList[i].section, reservedCount);
      delay(SHOW_RESULT); // time for watching driver
      lcd.clear();
      lcd.print("Press Button"); 
      return;
    }
  }
  lcd.clear();
  lcd.print("No Free Spot");
  delay(SHOW_RESULT); // time for watching driver
  lcd.clear();
  lcd.print("Press Button");
}

void clearParkingList() {
  for (int i = 0; i < MAX_SECTIONS; i++) {
    memset(parkingList[i].section, 0, sizeof(parkingList[i].section)); // Clear section name
    parkingList[i].spots = 0;         // Reset spots
    parkingList[i].entranceScore = 0; // Reset entrance score
    parkingList[i].score = 0;
  }
  parkingCount = 0;           // Reset parking count
  currentDisplayIndex = 0 ;
}

// Swap function to swap two ParkingSection elements
void swap(ParkingSection &a, ParkingSection &b) {
  ParkingSection temp = a;
  a = b;
  b = temp;
}

// Partition function for Quick Sort
int partition(ParkingSection arr[], int low, int high) {
  ParkingSection pivot = arr[high];
  int i = low - 1;

  for (int j = low; j < high; j++) {
    // Check if the current section has spots and compare scores
    if ((arr[j].spots > 0 && pivot.spots == 0) || 
        (arr[j].spots > 0 && arr[j].score < pivot.score)) {
      i++;
      swap(arr[i], arr[j]);
    }
  }
  swap(arr[i + 1], arr[high]);
  return i + 1;
}

// Quick Sort function
void quickSort(ParkingSection arr[], int low, int high) {
  if (low < high) {
    int pi = partition(arr, low, high); // Partition index
    quickSort(arr, low, pi - 1);       // Sort elements before partition
    quickSort(arr, pi + 1, high);      // Sort elements after partition
  }
}

// Get the number of reservations for a section
int getReservedCount(const char* sectionName) {
  int count = 0;
  for (int i = 0; i < reservationCount; i++) {
    if (strcmp(reservationList[i].section, sectionName) == 0) {
      count++;
    }
  }
  return count;
}

// Add a reservation
void addReservation(const char* sectionName, int spotID) {
  if (reservationCount < MAX_RESERVATIONS) {
    strcpy(reservationList[reservationCount].section, sectionName);
    reservationList[reservationCount].spotID = spotID;
    reservationList[reservationCount].expiry = millis() + RESERVATION_TIMEOUT;
    reservationCount++;
  }
}

// Manage and remove expired reservations
void manageReservations() {
  unsigned long currentTime = millis();
  for (int i = 0; i < reservationCount; i++) {
    if (currentTime > reservationList[i].expiry) {
      // Remove expired reservation
      for (int k = i; k < reservationCount - 1; k++) {
        reservationList[k] = reservationList[k + 1];
      }
      reservationCount--;
      i--;
    }
  }
}
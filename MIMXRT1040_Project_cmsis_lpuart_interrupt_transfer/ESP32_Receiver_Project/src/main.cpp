#include <Arduino.h>

/* --- Pin Assignments --- */
#define RXD2 16  // Connect to RT1040 TX
#define TXD2 17  // Connect to RT1040 RX

// The message format: [Start Byte] [Content] [End Byte]
const char* pingMsg = "!PING_123\n"; 
const int expectedBytes = 10;

void setup() {
  Serial.begin(115200);   // To PC Monitor
  
  // Initialize UART2: 115200 Baud, 8-bits, No Parity, 1 Stop Bit
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  
  Serial.println("ESP32 Master Initialized...");
}

void loop() {
  // 1. Send the Ping command to RT1040
  Serial.print("Sending: ");
  Serial.print(pingMsg);
  Serial2.print(pingMsg);

  // 2. Wait for Response (The "Pong")
  // We wait for a maximum of 500ms before giving up (Timeout)
  unsigned long startTime = millis();
  bool gotResponse = false;

  while (millis() - startTime < 500) {
    if (Serial2.available() >= expectedBytes) {
      gotResponse = true;
      break;
    }
  }

  // 3. Handle the Result
  if (gotResponse) {
    Serial.print("Received Pong: ");
    while(Serial2.available()) {
      Serial.print((char)Serial2.read()); // Print chars one by one
    }
  } else {
    Serial.println("Error: Pong Timeout (RT1040 not responding)");
    // Clear buffer in case of partial/corrupt data
    while(Serial2.available()) Serial2.read(); 
  }

  Serial.println("-------------------------");
  delay(2000); // Wait 2 seconds before next Ping
}
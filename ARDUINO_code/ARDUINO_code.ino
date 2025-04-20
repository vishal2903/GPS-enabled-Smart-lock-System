#include <LiquidCrystal.h>

// LCD Setup (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(7, 12, 8, 9, 10, 11);

#define LOCK_PIN 6  // Electromagnetic lock control

void setup() {
    Serial.begin(115200); // Serial communication with ESP32
    lcd.begin(16, 2);
    pinMode(LOCK_PIN, OUTPUT);
    digitalWrite(LOCK_PIN, LOW); // Keep lock closed initially
    lcd.print("Waiting for Auth...");
}

void loop() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        
        if (input == "PASS_OK") {
            lcd.clear();
            lcd.print("Access Granted");
            unlockDoor();
        } else if (input == "PASS_FAIL") {
            lcd.clear();
            lcd.print("Access Denied");
        }
        
        delay(2000);
        lcd.clear();
        lcd.print("Waiting for Auth...");
    }
}

void unlockDoor() {
    digitalWrite(LOCK_PIN, HIGH); // Unlock
    delay(5000); // Keep unlocked for 5 seconds
    digitalWrite(LOCK_PIN, LOW);  // Lock again
}
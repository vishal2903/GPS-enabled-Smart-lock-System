#include <TinyGPS++.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Keypad.h>

// ================= GPS Setup =================
HardwareSerial gpsSerial(2);
TinyGPSPlus gps;

// ================= Wi-Fi and ThingSpeak =================
const char* ssid = "AndroidAP";
const char* wifi_password = "incorrect";
const char* THINGSPEAK_API_KEY = "PCWM2E8OJH94Q0UN";
const char* THINGSPEAK_URL = "http://api.thingspeak.com/update";

WiFiServer server(80);

// ================= Sensor Pins =================
#define VIBRATION_SENSOR_PIN 19
#define IR_SENSOR_PIN 18

// ================= Keypad Setup =================
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {14, 27, 26, 25}; // Adjust as per wiring
byte colPins[COLS] = {33, 32, 35, 34}; // Adjust as per wiring
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ================= Security Setup =================
String enteredPassword = "";
const String correctPassword = "1234"; // Set the correct password

// Destination Coordinates for Authentication
const float DEST_LAT = 12.843089;  // Example: Replace with actual latitude
const float DEST_LON = 80.156880; // Example: Replace with actual longitude
const float GPS_TOLERANCE = 0.0005; // Allow small GPS error margin

bool passwordAccepted = false;
bool gpsAccepted = false;

void setup() {
    Serial.begin(115200);
    gpsSerial.begin(9600, SERIAL_8N1, 16, 17); // GPS RX=16, TX=17

    pinMode(VIBRATION_SENSOR_PIN, INPUT);
    pinMode(IR_SENSOR_PIN, INPUT);

    WiFi.begin(ssid, wifi_password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected. IP Address: " + WiFi.localIP().toString());

    server.begin();

    // 📡 *Setup UART Communication with Arduino*
    Serial1.begin(9600, SERIAL_8N1, 22, 23); // TX=22, RX=23
}

void loop() {
    readGPS();
    checkAuthentication();
    sendSensorDataToThingSpeak();
    handleWebServer();
    handleKeypad();
    delay(100);
}

// ================= GPS Reading Function =================
void readGPS() {
    while (gpsSerial.available() > 0) {
        gps.encode(gpsSerial.read());
    }
}

// ================= Authentication Check =================
void checkAuthentication() {
    float currentLat = gps.location.lat();
    float currentLon = gps.location.lng();

    if (gps.location.isValid()) {
        gpsAccepted = (abs(currentLat - DEST_LAT) <= GPS_TOLERANCE) &&
                      (abs(currentLon - DEST_LON) <= GPS_TOLERANCE);
    } else {
        gpsAccepted = false;
    }

    if (passwordAccepted && gpsAccepted) {
        Serial.println("ACCESS GRANTED - Unlocking Lock!");
        Serial1.println("PASS_OK");  
    } else {
        Serial.println("ACCESS DENIED");
        Serial1.println("ACCESS_DENIED");  
    }
}

// ================= Send Sensor Data to ThingSpeak =================
void sendSensorDataToThingSpeak() {
    int vibrationState = digitalRead(VIBRATION_SENSOR_PIN);
    int irState = !digitalRead(IR_SENSOR_PIN);

    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String url = String(THINGSPEAK_URL) + "?api_key=" + THINGSPEAK_API_KEY;
        url += "&field1=" + String(vibrationState);
        url += "&field2=" + String(irState);

        http.begin(url);
        int httpResponseCode = http.GET();
        if (httpResponseCode > 0) {
            Serial.println("ThingSpeak Update Successful");
        } else {
            Serial.println("ThingSpeak Update Failed");
        }
        http.end();
    } else {
        Serial.println("WiFi Disconnected");
    }
    delay(4000);
}

// ================= Web Server for GPS =================
void handleWebServer() {
    WiFiClient client = server.available();
    if (client) {
        float lat = gps.location.lat();
        float lon = gps.location.lng();
        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                if (c == '\n') break;
            }
        }
        sendGPSPage(client, lat, lon);
        client.stop();
    }
}

void sendGPSPage(WiFiClient& client, float lat, float lon) {
    String page = "<!DOCTYPE html><html><head><title>GPS Tracker</title>"
                  "<style>#map{height:400px;width:100%;}</style></head>"
                  "<body><h1>GPS Location</h1><div id='map'></div>"
                  "<script>function initMap(){"
                  "var options={zoom:12,center:{lat:" + String(lat) + ",lng:" + String(lon) + "}};"
                  "var map=new google.maps.Map(document.getElementById('map'),options);"
                  "var marker=new google.maps.Marker({position:{lat:" + String(lat) + ",lng:" + String(lon) + "},map:map});"
                  "}</script>"
                  "<script async defer src='https://maps.googleapis.com/maps/api/js?key=AIzaSyAw-96GvQ_hSffVC3hOrLj3TJZwMJjZMPw&callback=initMap'></script>"
                  "</body></html>";

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println(page);
}

// ================= Keypad Authentication =================
void handleKeypad() {
    char key = keypad.getKey();
    if (key) {
        Serial.print("Key Pressed: ");
        Serial.println(key);
        if (key == '#') {
            if (enteredPassword == correctPassword) {
                Serial.println("Correct Password");
                passwordAccepted = true;
            } else {
                Serial.println("Wrong Password");
                passwordAccepted = false;
            }
            enteredPassword = "";
        } else if (key == '*') {
            enteredPassword = "";
        } else {
            enteredPassword += key;
        }
    }
}
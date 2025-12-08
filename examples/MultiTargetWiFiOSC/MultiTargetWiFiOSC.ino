/**
 * MultiTargetWiFiOSC.ino
 * 
 * Send radar data over WiFi using OSC protocol.
 * Works with any ESP32 board with WiFi capability.
 * 
 * OSC Messages sent:
 *   /radar/1  (x, y, distance, angle, speed)  - Target 1
 *   /radar/2  (x, y, distance, angle, speed)  - Target 2
 *   /radar/3  (x, y, distance, angle, speed)  - Target 3
 *   /radar/count  (count)                      - Number of valid targets
 * 
 * Hardware:
 * - ESP32 with WiFi (ESP32, ESP32-S2, ESP32-S3, ESP32-C3, etc.)
 * - RD-03D radar connected to Serial1
 * 
 * Dependencies:
 * - WiFi library (built into ESP32 Arduino core)
 * - OSC library by Adrian Freed (install via Library Manager)
 */

#include <RD03D.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>

// ============== CONFIGURATION ==============

// WiFi credentials - EDIT THESE!
const char* WIFI_SSID = "YourNetworkName";
const char* WIFI_PASSWORD = "YourPassword";

// Radar pins
#define RADAR_RX_PIN 20
#define RADAR_TX_PIN 21

// OSC configuration
IPAddress oscTargetIP(192, 168, 1, 100);   // Computer running OSC receiver
const uint16_t oscTargetPort = 8000;
const uint16_t oscLocalPort = 8001;

// Rate limiting
#define MIN_OSC_INTERVAL_MS 20  // ~50 Hz max

// ============== GLOBALS ==============

RD03D radar;
WiFiUDP udp;
bool wifiConnected = false;
uint32_t lastOscTime = 0;

// ============== WIFI SETUP ==============

void connectWiFi() {
    Serial.printf("Connecting to WiFi: %s", WIFI_SSID);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 100) {
        delay(100);
        Serial.print(".");
        timeout++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        Serial.println("\nWiFi connected!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.print("Signal strength (RSSI): ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
    } else {
        wifiConnected = false;
        Serial.println("\nWiFi connection failed!");
    }
}

// ============== OSC SEND ==============

void sendOSC(RD03D_Target* targets, uint8_t count) {
    if (!wifiConnected) {
        // Try to reconnect if disconnected
        if (WiFi.status() != WL_CONNECTED) {
            static uint32_t lastReconnect = 0;
            if (millis() - lastReconnect > 5000) {
                lastReconnect = millis();
                Serial.println("WiFi disconnected, attempting reconnect...");
                WiFi.reconnect();
            }
            return;
        }
        wifiConnected = true;
    }
    
    // Rate limit
    uint32_t now = millis();
    if (now - lastOscTime < MIN_OSC_INTERVAL_MS) return;
    lastOscTime = now;
    
    // Send each valid target
    for (int i = 0; i < 3; i++) {
        if (targets[i].valid) {
            char address[20];
            snprintf(address, sizeof(address), "/radar/%d", i + 1);
            
            OSCMessage msg(address);
            msg.add((int32_t)targets[i].x);
            msg.add((int32_t)targets[i].y);
            msg.add(targets[i].distance);
            msg.add(targets[i].angle);
            msg.add((int32_t)targets[i].speed);
            
            udp.beginPacket(oscTargetIP, oscTargetPort);
            msg.send(udp);
            udp.endPacket();
            msg.empty();
        }
    }
    
    // Send target count
    OSCMessage countMsg("/radar/count");
    countMsg.add((int32_t)count);
    udp.beginPacket(oscTargetIP, oscTargetPort);
    countMsg.send(udp);
    udp.endPacket();
}

// ============== SETUP ==============

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== RD-03D OSC over WiFi ===\n");
    
    // Connect to WiFi
    connectWiFi();
    
    if (wifiConnected) {
        Serial.printf("OSC Target: %s:%d\n", oscTargetIP.toString().c_str(), oscTargetPort);
        udp.begin(oscLocalPort);
    }
    
    // Initialize radar with callback
    Serial.printf("\nRadar: RX=%d, TX=%d\n", RADAR_RX_PIN, RADAR_TX_PIN);
    radar.begin(Serial1, RADAR_RX_PIN, RADAR_TX_PIN);
    radar.onFrame(sendOSC);
    
    Serial.println("\nRadar ready, sending OSC over WiFi...\n");
}

// ============== LOOP ==============

void loop() {
    radar.update();
    
    // Status every 10 seconds
    static uint32_t lastStatus = 0;
    if (millis() - lastStatus > 10000) {
        lastStatus = millis();
        
        // Check WiFi status
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("[Status] Frames: %lu, Errors: %lu, WiFi: OK (RSSI: %d dBm)\n",
                          radar.getFrameCount(),
                          radar.getErrorCount(),
                          WiFi.RSSI());
        } else {
            Serial.printf("[Status] Frames: %lu, Errors: %lu, WiFi: DISCONNECTED\n",
                          radar.getFrameCount(),
                          radar.getErrorCount());
            wifiConnected = false;
        }
    }
}

/**
 * MultiTargetOSC.ino
 * 
 * Send radar data over Ethernet using OSC protocol.
 * Designed for ESP32-P4 with built-in Ethernet, but
 * works with any ESP32 + Ethernet adapter.
 * 
 * OSC Messages sent:
 *   /radar/1  (x, y, distance, angle, speed)  - Target 1
 *   /radar/2  (x, y, distance, angle, speed)  - Target 2
 *   /radar/3  (x, y, distance, angle, speed)  - Target 3
 *   /radar/count  (count)                      - Number of valid targets
 * 
 * Hardware:
 * - ESP32 with Ethernet (ESP32-P4, or ESP32 + W5500/LAN8720)
 * - RD-03D radar connected to Serial1
 * 
 * Dependencies:
 * - OSC library by Adrian Freed (install via Library Manager)
 */

#include <RD03D.h>
#include <ETH.h>
#include <NetworkUdp.h>
#include <OSCMessage.h>

// ============== CONFIGURATION ==============

// Radar pins
#define RADAR_RX_PIN 20
#define RADAR_TX_PIN 21

// Network configuration
IPAddress localIP(169, 254, 166, 20);       // ESP32 static IP
IPAddress subnet(255, 255, 0, 0);
IPAddress oscTargetIP(169, 254, 166, 10);   // Computer running OSC receiver
const uint16_t oscTargetPort = 8000;

// Rate limiting
#define MIN_OSC_INTERVAL_MS 20  // ~50 Hz max

// ============== GLOBALS ==============

RD03D radar;
NetworkUDP udp;
bool ethConnected = false;
uint32_t lastOscTime = 0;

// ============== ETHERNET EVENTS ==============

void onEthEvent(arduino_event_id_t event) {
    switch (event) {
        case ARDUINO_EVENT_ETH_START:
            Serial.println("ETH: Started");
            ETH.setHostname("esp32-radar");
            break;
        case ARDUINO_EVENT_ETH_CONNECTED:
            Serial.println("ETH: Link Up");
            break;
        case ARDUINO_EVENT_ETH_GOT_IP:
            Serial.print("ETH: IP = ");
            Serial.println(ETH.localIP());
            ethConnected = true;
            break;
        case ARDUINO_EVENT_ETH_DISCONNECTED:
            Serial.println("ETH: Disconnected");
            ethConnected = false;
            break;
        default:
            break;
    }
}

// ============== OSC SEND ==============

void sendOSC(RD03D_Target* targets, uint8_t count) {
    if (!ethConnected) return;
    
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
    Serial.println("\n=== RD-03D OSC over Ethernet ===\n");
    
    // Initialize Ethernet
    Network.onEvent(onEthEvent);
    ETH.begin();
    ETH.config(localIP, subnet);
    
    // Wait for connection
    Serial.print("Waiting for Ethernet");
    int timeout = 0;
    while (!ethConnected && timeout < 50) {
        delay(100);
        Serial.print(".");
        timeout++;
    }
    Serial.println();
    
    if (ethConnected) {
        Serial.printf("OSC Target: %s:%d\n", oscTargetIP.toString().c_str(), oscTargetPort);
        udp.begin(8001);
    } else {
        Serial.println("WARNING: Ethernet not connected!");
    }
    
    // Initialize radar with callback
    Serial.printf("Radar: RX=%d, TX=%d\n", RADAR_RX_PIN, RADAR_TX_PIN);
    radar.begin(Serial1, RADAR_RX_PIN, RADAR_TX_PIN);
    radar.onFrame(sendOSC);
    
    Serial.println("\nRadar ready, sending OSC...\n");
}

// ============== LOOP ==============

void loop() {
    radar.update();
    
    // Status every 10 seconds
    static uint32_t lastStatus = 0;
    if (millis() - lastStatus > 10000) {
        lastStatus = millis();
        Serial.printf("[Status] Frames: %lu, Errors: %lu, ETH: %s\n",
                      radar.getFrameCount(),
                      radar.getErrorCount(),
                      ethConnected ? "OK" : "DOWN");
    }
}

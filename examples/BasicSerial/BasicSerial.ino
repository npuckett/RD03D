/**
 * BasicSerial.ino
 * 
 * Beginner-friendly example showing how to read RD-03D radar data
 * and display it in the Serial Monitor.
 * 
 * Hardware:
 * - ESP32 (any variant with hardware UART)
 * - RD-03D radar sensor
 * 
 * Wiring:
 *   ESP32 GPIO20 (RX) ----> Radar TX
 *   ESP32 GPIO21 (TX) ----> Radar RX
 *   ESP32 3.3V        ----> Radar VCC
 *   ESP32 GND         ----> Radar GND
 * 
 * Note: TX and RX are crossed! Radar TX connects to ESP32 RX.
 */

#include <RD03D.h>

// Create the radar object
RD03D radar;

// Pin configuration - change these for your board if needed
const int RADAR_RX_PIN = 20;  // ESP32 receives data on this pin
const int RADAR_TX_PIN = 21;  // ESP32 transmits data on this pin

void setup() {
    // Start the Serial Monitor
    Serial.begin(115200);
    delay(1000);
    
    Serial.println();
    Serial.println("=== RD-03D Radar - Basic Example ===");
    Serial.println();
    
    // Start the radar
    // This connects to the radar on Serial1 using the pins we specified
    radar.begin(Serial1, RADAR_RX_PIN, RADAR_TX_PIN);
    
    Serial.println("Radar started! Waiting for targets...");
    Serial.println();
}

void loop() {
    // IMPORTANT: Call update() frequently to process incoming radar data
    radar.update();
    
    // Only print every 100ms (10 times per second) to avoid flooding the monitor
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint < 100) {
        return;  // Not time to print yet
    }
    lastPrint = millis();
    
    // Get the array of all targets
    RD03D_Target targets[3];
    targets[0] = *radar.getTarget(0);
    targets[1] = *radar.getTarget(1);
    targets[2] = *radar.getTarget(2);
    
    // Count how many targets are valid
    int count = radar.getTargetCount();
    
    // If we have any targets, print them
    if (count > 0) {
        Serial.print("Detected ");
        Serial.print(count);
        Serial.println(" target(s):");
        
        // Check each of the 3 possible targets
        for (int i = 0; i < 3; i++) {
            if (targets[i].valid) {
                Serial.print("  Target ");
                Serial.print(i + 1);
                Serial.print(": ");
                
                // Distance in centimeters
                Serial.print(targets[i].distance, 1);
                Serial.print(" cm");
                
                // Angle from center (negative = left, positive = right)
                Serial.print(" @ ");
                Serial.print(targets[i].angle, 1);
                Serial.print(" deg");
                
                // Position in millimeters
                Serial.print(" | X=");
                Serial.print(targets[i].x);
                Serial.print(" mm, Y=");
                Serial.print(targets[i].y);
                Serial.print(" mm");
                
                // Speed (negative = approaching, positive = moving away)
                Serial.print(" | Speed=");
                Serial.print(targets[i].speed);
                Serial.println(" cm/s");
            }
        }
        Serial.println();
    }
}

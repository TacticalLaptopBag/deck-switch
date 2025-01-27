#include <SoftwareSerial.h>
#include <stdint.h>
#include "Keyboard.h"

// All the Bluetooth device names to detect as a controller
// This array must be terminated with NULL
const char* const names[] = {
    "Xbox Wireless Controller",
    NULL,
};

// All the Bluetooth MAC Addresses that are recognized as controllers
// This array must be terminated with NULL
const char* const mac_addresses[] = {
    NULL,
};

// Pins
#define PIN_RELAY 2
#define PIN_BLE_TX 7
#define PIN_BLE_RX 8

// Baud rates
#define SERIAL_BAUD 9600
#define BLE_BAUD 9600

// 512 is max length of BLE name, 9 is length of incoming AT command response
#define MESSAGE_LEN 512 + 9

// How long (in milliseconds) to press the spacebar when a controller is detected
#define KEY_PRESS_MILLIS 3000
#define HEARTBEAT_TIMEOUT_MILLIS 15000


enum ScanState {
    // Nothing is happening
    IDLE,

    // The HM-10 has been asked to perform a discovery scan
    REQUEST,

    // The HM-10 is currently scanning for BLE devices
    SCANNING,

    // The HM-10 has discovered a MAC address and is currently sending it
    PARSE_MAC,

    // The HM-10 has found the name of a discovered device and is currently sending it
    PARSE_NAME,

    // The HM-10 has finished scanning
    FINISHED,
};

ScanState scanState = ScanState::IDLE;
bool scanContainsController = false;
bool relayOn = false;
uint64_t keyboardPressTime = 0;
uint64_t lastHeartbeatTime = 0;
size_t names_len = 0;
size_t addresses_len = 0;

char lastAddress[13];
char msgBuffer[MESSAGE_LEN];
uint16_t msgLen = 0;


SoftwareSerial ble(PIN_BLE_RX, PIN_BLE_TX);

/**
 * This function is called when a recognized controller is detected
 */
void onControllerDetected() {
    if(!relayOn) {
        // Set heartbeat time to give the Deck some time to start up
        lastHeartbeatTime = millis();

        keyboardPressTime = millis();
        // Press spacebar
        Keyboard.press(32);
    }

    digitalWrite(PIN_RELAY, HIGH);
    relayOn = true;
}

void onConnectionLost() {
    digitalWrite(PIN_RELAY, LOW);
    relayOn = false;
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Keyboard.begin();
    Serial.begin(SERIAL_BAUD);
    ble.begin(BLE_BAUD);

    pinMode(PIN_RELAY, OUTPUT);
    digitalWrite(PIN_RELAY, LOW);

    // Find length of recognized controller names
    const char* currentName = names[0];
    while(currentName != NULL) {
        currentName = names[++names_len];
    }

    // Find length of recognized controller MAC addresses
    const char* currentAddr = mac_addresses[0];
    while(currentAddr != NULL) {
        currentAddr = mac_addresses[++addresses_len];
    }

    delay(1000);

    // Disconnect
    ble.print("AT");
    delay(1000);
    // Master device
    ble.print("AT+ROLE1");
    delay(1000);
    // Only respond to AT commands
    ble.print("AT+IMME1");
    delay(1000);
    // Show names of discovered devices
    ble.print("AT+SHOW1");
    delay(1000);
    // Set scan length
    ble.print("AT+SCAN3");
    delay(1000);

    // Show output of above configs to Serial Monitor
    while(ble.available()) {
        Serial.print((char)ble.read());
    }

    Serial.print("\nReady (names: ");
    Serial.print(names_len);
    Serial.print(", addresses: ");
    Serial.print(addresses_len);
    Serial.println(")");
}

// Discovery flow:
// > AT+DISC?
// < OK+DISCS
// < OK+DIS#:123456789012
// < OK+NAME:-------------------\r\n
// < OK+DIS#:123456789012
// < OK+NAME:---------\r\n
// ...
// < OK+DISCE

void loop() {
    // Release spacebar after a set duration, if spacebar is held
    if(keyboardPressTime > 0 && millis() - keyboardPressTime >= KEY_PRESS_MILLIS) {
        Keyboard.releaseAll();
        keyboardPressTime = 0;
    }

    scanLoop();
    serialLoop();
}

void scanLoop() {
    if(scanState == IDLE) {
        Serial.println("$ Requesting scan...");
        ble.print("AT+DISC?");
        scanState = REQUEST;
        scanContainsController = false;
    } else if(scanState == REQUEST) {
        if(ble.available()) {
            msgBuffer[msgLen++] = (char)ble.read();
        }

        if(msgLen == 8) {
            if(strncmp("OK+DISCS", msgBuffer, 8) == 0) {
                Serial.println("$ Scanning...");
                scanState = SCANNING;
            }

            msgLen = 0;
        }
    } else if(scanState == SCANNING) {
        if(ble.available()) {
            msgBuffer[msgLen++] = (char)ble.read();
        }

        if(msgLen == 8) {
            if(strncmp("OK+DIS", msgBuffer, 6) == 0 && msgBuffer[7] == ':') {
                Serial.println("$ Discovered device, retrieving MAC address...");
                scanState = PARSE_MAC;
            } else if(strncmp("OK+NAME:", msgBuffer, 8) == 0) {
                Serial.println("$ Retrieving device name...");
                scanState = PARSE_NAME;
            } else if(strncmp("OK+DISCE", msgBuffer, 8) == 0) {
                scanState = FINISHED;
            }

            msgLen = 0;
        }
    } else if(scanState == PARSE_MAC) {
        if(ble.available()) {
            msgBuffer[msgLen++] = (char)ble.read();
        }

        if(msgLen == 12) {
            Serial.print("$ MAC address: ");
            strncpy(lastAddress, msgBuffer, 12);
            lastAddress[12] = '\0';
            Serial.println(lastAddress);

            if(!scanContainsController) {
                for(int i = 0; i < addresses_len; i++) {
                    if(strcmp(mac_addresses[i], lastAddress) == 0) {
                        Serial.println("--- CONTROLLER FOUND BY ADDRESS ---");
                        scanContainsController = true;
                        onControllerDetected();
                        break;
                    }
                }
            }

            msgLen = 0;
            scanState = SCANNING;
        }
    } else if(scanState == PARSE_NAME) {
        if(ble.available()) {
            msgBuffer[msgLen++] = (char)ble.read();
        }

        if(msgLen >= 2 && msgBuffer[msgLen-2] == '\r' && msgBuffer[msgLen-1] == '\n') {
            Serial.print("$ Name: ");
            char* name = (char*)calloc(msgLen-1, sizeof(char));
            strncpy(name, msgBuffer, msgLen-2);
            Serial.println(name);

            if(!scanContainsController) {
                for(int i = 0; i < names_len; i++) {
                    size_t len = strlen(names[i]);
                    if(strncasecmp(names[i], name, len) == 0) {
                        Serial.println("--- CONTROLLER FOUND BY NAME ---");
                        scanContainsController = true;
                        onControllerDetected();
                        break;
                    }
                }
            }

            msgLen = 0;
            scanState = SCANNING;

            free(name);
        }
    } else if(scanState == FINISHED) {
        Serial.println("$ Finished scanning.");
        scanState = IDLE;
    }
}

void serialLoop() {
    if(Serial.available()) {
        char rx = (char)Serial.read();
        if(rx == '!') {
            lastHeartbeatTime = millis();
        }
    }

    if(relayOn && millis() - lastHeartbeatTime > HEARTBEAT_TIMEOUT_MILLIS) {
        onConnectionLost();
    }
}

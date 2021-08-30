#include <Arduino.h>
// #include <SoftwareSerial.h>  // Remove if using HardwareSerial
// #include "SoftwareSerial.h"

#include <PubSubClient.h>
#include <WiFi.h>

#include "MHZ19.h"
// #define RX_PIN 10                                          // Rx pin which the MHZ19 Tx pin is attached to
// #define TX_PIN 11                                          // Tx pin which the MHZ19 Rx pin is attached to
#define MHZ19_BAUDRATE 9600  // Device to MH-Z19 Serial baudrate (should not be changed)

//!The pins used by ESP32 are defined in this way:
// #define RX_PIN 33 // Rx pin which the MHZ19 Tx pin is attached to
// #define TX_PIN 32 // Tx pin which the MHZ19 Rx pin is attached to
// int dhtPin = 4;

//!The pins used by ESP32 are defined in this way:
#define RX_PIN 16  // Rx pin which the MHZ19 Tx pin is attached to
#define TX_PIN 17  // Tx pin which the MHZ19 Rx pin is attached to
// int dhtPin = 4;

MHZ19 myMHZ19;  // Constructor for library
// SoftwareSerial mySerial;  // (Uno example) create device to MH-Z19 serial

unsigned long getDataTimer = 0;

const char* ssid = "notwork";
const char* password = "a new router can solve many problems";
// const char* mqtt_server = "iotstack.local";
const char* mqtt_server = "192.168.0.100";


WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {
    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    randomSeed(micros());

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    // Switch on the LED if an 1 was received as first character
    if ((char)payload[0] == '1') {
        digitalWrite(BUILTIN_LED, LOW);  // Turn the LED on (Note that LOW is the voltage level
                                         // but actually the LED is on; this is because
                                         // it is active low on the ESP-01)
    } else {
        digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
    }
}

void reconnect() {
    // Loop until we're reconnected
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Create a random client ID
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);
        // Attempt to connect
        if (client.connect(clientId.c_str())) {
            Serial.println("connected");
            // Once connected, publish an announcement...
            client.publish("co2sensor", "hello co2");
            // ... and resubscribe
            client.subscribe("inTopic");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void setup() {
    delay(2000);
    Serial.begin(9600);  // Device to serial monitor feedback

    // mySerial.begin(MHZ19_BAUDRATE, SWSERIAL_8N1, RX_PIN, TX_PIN);
    // mySerial.enableIntTx(false);

    // mySerial.begin(MHZ19_BAUDRATE);
    // myMHZ19.printCommunication();                            // Error Codes are also included here if found (mainly for debugging/interest)
    // (Uno example) device to MH-Z19 serial start

    // Serial2.begin(9600, SERIAL_8N1);
    Serial2.begin(9600);

    // myMHZ19.begin(mySerial);  // *Serial(Stream) refence must be passed to library begin().
    myMHZ19.begin(Serial2);  // *Serial(Stream) refence must be passed to library begin().

    myMHZ19.autoCalibration();  // Turn auto calibration ON (OFF autoCalibration(false))

    char myVersion[4];
    myMHZ19.getVersion(myVersion);

    Serial.print("\nFirmware Version: ");
    for (byte i = 0; i < 4; i++) {
        Serial.print(myVersion[i]);
        if (i == 1)
            Serial.print(".");
    }
    Serial.println("");

    Serial.print("Range: ");
    Serial.println(myMHZ19.getRange());
    Serial.print("Background CO2: ");
    Serial.println(myMHZ19.getBackgroundCO2());
    Serial.print("Temperature Cal: ");
    Serial.println(myMHZ19.getTempAdjustment());
    Serial.print("ABC Status: ");
    myMHZ19.getABC() ? Serial.println("ON") : Serial.println("OFF");

    pinMode(BUILTIN_LED, OUTPUT);  // Initialize the BUILTIN_LED pin as an output
    // Serial.begin(9600);
    setup_wifi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
}

void loop() {
    int CO2;
    if (millis() - getDataTimer >= 2000) {
        /* note: getCO2() default is command "CO2 Unlimited". This returns the correct CO2 reading even 
        if below background CO2 levels or above range (useful to validate sensor). You can use the 
        usual documented command with getCO2(false) */

        CO2 = myMHZ19.getCO2();  // Request CO2 (as ppm)

        Serial.print("CO2 (ppm): ");
        Serial.println(CO2);

        // int8_t Temp;
        // Temp = myMHZ19.getTemperature();                     // Request Temperature (as Celsius)
        // Serial.print("Temperature (C): ");
        // Serial.println(Temp);

        getDataTimer = millis();

        if (!client.connected()) {
            reconnect();
        }
        client.loop();

        unsigned long now = millis();
        if (now - lastMsg > 2000) {
            lastMsg = now;
            ++value;
            snprintf(msg, MSG_BUFFER_SIZE, "%ld", CO2);
            Serial.print("Publish message: ");
            Serial.println(msg);
            client.publish("co2sensor_01/co2ppm", msg);

            // client.publish("co2sensor", CO2);
            //
        }
    }
}
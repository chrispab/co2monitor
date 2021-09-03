#include <Arduino.h>
// #include <SoftwareSerial.h>  // Remove if using HardwareSerial
// #include "SoftwareSerial.h"

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <SPIFFS.h>
#include <WiFi.h>

#include "DHTesp.h"
#include "LedFader.h"
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

#define ESP32_ONBOARD_BLUE_LED_PIN GPIO_NUM_2  // RHS_P_4 esp32 devkit on board blue LED
#define HEART_BEAT_TIME 500
LedFader heartBeatLED(ESP32_ONBOARD_BLUE_LED_PIN, 1, 0, 255, HEART_BEAT_TIME, true);

unsigned long getDataTimer = 0;

const char* work_ssid = "BTF_Staff";
// const char* work_ssid = "btf_staff";
const char* work_password = "Rei1Vnbu!";
// const char* work_password = "rio2016!";

const char* ssid = "notwork";
const char* password = "a new router can solve many problems";

const char* soft_ap_ssid = "CO2 Monitor";
const char* soft_ap_password = "password";

// const char* mqtt_server = "iotstack.local";
const char* mqtt_server = "192.168.0.100";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// WiFiServer espServer(80); /* Instance of WiFiServer with port number 80 */
/* 80 is the Port Number for HTTP Web Server */
/* A String to capture the incoming HTTP GET Request */
String request;

WiFiClient espClient;
PubSubClient MQTTclient(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

#define MONITOR_SPEED 115200
#define MAX_SAFE_LEVEL 800
#define MAX_MID_LEVEL 1000

#define dhtPin 25
DHTesp dht22;

void setup_wifi() {
    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    // WiFi.mode(WIFI_MODE_APSTA);
    // WiFi.softAP(soft_ap_ssid, soft_ap_password);

    //try work wifi first
    WiFi.begin(work_ssid, work_password);
    //try to connect for 5 secs
    boolean timedOut = false;
    int counter = 0;
    while ((WiFi.status() != WL_CONNECTED) && !timedOut) {
        delay(500);
        Serial.print("try work wifi.");
        counter++;
        if (counter == 5) timedOut = true;
    }

    if (WiFi.status() != WL_CONNECTED) {
        WiFi.begin(ssid, password);

        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print("try home wifi.");
        }
    }

    randomSeed(micros());

    Serial.print("ESP32 IP as soft AP: ");
    Serial.println(WiFi.softAPIP());

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

// MQTT start
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
    // Loop until we're reconnected or timed out
    // try for 3 secs to connect
    boolean timedOut = false;
    int startTime = millis();
    while (!MQTTclient.connected() && !timedOut) {
        Serial.print("Attempting MQTT connection...");
        // Create a random client ID
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);
        // Attempt to connect
        if (MQTTclient.connect(clientId.c_str())) {
            Serial.println("connected to MQTT server");
            // Once connected, publish an announcement...
            MQTTclient.publish("co2sensor", "hello co2");
            // ... and resubscribe
            // MQTTclient.subscribe("inTopic");
        } else {
            Serial.print("failed to connect to MQTT server, rc=");
            Serial.print(MQTTclient.state());
            Serial.println(" try again in 1 seconds");
            // Wait 5 seconds before retrying
            delay(1000);
        }
        if ((millis() - startTime) >= 3000) timedOut = true;
    }
}
//MQTT end

String readCO2Sensor() {
    int CO2ppm = myMHZ19.getCO2();  // Request CO2 (as ppm)
    // float p = bme.readPressure() / 100.0F;
    if (CO2ppm == 0) {
        Serial.println("Failed to read from CO2 sensor!");
        return String(CO2ppm);
    } else {
        // Serial.print("Sensor reads : ");
        // Serial.println(CO2ppm);
        return String(CO2ppm);
    }
}
#define FORMAT_SPIFFS_IF_FAILED true

void setup() {
    delay(2000);
    heartBeatLED.begin();
    dht22.setup(dhtPin, DHTesp::DHT22);
    Serial.begin(MONITOR_SPEED);  // Device to serial monitor feedback

    // Initialize SPIFFS
    if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

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

    // pinMode(BUILTIN_LED, OUTPUT);  // Initialize the BUILTIN_LED pin as an output
    // Serial.begin(9600);
    setup_wifi();
    MQTTclient.setServer(mqtt_server, 1883);
    MQTTclient.setCallback(callback);

    delay(2000);
    Serial.print("\n");

    Serial.print("\n");
    Serial.print("The URL of CO2 monitor Web Server is: ");
    Serial.print("http://");
    Serial.println(WiFi.localIP());
    Serial.print("\n");
    Serial.println("Use the above URL in your Browser to access the CO2 monitor Web Server\n");

    // getDataTimer = millis() - 11000;
    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(SPIFFS, "/index.html");
    });
    server.on("/co2", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "text/plain", readCO2Sensor().c_str());
        Serial.println("temperature readCO2Sensor() called from ajax");
    });
    server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest* request) {
        float temperature = dht22.getTemperature();
        request->send(200, "text/plain", String(temperature));
    });
    server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest* request) {
        float humidity = dht22.getHumidity();
        request->send(200, "text/plain", String(humidity));
    });

    // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    //     request->send(200, "text/plain", "Hello, world");
    // });
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(SPIFFS, "/favicon-32x32.png", "image/png");
    });
    // Start server
    Serial.println("Starting CO2 monitor Web Server...");
    server.begin();
    Serial.println("CO2 monitor Web Server Started");
}

int CO2 = 100;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

void loop() {
    heartBeatLED.update();

    if (millis() - getDataTimer >= 10000) {  //get data every n ms
        /* note: getCO2() default is command "CO2 Unlimited". This returns the correct CO2 reading even 
        if below background CO2 levels or above range (useful to validate sensor). You can use the 
        usual documented command with getCO2(false) */

        CO2 = myMHZ19.getCO2();  // Request CO2 (as ppm)

        Serial.println("Reading Sensors:");
        Serial.print("CO2 (ppm): ");
        Serial.println(CO2);
        Serial.print("Temperature (C): ");
        Serial.println(dht22.getTemperature());
        Serial.print("Humidity (C): ");
        Serial.println(dht22.getHumidity());

        if (!MQTTclient.connected()) {
            reconnect();
        }

        snprintf(msg, MSG_BUFFER_SIZE, "%d", CO2);
        Serial.print("MQTT Publish message: co2sensor_01/co2ppm : ");
        Serial.println(msg);

        if (MQTTclient.connected()) {
            MQTTclient.loop();
            MQTTclient.publish("co2sensor_01/co2ppm", msg);
        } else
            Serial.println("MQTT NOT connected - cant publish CO2");

        // MQTTclient.publish("co2sensor", CO2);
        //

        getDataTimer = millis();
    }
}

// pio run -t uploadfs
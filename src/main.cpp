#include <Arduino.h>
// #include <SoftwareSerial.h>  // Remove if using HardwareSerial
// #include "SoftwareSerial.h"

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
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

const char *work_ssid = "BTF_Staff";
// const char* work_ssid = "btf_staff";
const char *work_password = "Rei1Vnbu!";
// const char* work_password = "rio2016!";

const char *home_ssid = "notwork";
const char *home_password = "a new router can solve many problems";

const char *soft_ap_ssid = "CO2 Monitor";
const char *soft_ap_password = "password";

// const char* mqtt_server = "iotstack.local";
const char *mqtt_server = "192.168.0.100";

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

// REPLACE with your Domain name and URL path or IP address with path
// const char* serverName = "http://example.com/post-data.php";
// const char* serverName = "http://192.168.0.50:8080/post-data.php"; // dell pc
const char *remoteServerIP = "192.168.0.199";

// const char *serverName = "http://192.168.0.199:8080/post-data.php"; // work hp @ home wifi
const char *serverName = "http://dotty.dynu.com:8080/post-data.php";  // work hp @ home wifi

// Keep this API Key value to be compatible with the PHP code provided in the project page.
// If you change the apiKeyValue value, the PHP file /post-data.php also needs to have the same key
String apiKeyValue = "tPmAT5Ab3j7F9";

boolean try_wifi_connect(const char *ssid, const char *password) {
    WiFi.begin(ssid, password);
    //try to connect for 5 secs
    // boolean timedOut = false;
    int counter = 0;
    while ((WiFi.status() != WL_CONNECTED)) {
        if (counter == 20) {
            Serial.print("could not connect to :");
            Serial.println(ssid);
            return false;
        }
        counter++;
        Serial.print("trying to connect to :");
        Serial.println(ssid);

        delay(500);  //wait, it might still connect
    }
    Serial.print("connected to :");
    Serial.println(ssid);
    return true;
}

void setup_wifi() {
    delay(10);
    // We start by connecting to a WiFi network
    // Serial.println();
    // Serial.print("Connecting to ");
    // Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    // or
    // WiFi.mode(WIFI_MODE_APSTA);
    // WiFi.softAP(soft_ap_ssid, soft_ap_password);

    //try work wifi first
    boolean connected = false;
    connected = try_wifi_connect(work_ssid, work_password);
    if (!connected) {
        connected = try_wifi_connect(home_ssid, home_password);
    }
    if (!connected) {
        return;  // restart device
    }

    randomSeed(micros());  //??

    Serial.print("ESP32 IP as soft AP: ");
    Serial.println(WiFi.softAPIP());

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("SSID connected to : ");
    Serial.println(WiFi.SSID());
}

// MQTT start
void callback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    // Switch on the LED if an 1 was received as first character
    // if ((char)payload[0] == '1') {
    //     digitalWrite(BUILTIN_LED, LOW);  // Turn the LED on (Note that LOW is the voltage level
    //                                      // but actually the LED is on; this is because
    //                                      // it is active low on the ESP-01)
    // } else {
    //     digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
    // }
}

void reconnectMQTT() {
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
        if ((millis() - startTime) >= 3000)
            timedOut = true;
    }
}
//MQTT end

//read co2 sensor and return as string
int lastGoodCO2ppm;

String readCO2Sensor() {
    int CO2ppm = myMHZ19.getCO2();  // Request CO2 (as ppm)
    // float p = bme.readPressure() / 100.0F;
    if (CO2ppm == 0) {  //! dont send back 0 - use prev or get till a number
        Serial.println("!!!!!!     Failed to read from CO2 sensor! 0 read");

        //try restarting the serial i/f to sensor
        Serial2.begin(9600);

        // myMHZ19.begin(mySerial);  // *Serial(Stream) refence must be passed to library begin().
        myMHZ19.begin(Serial2);  // *Serial(Stream) refence must be passed to library begin().
        CO2ppm = myMHZ19.getCO2();

        if (CO2ppm > 0) {
            return String(CO2ppm);
        } else {  //return previous good reading made
            return String(lastGoodCO2ppm);
        }
    } else {
        // Serial.print("Sensor reads : ");
        // Serial.println(CO2ppm);
        lastGoodCO2ppm = CO2ppm;
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

    // sofserial setup if using
    // mySerial.begin(MHZ19_BAUDRATE, SWSERIAL_8N1, RX_PIN, TX_PIN);
    // mySerial.enableIntTx(false);
    // mySerial.begin(MHZ19_BAUDRATE);
    // myMHZ19.printCommunication();                            // Error Codes are also included here if found (mainly for debugging/interest)

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

    //ONLY use mqtt if at home
    if (WiFi.SSID() == home_ssid) {
        MQTTclient.setServer(mqtt_server, 1883);
        MQTTclient.setCallback(callback);
    }
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
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(SPIFFS, "/index.html"); });
    server.on("/co2", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/plain", readCO2Sensor().c_str());
        Serial.println("readCO2Sensor() called from ajax");
    });
    server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request) {
        float temperature = dht22.getTemperature();
        request->send(200, "text/plain", String(temperature));
    });
    server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request) {
        float humidity = dht22.getHumidity();
        request->send(200, "text/plain", String(humidity));
    });

    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(SPIFFS, "/favicon-32x32.png", "image/png"); });
    // Start server
    Serial.println("Starting CO2 monitor Web Server...");
    server.begin();
    Serial.println("CO2 monitor Web Server Started");
}

void postDataToRemoteDB(int CO2, float Temperature, float Humidity) {
    //Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClient client;
        HTTPClient http;

        // Your Domain name with URL path or IP address with path
        http.begin(client, serverName);

        // Specify content-type header
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");

        //! trim t, h to 1dp!!
        // Prepare your HTTP POST request data
        // String httpRequestData = "api_key=" + apiKeyValue + "&value1=" + String(CO2) + "&value2=" + String(Temperature) + "&value3=" + String(Humidity) + "";
        String httpRequestData = "api_key=" + apiKeyValue + "&value1=" + String(CO2) + "&value2=" + String(Temperature, 1) + "&value3=" + String(Humidity, 1);
        // Serial.println(String(Temperature, 1));

        Serial.print("remote server");
        Serial.println(serverName);

        Serial.print("httpRequestData: ");
        Serial.println(httpRequestData);

        // You can comment the httpRequestData variable above
        // then, use the httpRequestData variable below (for testing purposes without the BME280 sensor)
        //String httpRequestData = "api_key=tPmAT5Ab3j7F9&value1=24.75&value2=49.54&value3=1005.14";

        // Send HTTP POST request
        int httpResponseCode = http.POST(httpRequestData);

        // If you need an HTTP request with a content type: text/plain
        //http.addHeader("Content-Type", "text/plain");
        //int httpResponseCode = http.POST("Hello, World!");

        // If you need an HTTP request with a content type: application/json, use the following:
        //http.addHeader("Content-Type", "application/json");
        //int httpResponseCode = http.POST("{\"value1\":\"19\",\"value2\":\"67\",\"value3\":\"78\"}");

        if (httpResponseCode > 0) {
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);
        } else {
            Serial.print("Error code: ");
            Serial.println(httpResponseCode);
        }
        // Free resources
        http.end();
    } else {
        Serial.println("WiFi Disconnected");
    }
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

    if (millis() - getDataTimer >= 15000) {  //get data every n ms
        /* note: getCO2() default is command "CO2 Unlimited". This returns the correct CO2 reading even 
        if below background CO2 levels or above range (useful to validate sensor). You can use the 
        usual documented command with getCO2(false) */

        CO2 = readCO2Sensor().toInt();;  // Request CO2 (as ppm)

        Serial.println("Reading Sensors:");
        Serial.print("CO2 (ppm): ");
        Serial.println(CO2);
        Serial.print("Temperature (C): ");
        Serial.println(dht22.getTemperature());
        Serial.print("Humidity (C): ");
        Serial.println(dht22.getHumidity());

        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi is NOT connected - trying wifi_setup()");
            setup_wifi();
        }
        if (WiFi.SSID() == home_ssid) {
            if (!MQTTclient.connected()) {
                reconnectMQTT();
            }

            snprintf(msg, MSG_BUFFER_SIZE, "%d", CO2);
            Serial.print("MQTT Publish message: co2sensor_01/co2ppm : ");
            Serial.println(msg);
            if (MQTTclient.connected()) {
                MQTTclient.loop();
                MQTTclient.publish("co2sensor_01/co2ppm", msg);
            } else
                Serial.println("MQTT NOT connected - cant publish CO2");
        }
        //send the data to the remote db to store
        postDataToRemoteDB(CO2, dht22.getTemperature(), dht22.getHumidity());

        getDataTimer = millis();
    }
}

//to upload /data folder to device SPIFFS
// pio run -t uploadfs
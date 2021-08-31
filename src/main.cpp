#include <Arduino.h>
// #include <SoftwareSerial.h>  // Remove if using HardwareSerial
// #include "SoftwareSerial.h"

#include <PubSubClient.h>
#include <WiFi.h>

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

const char* ssid = "notwork";
const char* password = "a new router can solve many problems";
// const char* mqtt_server = "iotstack.local";
const char* mqtt_server = "192.168.0.100";

WiFiServer espServer(80); /* Instance of WiFiServer with port number 80 */
/* 80 is the Port Number for HTTP Web Server */
/* A String to capture the incoming HTTP GET Request */
String request;

WiFiClient espClient;
PubSubClient MQTTclient(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

#define MAX_SAFE_LEVEL 800
#define MAX_MID_LEVEL 1000

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
    while (!MQTTclient.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Create a random client ID
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);
        // Attempt to connect
        if (MQTTclient.connect(clientId.c_str())) {
            Serial.println("connected");
            // Once connected, publish an announcement...
            MQTTclient.publish("co2sensor", "hello co2");
            // ... and resubscribe
            MQTTclient.subscribe("inTopic");
        } else {
            Serial.print("failed, rc=");
            Serial.print(MQTTclient.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void setup() {
    delay(2000);
    heartBeatLED.begin();
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

    // pinMode(BUILTIN_LED, OUTPUT);  // Initialize the BUILTIN_LED pin as an output
    // Serial.begin(9600);
    setup_wifi();
    MQTTclient.setServer(mqtt_server, 1883);
    MQTTclient.setCallback(callback);

    delay(2000);
    Serial.print("\n");
    Serial.println("Starting ESP32 Web Server...");
    espServer.begin(); /* Start the HTTP web Server */
    Serial.println("ESP32 Web Server Started");
    Serial.print("\n");
    Serial.print("The URL of ESP32 Web Server is: ");
    Serial.print("http://");
    Serial.println(WiFi.localIP());
    Serial.print("\n");
    Serial.println("Use the above URL in your Browser to access ESP32 Web Server\n");

    getDataTimer = millis() - 11000;
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

    if (millis() - getDataTimer >= 10000) {
        /* note: getCO2() default is command "CO2 Unlimited". This returns the correct CO2 reading even 
        if below background CO2 levels or above range (useful to validate sensor). You can use the 
        usual documented command with getCO2(false) */

        CO2 = myMHZ19.getCO2();  // Request CO2 (as ppm)

        Serial.print("CO2 (ppm): ");
        Serial.println(CO2);

        getDataTimer = millis();

        if (!MQTTclient.connected()) {
            reconnect();
        }
        MQTTclient.loop();

        unsigned long now = millis();
        if (now - lastMsg > 2000) {
            lastMsg = now;
            ++value;
            snprintf(msg, MSG_BUFFER_SIZE, "%ld", CO2);
            Serial.print("MQTT Publish message: co2sensor_01/co2ppm : ");
            Serial.println(msg);
            MQTTclient.publish("co2sensor_01/co2ppm", msg);

            // MQTTclient.publish("co2sensor", CO2);
            //
        }
    }

    //!web section

    WiFiClient WebServerclient = espServer.available(); /* Listen for incoming clients */
    if (!WebServerclient) {
        return;
    }

    Serial.println("New WebServerclient!!!");
    boolean currentLineIsBlank = true;

    if (WebServerclient) {  // If a new client connects,
        currentTime = millis();
        previousTime = currentTime;
        // while (WebServerclient.connected()) {
        while (WebServerclient.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
            currentTime = millis();

            if (WebServerclient.available()) {
                char c = WebServerclient.read();
                request += c;
                Serial.write(c);
                /* if you've gotten to the end of the line (received a newline */
                /* character) and the line is blank, the http request has ended, */
                /* so you can send a reply */
                if (c == '\n' && currentLineIsBlank) {
                    /* Extract the URL of the request */
                    /* We have four URLs. If IP Address is 192.168.1.6 (for example),
        * then URLs are: 
        * 192.168.1.6/GPIO16ON
        * 192.168.1.6/GPIO16OFF
        * 192.168.1.6/GPIO17ON
        * 192.168.1.6/GPIO17OFF
        */
                    /* Based on the URL from the request, turn the LEDs ON or OFF */
                    if (request.indexOf("/GPIO16ON") != -1) {
                        Serial.println("GPIO16 LED is ON");
                        // digitalWrite(gpio16LEDPin, HIGH);
                        // gpio16Value = HIGH;
                    }
                    if (request.indexOf("/GPIO16OFF") != -1) {
                        Serial.println("GPIO16 LED is OFF");
                        // digitalWrite(gpio16LEDPin, LOW);
                        // gpio16Value = LOW;
                    }
                    if (request.indexOf("/GPIO17ON") != -1) {
                        Serial.println("GPIO17 LED is ON");
                        // digitalWrite(gpio17LEDPin, HIGH);
                        // gpio17Value = HIGH;
                    }
                    if (request.indexOf("/GPIO17OFF") != -1) {
                        Serial.println("GPIO17 LED is OFF");
                        // digitalWrite(gpio17LEDPin, LOW);
                        // gpio17Value = LOW;
                    }

                    /* HTTP Response in the form of HTML Web Page */
                    WebServerclient.println("HTTP/1.1 200 OK");
                    WebServerclient.println("Content-Type: text/html");
                    WebServerclient.println("Connection: close");
                    WebServerclient.println();  //  IMPORTANT

                    WebServerclient.println("<!DOCTYPE HTML>");
                    WebServerclient.println("<html>");
                    WebServerclient.println("<head>");
                    WebServerclient.println("<meta http-equiv=\"refresh\" content=\"11\">");
                    WebServerclient.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
                    WebServerclient.println("<link rel=\"icon\" href=\"data:,\">");
                    WebServerclient.println("<style>");
                    WebServerclient.println("html { font-family: Courier New; display: inline-block; margin: 0px auto; text-align: center;}");
                    WebServerclient.println(".button {border: none; color: white; padding: 10px 20px; text-align: center;");
                    WebServerclient.println("text-decoration: none; font-size: 25px; margin: 2px; cursor: pointer;}");
                    WebServerclient.println(".button1 {background-color: #FF0000;}");
                    WebServerclient.println(".button2 {background-color: #00FF00;}");

                    // WebServerclient.println("meter { width: 50%; height: 75px; }");
                    WebServerclient.println("meter { width: 50%; height: 75px; border: 1px solid #ccc; border-radius: 3px;}");
                    // WebServerclient.println("meter::-webkit-meter-bar { background: none; background-color: whiteSmoke; box-shadow: 0 5px 5px -5px #333 inset;}");

                    WebServerclient.println("</style>");
                    WebServerclient.println("</head>");
                    WebServerclient.println("<body>");
                    WebServerclient.println("<h2>ESP32 CO2 Monitor</h2>");

                    WebServerclient.print("<meter class=\"co2_meter\" min=\"400\" low=\"800\" high=\"1000\"max=\"1500\" optimum=\"500\" value=\"");
                    WebServerclient.print(CO2);
                    WebServerclient.println("\"></meter>");
                    // if (gpio16Value == LOW) {
                    WebServerclient.print("<h1>CO2 level(ppm): ");
                    WebServerclient.print(CO2);
                    WebServerclient.println("</h1>");
                    if (CO2 <= MAX_SAFE_LEVEL) {
                        WebServerclient.println("<h2>Level is Good</h2>");
                    } else if (CO2 <= MAX_MID_LEVEL) {
                        WebServerclient.println("<h2>Level could be better</h2>");
                    } else if (CO2 > MAX_MID_LEVEL) {
                        WebServerclient.println("<h2>WARNING High Level!</h2>");
                    }
                    WebServerclient.println("</body>");
                    WebServerclient.println("</html>");

                    break;
                }
                if (c == '\n') {
                    currentLineIsBlank = true;
                } else if (c != '\r') {
                    currentLineIsBlank = false;
                }
                // WebServerclient.print("\n");
            }
        }

        delay(2);
        request = "";
        // WebServerclient.flush();
        WebServerclient.stop();
        Serial.println("WebServerclient disconnected");
        Serial.print("\n");
        delay(2);
    }
}
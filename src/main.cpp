#include <Arduino.h>
// #include <SoftwareSerial.h>  // Remove if using HardwareSerial
// #include "SoftwareSerial.h"

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <SPIFFS.h>
#include <WiFi.h>

// ArduinoOTA
// #include <WiFi.h>
// #include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ezAnalogKeypad.h>

#include "DHTesp.h"
#include "Display.h"
#include "LedFader.h"
#include "MHZ19.h"
#include "MyOTA.h"
#include "secrets.h"
#include "version.h"

// #define RX_PIN 10                                          // Rx pin which the MHZ19 Tx pin is attached to
// #define TX_PIN 11                                          // Tx pin which the MHZ19 Rx pin is attached to
#define MHZ19_BAUDRATE 9600  // Device to MH-Z19 Serial baudrate (should not be changed)

//! The pins used by ESP32 are defined in this way:
// #define RX_PIN 33 // Rx pin which the MHZ19 Tx pin is attached to
// #define TX_PIN 32 // Tx pin which the MHZ19 Rx pin is attached to
// int dhtPin = 4;

//! The pins used by ESP32 are defined in this way:
#define RX_PIN 16  // Rx pin which the MHZ19 Tx pin is attached to
#define TX_PIN 17  // Tx pin which the MHZ19 Rx pin is attached to
// int dhtPin = 4;

MHZ19 myMHZ19;  // Constructor for library
// SoftwareSerial mySerial;  // (Uno example) create device to MH-Z19 serial

//! LEDs
#define ESP32_ONBOARD_BLUE_LED_PIN GPIO_NUM_2  // RHS_P_4 esp32 devkit on board blue LED
#define HEART_BEAT_TIME 500
LedFader heartBeatLED(ESP32_ONBOARD_BLUE_LED_PIN, 0, 0, 255, HEART_BEAT_TIME, true);

// #define RED_LED_PIN GPIO_NUM_13  // LHS_P_3 esp32 devkit
#define RED_LED_PIN GPIO_NUM_14  // LHS_P_3 esp32 devkit

#define HEART_BEAT_TIME 500
LedFader highLevelLED(RED_LED_PIN, 1, 0, 255, HEART_BEAT_TIME, true);

// #define GREEN_LED_PIN GPIO_NUM_12  // LHS_P_4 esp32 devkit
#define GREEN_LED_PIN GPIO_NUM_13  // LHS_P_4 esp32 devkit

#define HEART_BEAT_TIME 500
LedFader lowLevelLED(GREEN_LED_PIN, 2, 0, 255, HEART_BEAT_TIME, true);

// #define YELLOW_LED_PIN GPIO_NUM_14  // LHS_P_5 esp32 devkit
#define YELLOW_LED_PIN GPIO_NUM_12  // LHS_P_5 esp32 devkit

#define HEART_BEAT_TIME 500
LedFader mediumLevelLED(YELLOW_LED_PIN, 3, 0, 255, HEART_BEAT_TIME, true);

#define SOUNDER_PIN GPIO_NUM_15  // rHS_P_3 esp32 devkit

#define BUZZER_PIN SOUNDER_PIN

#ifdef ARDUINO_ARCH_ESP32

#define SOUND_PWM_CHANNEL 4
#define SOUND_RESOLUTION 8                      // 8 bit resolution
#define SOUND_ON (1 << (SOUND_RESOLUTION - 1))  // 50% duty cycle
#define SOUND_OFF 0                             // 0% duty cycle

void tone(int pin, int frequency, int duration) {
    ledcSetup(SOUND_PWM_CHANNEL, frequency, SOUND_RESOLUTION);  // Set up PWM channel
    ledcAttachPin(pin, SOUND_PWM_CHANNEL);                      // Attach channel to pin
    ledcWrite(SOUND_PWM_CHANNEL, SOUND_ON);
    delay(duration);
    ledcWrite(SOUND_PWM_CHANNEL, SOUND_OFF);
}
#endif

const char *work_ssid = SSID_1;
const char *work_password = PASSWORD_1;

const char *home_ssid = SSID_2;
const char *home_password = PASSWORD_2;

const char *soft_ap_ssid = SOFT_AP_SSID;
const char *soft_ap_password = SOFT_AP_PASSWORD;

// const char* mqtt_server = "iotstack.local";
const char *mqtt_server = "192.168.0.100";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// WiFiServer espServer(80); /* Instance of WiFiServer with port number 80 */
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

#define KEYPAD_PIN 36               // GPIO36 pyhs pin 14 LHS
ezAnalogKeypad keypad(KEYPAD_PIN);  // create ezAnalogKeypad object that attach to pin KEYPAD_PIN

// REPLACE with your Domain name and URL path or IP address with path
const char *remoteServerIP = "192.168.0.199";

// const char *serverName = "http://192.168.0.199:8080/post-data.php";
const char *serverName = "http://dotty.dynu.com:8080/post-data.php";

// API Key value
String apiKeyValue = "tPmAT5Ab3j7F9";

// #include <Adafruit_GFX.h>
// #include <Adafruit_I2CDevice.h>
// #include <Adafruit_SSD1306.h>
// #include <SPI.h>
// #include <Wire.h>

//******************************** u8g2 start
// create the display object
// #include "Display.h"

#define OLED_CLOCK_PIN GPIO_NUM_22  // RHS_P_14 SCL
#define OLED_DATA_PIN GPIO_NUM_21   // RHS_P_11 SDA
Display myDisplay(U8G2_R0, /* reset=*/U8X8_PIN_NONE, OLED_CLOCK_PIN, OLED_DATA_PIN);
// create the display object
// Display myDisplay(U8G2_R0, /* reset=*/U8X8_PIN_NONE, OLED_CLOCK_PIN, OLED_DATA_PIN);

enum displayModes {
    NORMAL,
    BIG_TEMP,
    MULTI
};

//#define SYS_FONT u8g2_font_8x13_tf
#define SYS_FONT u8g2_font_6x12_tf        // 7 px high
#define BIG_TEMP_FONT u8g2_font_fub30_tf  // 30px hieght
// 33 too big - #define BIG_TEMP_FONT u8g2_font_inb33_mf
//********************************ug82 end

boolean try_wifi_connect(const char *ssid, const char *password) {
    char src[50], dest[50];
    strcpy(src, ".");
    strcpy(dest, "");

    WiFi.begin(ssid, password);
    // try to connect for 10 secs
    myDisplay.wipe();
    myDisplay.writeLine("trying to connect..", 1);
    myDisplay.writeLine(ssid, 2);
    myDisplay.refresh();
    int counter = 0;
    while ((WiFi.status() != WL_CONNECTED)) {
        if (counter == 10) {
            Serial.print("could not connect to :");
            myDisplay.writeLine("could not connect", 3);
            myDisplay.refresh();
            delay(1000);
            Serial.println(ssid);
            return false;
        }
        counter++;
        Serial.print("trying to connect to :");
        Serial.println(ssid);
        myDisplay.writeLine("trying to connect..", 1);
        myDisplay.writeLine(ssid, 2);
        strcat(dest, src);
        myDisplay.writeLine(dest, 3);

        myDisplay.refresh();

        delay(1000);  // wait, it might still connect
    }
    Serial.print("connected to :");
    myDisplay.writeLine("connected", 4);
    myDisplay.refresh();

    Serial.println(ssid);
    delay(1000);
    return true;
}

void showText(const char *text, int x = 0, int y = 32) {
    myDisplay.clearBuffer();

    myDisplay.setFont(u8g2_font_fur11_tf);
    myDisplay.setCursor(x, y);  //! 25 is optimal for size 1 default bitmap font
    myDisplay.print(text);
    myDisplay.sendBuffer();
}

void setup_wifi() {
    WiFi.mode(WIFI_STA);
    // or
    // WiFi.mode(WIFI_MODE_APSTA);
    // WiFi.softAP(soft_ap_ssid, soft_ap_password);

    // try work wifi first
    boolean connected = false;
    showText(work_ssid);
    connected = try_wifi_connect(work_ssid, work_password);
    if (!connected) {
        connected = try_wifi_connect(home_ssid, home_password);
    }
    if (!connected) {
        myDisplay.wipe();
        myDisplay.writeLine("cant connect", 1);
        myDisplay.writeLine("Restarting...", 3);
        myDisplay.refresh();
        delay(5000);
        ESP.restart();  // restart device
    }
    myDisplay.wipe();
    myDisplay.writeLine("connected to:", 1);
    myDisplay.writeLine(WiFi.SSID().c_str(), 2);
    myDisplay.writeLine("IP:", 3);
    myDisplay.writeLine(WiFi.localIP().toString().c_str(), 4);
    myDisplay.refresh();
    delay(5000);

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
// MQTT end

// read co2 sensor and return as string
int lastGoodCO2ppm;

String readCO2Sensor() {
    int CO2ppm = myMHZ19.getCO2();  // Request CO2 (as ppm)
    // float p = bme.readPressure() / 100.0F;
    if (CO2ppm == 0) {  //! dont send back 0 - use prev or get till a number
        Serial.println("!!!!!!     Failed to read from CO2 sensor! 0 read");

        // try restarting the serial i/f to sensor
        Serial2.begin(9600);

        // myMHZ19.begin(mySerial);  // *Serial(Stream) refence must be passed to library begin().
        myMHZ19.begin(Serial2);  // *Serial(Stream) refence must be passed to library begin().
        CO2ppm = myMHZ19.getCO2();

        if (CO2ppm > 0) {
            return String(CO2ppm);
        } else {  // return previous good reading made
            return String(lastGoodCO2ppm);
        }
    } else {
        int8_t Temp;
        Temp = myMHZ19.getTemperature();  // Request Temperature (as Celsius)
        Serial.print("MHZ Temperature (C): ");
        Serial.println(Temp);

        // Serial.print("Sensor reads : ");
        // Serial.println(CO2ppm);
        lastGoodCO2ppm = CO2ppm;
        return String(CO2ppm);
    }
}
#define FORMAT_SPIFFS_IF_FAILED true

void init_display() {
    // x = 0->127
    // y = 0->63
    // 4 lines @ y=15,31,47,63
    myDisplay.begin();

    // myDisplay.clearBuffer();
    myDisplay.setFont(u8g2_font_fub17_tr);
    myDisplay.setFont(u8g2_font_fur11_tr);

    myDisplay.writeLine("Initialising", 1);
    myDisplay.writeLine(VERSION, 2);

    myDisplay.refresh();
}
void showOTAPage() {
    myDisplay.clearBuffer();

    myDisplay.setFont(u8g2_font_fur11_tf);
    myDisplay.setCursor(0, 32);  //! 25 is optimal for size 1 default bitmap font
    myDisplay.print("OTA Update");
    myDisplay.sendBuffer();
}

int postDataToRemoteDB(int CO2, float Temperature, float Humidity) {
    int resultCode = 0;
    // Check WiFi connection status
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

        Serial.print("remote server : ");
        Serial.println(serverName);

        Serial.print("httpRequestData : ");
        Serial.println(httpRequestData);

        // You can comment the httpRequestData variable above
        // then, use the httpRequestData variable below (for testing purposes without the BME280 sensor)
        // String httpRequestData = "api_key=tPmAT5Ab3j7F9&value1=24.75&value2=49.54&value3=1005.14";

        // Send HTTP POST request
        int httpResponseCode = http.POST(httpRequestData);

        // If you need an HTTP request with a content type: text/plain
        // http.addHeader("Content-Type", "text/plain");
        // int httpResponseCode = http.POST("Hello, World!");

        // If you need an HTTP request with a content type: application/json, use the following:
        // http.addHeader("Content-Type", "application/json");
        // int httpResponseCode = http.POST("{\"value1\":\"19\",\"value2\":\"67\",\"value3\":\"78\"}");

        //! what if no response from logging server???

        if (httpResponseCode > 0) {  // got a response code from server so ok - not neccsarily 200 ok, poss 500, 4304 etc
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);
        } else {  // error, -1,  probably cos of bad connection e.g dns failure etc
            Serial.print("Error code: ");
            Serial.println(httpResponseCode);
        }
        resultCode = httpResponseCode;
        // Free resources
        http.end();
    } else {
        Serial.println("WiFi Disconnected");
        resultCode = 0;
    }
    return resultCode;
}

void beep(size_t length) {
    // size_t wait = 1;
    for (size_t i = 0; i < length; i++) {
        digitalWrite(SOUNDER_PIN, HIGH);
        // for (size_t i = 0; i < wait; i++) {
        //     i++;
        //     i--;
        // }
        delay(1);
        digitalWrite(SOUNDER_PIN, LOW);
        // for (size_t i = 0; i < wait; i++) {
        //     i++;
        //     i--;
        // }
        delay(1);
    }
}
boolean audibleWarning = false;
int previousCO2 = 400;

#define xwidth = 128;
#define yhieght = 64;

void updateLEDDisplay(int co2, float Temperature, float Humidity) {
    int highLevel = 800;
    int mediumLevel = 700;
    // int lowLevel = 400;
    boolean co2Rising;

    // calc val for beat period
    //  int period =
    // in 400 to 1000, slow fast
    //  1200 - in
    unsigned long msPerCycle = (3000 - (unsigned long)co2) / 3;
    // lowLevelLED.setMsPerCycle(msPerCycle);
    // mediumLevelLED.setMsPerCycle(msPerCycle);
    // highLevelLED.setMsPerCycle(msPerCycle);
    Serial.print("msPerCycle: ");
    Serial.println(msPerCycle);

    if (co2 >= previousCO2) {
        co2Rising = true;
    } else {
        co2Rising = false;
    }
    previousCO2 = co2;

    if (co2 >= highLevel) {
        lowLevelLED.fullOff();
        mediumLevelLED.fullOff();
        highLevelLED.on();
        highLevelLED.setMsPerCycle(msPerCycle);
        if (audibleWarning) {
            tone(SOUNDER_PIN, 440, 250);
            tone(SOUNDER_PIN, 880, 250);
        }
        Serial.print("highLevel: ");
        Serial.println(co2);
    } else if (co2 >= mediumLevel) {
        lowLevelLED.fullOff();
        mediumLevelLED.on();
        highLevelLED.fullOff();
        mediumLevelLED.setMsPerCycle(msPerCycle);
        // digitalWrite(SOUNDER_PIN, HIGH);
        // delay(300);
        // digitalWrite(SOUNDER_PIN, LOW);
        if (audibleWarning) {
            tone(SOUNDER_PIN, 880, 150);
        }
        // tone(SOUNDER_PIN, 440, 500);
        Serial.print("mediumLevel: ");
        Serial.println(co2);
    } else {
        lowLevelLED.on();
        mediumLevelLED.fullOff();
        highLevelLED.fullOff();
        lowLevelLED.setMsPerCycle(msPerCycle);
        // beep(200);
        // delay(200);
        // tone(SOUNDER_PIN, 440, 10);
        Serial.print("lowLevel: ");
        Serial.println(co2);
    }

    char co2string[20];
    itoa(co2, co2string, 10);

    myDisplay.clearBuffer();
    myDisplay.setFont(BIG_TEMP_FONT);
    myDisplay.setFont(u8g2_font_fub30_tf);
    myDisplay.setFont(u8g2_font_fur30_tr);

    // myDisplay.setFont(u8g2_font_profont29_tf);
    // myDisplay.setFont(u8x8_font_profont29_2x3_r);
    int charW;
    int charH;
    myDisplay.setFont(u8g2_font_profont29_tf);
    charW = 16;

    myDisplay.setFont(u8g2_font_logisoso34_tn);
    charW = 41;

    myDisplay.setFont(u8g2_font_logisoso32_tn);
    charW = 18;
    charH = 32;

    // myDisplay.setFont(u8g2_font_logisoso34_tn);
    // charW = 19;
    // charH = 34;

    // myDisplay.setFont(u8g2_font_logisoso38_tn);
    // charW = 22;
    // charH = 38;

    // myDisplay.setFont(u8g2_font_logisoso42_tn);
    // charW = 24;
    // charH = 42;

    // myDisplay.setFont(u8g2_font_inr24_mf);

    myDisplay.drawStr(0, charH, co2string);
    int w = myDisplay.getStrWidth(co2string);

    myDisplay.setFont(u8g2_font_fur17_tr);
    myDisplay.setFont(u8g2_font_fur11_tr);
    // myDisplay.setFont(u8g2_font_profont22_tf);
    // myDisplay.drawStr(70, 30, "ppm");
    // myDisplay.drawStr((strlen(co2string) * (charW+1)) + 2, charH, "ppm");
    myDisplay.drawStr((w), charH, "ppm");

    // display.println("Initialising..");

    myDisplay.setFont(u8g2_font_open_iconic_arrow_2x_t);
    char trendString[2];
    if (co2Rising) {
        trendString[0] = '\x47';  // up on
    } else {
        trendString[0] = '\x44';  // down on
    }
    trendString[1] = '\x00';
    myDisplay.drawStr(85, 17, trendString);

    //! audio alert indicator
    myDisplay.setFont(u8g2_font_streamline_interface_essential_alert_t);
    char iconString[2];
    if (audibleWarning) {
        iconString[0] = '\x33';  // alarm on
    } else {
        iconString[0] = '\x30';  // alarm off
    }

    iconString[1] = '\x00';

    myDisplay.drawStr(107, 21, iconString);

    // display.setFont(&FreeMonoBold12pt7b);
    // display.setFont(&FreeSansBold18pt7b);

    char tempStr[7];

    dtostrf(Temperature, 3, 1, tempStr);
    tempStr[4] = '\xb0';
    tempStr[5] = 'C';
    tempStr[6] = '\x00';

    myDisplay.setFont(u8g2_font_fur11_tf);
    myDisplay.setFont(u8g2_font_profont22_tf);
    myDisplay.setFont(u8g2_font_10x20_tf);
    myDisplay.setFont(u8g2_font_fur14_tf);
    myDisplay.setFont(u8g2_font_fub11_tf);
    myDisplay.setFont(u8g2_font_profont17_tf);

    // u8g2_font_profont22_tf

    myDisplay.drawStr(0, 63, tempStr);

    char humiStr[7];

    dtostrf(Humidity, 3, 1, humiStr);
    humiStr[4] = '%';
    humiStr[5] = 'H';
    humiStr[6] = '\x00';

    // myDisplay.setFont(u8g2_font_fur11_tf);

    myDisplay.drawStr(63, 63, humiStr);

    myDisplay.sendBuffer();
}

void showInfoPage() {
    myDisplay.clearBuffer();

    myDisplay.setFont(u8g2_font_fur11_tf);
    myDisplay.setCursor(0, 31);  //! 25 is optimal for size 1 default bitmap font
    myDisplay.print(WiFi.localIP());
    myDisplay.sendBuffer();
}

int CO2 = 100;

unsigned long updateInterval = 15000;
unsigned long lastDisplayUpdate = millis() - updateInterval - 100;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

float temperature;
float humidity;

void setup() {
    tone(SOUNDER_PIN, 880, 200);
    init_display();
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, LOW);

    delay(2000);
    pinMode(SOUNDER_PIN, OUTPUT);
    digitalWrite(SOUNDER_PIN, LOW);
    heartBeatLED.begin();
    lowLevelLED.begin();
    mediumLevelLED.begin();
    highLevelLED.begin();
    dht22.setup(dhtPin, DHTesp::DHT22);
    Serial.begin(MONITOR_SPEED);  // Device to serial monitor feedback

    // Initialize SPIFFS
    if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    Serial2.begin(9600);

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

    // ONLY use mqtt if at home
    if (WiFi.SSID() == home_ssid) {
        MQTTclient.setServer(mqtt_server, 1883);
        MQTTclient.setCallback(callback);
    }
    // delay(2000);
    Serial.print("\n");

    Serial.print("\n");
    Serial.print("The URL of CO2 monitor Web Server is: ");
    Serial.print("http://");
    Serial.println(WiFi.localIP());
    Serial.print("\n");
    Serial.println("Use the above URL in your Browser to access the CO2 monitor Web Server\n");

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

    delay(1000);

    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
            type = "sketch";
        else  // U_SPIFFS
            type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
        showOTAPage();
    });
    setupOTA();

    // inverted
    int openval = 4095;
    keypad.setDebounceTime(300);
    keypad.setNoPressValue(openval - 4095);   // analog value when no key is pressed
    keypad.registerKey('e', openval - 2790);  // analog value when the key '*' is pressed
    keypad.registerKey('r', openval - 1840);  // analog value when the key '4' is pressed
    keypad.registerKey('d', openval - 1145);  // analog value when the key '3' is pressed
    keypad.registerKey('u', openval - 410);   // analog value when the key '2' is pressed
    keypad.registerKey('l', openval - 0);     // analog value when the key '1' is pressed
}

void loop() {
    heartBeatLED.update();
    lowLevelLED.update();
    mediumLevelLED.update();
    highLevelLED.update();

    if (millis() - lastDisplayUpdate >= updateInterval) {  // get data every n ms
        /* note: getCO2() default is command "CO2 Unlimited". This returns the correct CO2 reading even
        if below background CO2 levels or above range (useful to validate sensor). You can use the
        usual documented command with getCO2(false) */

        CO2 = readCO2Sensor().toInt();
        // Request CO2 (as ppm)
        temperature = dht22.getTemperature();
        humidity = dht22.getHumidity();
        // float temperature = roundf(dht22.getTemperature() * 10) / 10;
        // float humidity = roundf(dht22.getHumidity() * 10) / 10;

        Serial.println("Reading Sensors:");
        Serial.print("CO2 (ppm): ");
        Serial.println(CO2);
        Serial.print("Temperature (C): ");
        Serial.println(temperature);
        Serial.print("Humidity (C): ");
        Serial.println(humidity);

        updateLEDDisplay(CO2, temperature, humidity);

        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi is NOT connected - trying setup_wifi()");
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
        // send the data to the remote db to store
        int postStatus = postDataToRemoteDB(CO2, dht22.getTemperature(), dht22.getHumidity());
        if (postStatus < 0) {
            if (postStatus == -11) {
                // bad responbse from db server
                myDisplay.wipe();
                myDisplay.setFont(u8g2_font_fur11_tf);

                myDisplay.writeLine("No response - DB", 1);
                // myDisplay.writeLine(WiFi.SSID().c_str(), 2);
                myDisplay.writeLine("ERROR -11", 3);
                // myDisplay.writeLine(WiFi.localIP().toString().c_str(), 4);
                myDisplay.refresh();
                delay(500);
            } else {
                Serial.println("WiFi status is < 0 - trying setup_wifi()");
                setup_wifi();
            }
        }

        lastDisplayUpdate = millis();
    }
    ArduinoOTA.handle();

    // Serial.println(analogRead(KEYPAD_PIN));
    // delay(100);

    unsigned char key = keypad.getKey();
    if (key) {
        Serial.println((char)key);

        if (key == 'r') {
            // toggle audible warning beeps
            if (audibleWarning == false) {
                audibleWarning = true;
                Serial.println("Audio warnings ON");
                updateLEDDisplay(CO2, temperature, humidity);
                tone(SOUNDER_PIN, 440, 200);
                tone(SOUNDER_PIN, 660, 200);
                tone(SOUNDER_PIN, 880, 200);

            } else {
                audibleWarning = false;
                Serial.println("Audio warnings OFF");
                updateLEDDisplay(CO2, temperature, humidity);

                tone(SOUNDER_PIN, 880, 200);
                tone(SOUNDER_PIN, 660, 200);
                tone(SOUNDER_PIN, 440, 200);
            }
        }
        if (key == 'l') {
            showInfoPage();
        }
    }
}

// to upload /data folder to device SPIFFS for local webpage serving
//  pio run -t uploadfs
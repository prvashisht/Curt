#include <Credentials.h> // contains WiFi credentials, could contain any secret key
#include <CustomOTA.h> // enables WiFi, sets up OTA updates
#include <AsyncTCP.h> // Base for Async Web Server
#include <ESPAsyncWebServer.h> // Creates asynchronous web servers
#include <WebSerial.h> // enables reading "Serial" output over web (acts as an emulator)
#include <Stepper_Motor.h>

//CHANGE THESE FOR EVERY DEVICE
//
#define DEVICE_HOSTNAME "MiddleRoomCurtain"
#define STATIC_IP_HOST_ADDRESS 85
String curtain_api_secret = MIDDLE_ROOM_CURTAIN_API_SECRET;
//
//CHANGE THESE FOR EVERY DEVICE

#define PIN_SLEEP 5
#define PIN_DIR 16
#define PIN_STEP 17
#define PIN_RESET 18
#define PIN_MS3 19
#define PIN_MS2 21
#define PIN_MS1 22
#define PIN_ENABLE 23
#define PIN_LED 25
#define PIN_LIMIT_SWITCH 26
#define PIN_BUTTONS 34

#define CPU_FREQUENCY 80
#define SERIAL_BAUD_RATE 115200
#define ANALOG_RESOLUTION 12

#define STEPS_PER_ROTATION 200
#define RIGHT_BUTTON_ANALOG_THRESHOLD 1000
#define LEFT_BUTTON_ANALOG_THRESHOLD 3000

bool isCurtainLimitSwitchPressed = false;

// Enum for which input button was pressed
enum inputButtons { INPUT_NONE, INPUT_BUTTON_CLOCKWISE, INPUT_BUTTON_ANTI_CLOCKWISE };
inputButtons pressedButton = INPUT_NONE;

// Enum for curtain position
enum curtainPositions { CURTAIN_OPENED, CURTAIN_CLOSED, CURTAIN_MIDWAY };
curtainPositions curtainPosition = CURTAIN_OPENED;

// Enum for type of override from the web
// TODO: Replace this with a proper priority list.
enum webOverrideStatus { OVERRIDE_NONE, OVERRIDE_OPEN_CURTAIN, OVERRIDE_CLOSE_CURTAIN };
webOverrideStatus webControlOverride = OVERRIDE_NONE;

// Enum for current state of the motor
enum MotorActions { MOTOR_DISABLE, MOTOR_CLOCKWISE, MOTOR_ANTI_CLOCKWISE };
MotorActions motorAction = MOTOR_DISABLE;

// Enum for current entity controlling the motor
// TODO: Use CONTROL_WEBSERIAL_MONITOR for a debug mode with interactive inputs.
enum MotorControllerType { CONTROL_NONE, CONTROL_BUTTON, CONTROL_WEB_API, CONTROL_WEBSERIAL_MONITOR };
MotorControllerType motorControllerType = CONTROL_NONE;

Stepper_Motor motor(PIN_ENABLE, PIN_DIR, PIN_STEP, PIN_SLEEP, PIN_RESET, PIN_MS1, PIN_MS2, PIN_MS3, WebSerial);

void setCurtainToOpen() {
  motor.clockwise();
}
void setCurtainToClose() {
  motor.antiClockwise();
}
void setupPins() {
    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_BUTTONS, INPUT);
    pinMode(PIN_LIMIT_SWITCH, INPUT);

    digitalWrite(PIN_LED, HIGH);
    delay(2000);
    digitalWrite(PIN_LED, LOW); // check if the LED is working fine.
}
void setupWiFi() {
    const char *ssid = WIFI_SSID;
    const char *password = WIFI_PW;

    IPAddress staticIP(192, 168, 178, STATIC_IP_HOST_ADDRESS);
    IPAddress gateway(192, 168, 178, 1);
    IPAddress subnet(255, 255, 255, 0);
    IPAddress dns1(192, 168, 178, 13); // optional
    IPAddress dns2(1, 1, 1, 1);        // optional

    if (!WiFi.config(staticIP, gateway, subnet, dns1, dns2)) {
        Serial.println("STA Failed to configure");
    }

    setupOTA(DEVICE_HOSTNAME, WIFI_SSID, WIFI_PW);
}

// Set up Web Server
AsyncWebServer server(80);
// HTML page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial, sans-serif; display: inline-block; text-align: center; background-color: #f0f0f0; color: #333;}
    h2 {font-size: 3.0rem; color: #007BFF;}
    h4 {font-size: 2.0rem; color: #007BFF; margin-top: 30px;}
    p {font-size: 1.5rem;}
    body {max-width: 600px; margin: 0 auto; padding: 25px; box-shadow: 0px 0px 20px rgba(0, 0, 0, 0.1); background: #fff; border-radius: 10px;}
    button {padding: 16px 40px; font-size: 1.5rem; margin: 10px 5px; border: none; border-radius: 5px; background-color: #007BFF; color: white; cursor: pointer; transition: background-color 0.3s ease;}
    button:hover {background-color: #0056b3;}
    #buttonContainer {margin-top: 20px;}
  </style>
</head>
<body>
  <h2>ESP Web Server</h2>
  <div id="buttonContainer">
    <h4>Curtain</h4>
    <button onclick="toggleCurtain('open')">Open Curtain</button>
    <button onclick="toggleCurtain('close')">Close Curtain</button>
    <h4>Set Curtain Position</h4>
    <button onclick="setCurtainPosition('open')">Set as Open</button>
    <button onclick="setCurtainPosition('close')">Set as Closed</button>
  </div>
  <script>
    var apiSecret = '%API_SECRET%';

    function sendRequest(url) {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", url, true);
      xhr.send();
    }

    function toggleCurtain(position) {
      sendRequest(`/move_curtain?position=${position}&api_secret=${apiSecret}`);
    }

    function setCurtainPosition(position) {
      sendRequest(`/set_curtain_position?position=${position}&api_secret=${apiSecret}`);
    }
  </script>
</body>
</html>
)rawliteral";
// Replaces placeholder with button section in your web page
String processor(const String &var) {
    if (var == "API_SECRET") {
        return curtain_api_secret;
    }
    return String();
}
void createServerEndpoints() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html, processor);
    });

    server.on("/move_curtain", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!(request->hasParam("position") && request->hasParam("api_secret"))) {
            request->send(400, "text/plain", "You need 'position' and 'api_secret' params!");
            return;
        }

        String requested_position = request->getParam("position")->value();
        String api_secret = request->getParam("api_secret")->value();

        if (api_secret != curtain_api_secret) {
            request->send(401, "text/plain", "Not authorised to operate this heavy machinery!");
            return;
        }

        bool shouldOperate = false;
        if (requested_position == "open") {
            webControlOverride = OVERRIDE_OPEN_CURTAIN;
            shouldOperate = curtainPosition != CURTAIN_OPENED;
        } else if (requested_position == "close") {
            webControlOverride = OVERRIDE_CLOSE_CURTAIN;
            shouldOperate = curtainPosition != CURTAIN_CLOSED;
        }

        if (shouldOperate) request->send(200, "text/plain", "OK");
        else {
            request->send(400, "text/plain", "Curtain already at the requested position");
            webControlOverride = OVERRIDE_NONE;
        }
    });

    // New endpoint to set curtain position
    server.on("/set_curtain_position", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!(request->hasParam("position") && request->hasParam("api_secret"))) {
            request->send(400, "text/plain", "You need 'position' and 'api_secret' params!");
            return;
        }

        String requested_position = request->getParam("position")->value();
        String api_secret = request->getParam("api_secret")->value();

        if (api_secret != curtain_api_secret) {
            request->send(401, "text/plain", "Not authorised to set curtain position!");
            return;
        }

        if (requested_position == "open") {
            curtainPosition = CURTAIN_OPENED;
            request->send(200, "text/plain", "Curtain position set to open");
        } else if (requested_position == "close") {
            curtainPosition = CURTAIN_CLOSED;
            request->send(200, "text/plain", "Curtain position set to closed");
        } else {
            request->send(400, "text/plain", "Invalid position value. Use 'open' or 'close'.");
        }
    });
}
void beginOnlineServer() {
    server.begin();
}


void processWebSerialInput(uint8_t *data, size_t len) {
    WebSerial.println("Received Data...");
    String incomingMessage = "";
    for (int i = 0; i < len; i++) {
        incomingMessage += char(data[i]);
    }
    Serial.println(incomingMessage);
    if (incomingMessage == "gateway") WebSerial.println(WiFi.gatewayIP().toString());
    if (incomingMessage == "filename") WebSerial.println(__FILE__);
}
void enableWebSerial() {
    WebSerial.begin(&server);
    WebSerial.onMessage(processWebSerialInput);
}

void readButtonInputs() {
    int buttonValue = analogRead(PIN_BUTTONS);
    if (buttonValue > LEFT_BUTTON_ANALOG_THRESHOLD) {
        pressedButton = INPUT_BUTTON_ANTI_CLOCKWISE;
    } else if (buttonValue > RIGHT_BUTTON_ANALOG_THRESHOLD) {
        pressedButton = INPUT_BUTTON_CLOCKWISE;
    } else {
        pressedButton = INPUT_NONE;
    }

    isCurtainLimitSwitchPressed = digitalRead(PIN_LIMIT_SWITCH);
}
void respondToButtonInputs() {
    // move logic inside common move_curtain function
    if (isCurtainLimitSwitchPressed || !(digitalRead(PIN_ENABLE) || pressedButton != INPUT_NONE) && webControlOverride == OVERRIDE_NONE) {
        motor.disable();
    } else if (digitalRead(PIN_ENABLE)) {
        if (pressedButton == INPUT_BUTTON_CLOCKWISE) {
            motor.clockwise();
        } else if (pressedButton == INPUT_BUTTON_ANTI_CLOCKWISE) {
            motor.antiClockwise();
        }

        if (pressedButton != INPUT_NONE) {
            motor.enable();
            motor.takeSteps();
            motor.disable();
        }
    }
}
void moveCurtain(curtainPositions target_position, MotorActions target_direction, int num_rotations, int num_offset_rotations) {
    if (curtainPosition != target_position) {
        target_direction == MOTOR_CLOCKWISE ? motor.clockwise() : motor.antiClockwise();
        motor.enable();
        motor.takeSteps(STEPS_PER_ROTATION * num_rotations);
        target_direction == MOTOR_CLOCKWISE ? motor.antiClockwise() : motor.clockwise();
        motor.takeSteps(STEPS_PER_ROTATION * num_offset_rotations);
        motor.disable();
        curtainPosition = target_position;
    }
}
void processWebControls() {
    // move logic inside common move_curtain function
    byte num_rotations = 22, num_offset_rotations = 3;
    switch (webControlOverride) {
        case OVERRIDE_OPEN_CURTAIN:
            moveCurtain(CURTAIN_OPENED, MOTOR_CLOCKWISE, num_rotations, num_offset_rotations);
            break;
        case OVERRIDE_CLOSE_CURTAIN:
            moveCurtain(CURTAIN_CLOSED, MOTOR_ANTI_CLOCKWISE, num_rotations, num_offset_rotations);
            break;
    }
    webControlOverride = OVERRIDE_NONE;
}

void setLEDState() {
    if (!digitalRead(PIN_ENABLE)) {
        digitalWrite(PIN_LED, HIGH);
    } else {
        digitalWrite(PIN_LED, LOW);
    }
}

void setupWebServer() {
  createServerEndpoints();
  enableWebSerial();
  beginOnlineServer();
}

void setup() {
    setCpuFrequencyMhz(CPU_FREQUENCY);
    Serial.begin(SERIAL_BAUD_RATE);
    analogReadResolution(ANALOG_RESOLUTION); // test lower resolution for smaller button value range

    setupPins();
    setupWiFi();
    setupWebServer();

    WebSerial.print("Running ");
    WebSerial.println(__FILE__);
}

void loop() {
    ArduinoOTA.handle();
    readButtonInputs();
    respondToButtonInputs();

    processWebControls();

    setLEDState();
}

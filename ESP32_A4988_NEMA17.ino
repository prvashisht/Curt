#include <Credentials.h> // contains WiFi credentials, could contain any secret key
#include <CustomOTA.h> // enables WiFi, sets up OTA updates
#include <AsyncTCP.h> // Base for Async Web Server
#include <ESPAsyncWebServer.h> // Creates asynchronous web servers
#include <WebSerial.h> // enables reading "Serial" output over web (acts as an emulator)
#include <Stepper_Motor.h>

const byte PIN_SLEEP = 5;
const byte PIN_DIR = 16;
const byte PIN_STEP = 17;
const byte PIN_RESET = 18;
const byte PIN_MS3 = 19;
const byte PIN_MS2 = 21;
const byte PIN_MS1 = 22;
const byte PIN_ENABLE = 23;
const byte PIN_LED = 25;
const byte PIN_LIMIT_SWITCH = 26;
const byte PIN_BUTTONS = 34;

bool isLimitSwitchPressed = false;
enum inputButtons { NONE, BUTTON_CLOCKWISE, BUTTON_ANTI_CLOCKWISE };
inputButtons pressedButton = NONE;
int BUTTON_CLOCKWISE_THRESHOLD = 3000;
int BUTTON_ANTI_CLOCKWISE_THRESHOLD = 1000;

bool isCurtainClosed = false;
enum webOverrideStatus { STAY, OPEN_CURTAIN, CLOSE_CURTAIN };
webOverrideStatus webOverride = STAY;

Stepper_Motor motor(PIN_ENABLE, PIN_DIR, PIN_STEP, PIN_SLEEP, PIN_RESET, PIN_MS1, PIN_MS2, PIN_MS3);

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
    IPAddress staticIP(192, 168, 178, 85);
    IPAddress gateway(192, 168, 178, 1);
    IPAddress subnet(255, 255, 255, 0);
    IPAddress dns1(192, 168, 178, 13); // optional
    IPAddress dns2(1, 1, 1, 1);        // optional

    if (!WiFi.config(staticIP, gateway, subnet, dns1, dns2)) {
        Serial.println("STA Failed to configure");
    }

    setupOTA("LastRoomCurtain", WIFI_SSID, WIFI_PW);
}

// Set up Web Server
AsyncWebServer server(80);
// HTML page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 6px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 3px}
    input:checked+.slider {background-color: #b30000}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>ESP Web Server</h2>
  %BUTTONPLACEHOLDER%
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
//  if(element.checked){ xhr.open("GET", "/update?output="+element.id+"&state=1", true); }
//  else { xhr.open("GET", "/update?output="+element.id+"&state=0", true); }
  if(element.checked){ xhr.open("GET", "/open", true); }
  else { xhr.open("GET", "/close", true); }
  xhr.send();
}
</script>
</body>
</html>
)rawliteral";
// Replaces placeholder with button section in your web page
String processor(const String &var) {
    if (var == "BUTTONPLACEHOLDER") {
        String checkedString = isCurtainClosed ? "checked" : "";
        String buttons = "";
        buttons += "<h4>Curtain</h4><label class='switch'><input type='checkbox' onchange='toggleCheckbox(this)' id='2' " + checkedString + "><span class='slider'></span></label>";
        return buttons;
    }
    return String();
}
void createServerEndpoints() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html, processor);
    });

    server.on("/open", HTTP_GET, [](AsyncWebServerRequest *request) {
        webOverride = OPEN_CURTAIN;
        request->send(200, "text/plain", "OK");
    });

    server.on("/close", HTTP_GET, [](AsyncWebServerRequest *request) {
        webOverride = CLOSE_CURTAIN; // TODO: Not working, only opens with api calls
        request->send(200, "text/plain", "OK");
    });
}
void beginOnlineServer() {
    server.begin();
}

void enableWebSerial() {
    WebSerial.begin(&server);
    WebSerial.msgCallback(processWebSerialInput);
}
void processWebSerialInput(uint8_t *data, size_t len) {
    WebSerial.println("Received Data...");
    String d = "";
    for (int i = 0; i < len; i++) {
        d += char(data[i]);
    }
    Serial.println(d);
    if (d == "open") {
        webOverride = OPEN_CURTAIN;
    }
    if (d == "close") {
        webOverride = CLOSE_CURTAIN;
    }
    if (d == "gateway") {
        WebSerial.println(WiFi.gatewayIP().toString());
    }
}

void readButtonInputs() {
    int buttonValue = analogRead(PIN_BUTTONS);
    if (buttonValue > BUTTON_CLOCKWISE_THRESHOLD) {
        pressedButton = BUTTON_CLOCKWISE;
    } else if (buttonValue > BUTTON_ANTI_CLOCKWISE_THRESHOLD) {
        pressedButton = BUTTON_ANTI_CLOCKWISE;
    } else {
        pressedButton = NONE;
    }

    isLimitSwitchPressed = digitalRead(PIN_LIMIT_SWITCH);
}
void respondToButtonInputs() {
    // move logic inside common move_curtain function
    if (isLimitSwitchPressed || !(digitalRead(PIN_ENABLE) || pressedButton != NONE) && webOverride != STAY) {
        motor.disable();
    } else if (digitalRead(PIN_ENABLE)) {
        if (pressedButton == BUTTON_CLOCKWISE) {
            motor.clockwise();
        } else if (pressedButton == BUTTON_ANTI_CLOCKWISE) {
            motor.antiClockwise();
        }

        if (pressedButton != NONE) {
            motor.takeSteps();
        }
    }
}

void processWebControls() {
    // move logic inside common move_curtain function
    switch (webOverride) {
        case OPEN_CURTAIN:
            WebSerial.println("opening -- DO NOT UPDATE THE CODE RIGHT NOW");
            motor.takeSteps(200 * 15);
            isCurtainClosed = false;
            webOverride = STAY;
            WebSerial.print("SAFE TO UPDATE -- isCurtainClosed: ");
            WebSerial.println(isCurtainClosed);
            break;
        case CLOSE_CURTAIN:
            WebSerial.println("closing -- DO NOT UPDATE THE CODE RIGHT NOW");
            motor.takeSteps(200 * 15);
            isCurtainClosed = true;
            webOverride = STAY;
            WebSerial.print("SAFE TO UPDATE -- isCurtainClosed: ");
            WebSerial.println(isCurtainClosed);
            break;
    }
}

void setLEDState() {
    if (!digitalRead(PIN_ENABLE)) {
        digitalWrite(PIN_LED, HIGH);
    } else {
        digitalWrite(PIN_LED, LOW);
    }
}

void setup() {
    setCpuFrequencyMhz(80);
    Serial.begin(115200);

    setupPins();
    setupWiFi();
    createServerEndpoints();
    enableWebSerial();
    beginOnlineServer();

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

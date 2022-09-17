#include <Credentials.h>
#include <CustomOTA.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSerial.h>
#include <Curtains.h>

const int PIN_SLEEP = 5;
const int PIN_DIR = 16;
const int PIN_STEP = 17;
const int PIN_RESET = 18;
const int PIN_MS3 = 19;
const int PIN_MS2 = 21;
const int PIN_MS1 = 22;
const int PIN_ENABLE = 23;
const int PIN_LED = 25;
const int PIN_LIMIT_SWITCH = 26;
const int PIN_BUTTONS = 34;

const int TOTAL_STEPS = 200 * 44;
const int delay_time_us = 1000;
const int pause_time_ms = 1000;

const int BTN_ONE_THRESHOLD = 2750;
const int BTN_TWO_THRESHOLD = 3500;
const int BTN_THRESHOLD_BUFFER = 250;

const int BUTTON_ANTI_CLOCKWISE = 1;
const int BUTTON_CLOCKWISE = 2;

bool overriden = false;

const char *ssid = WIFI_SSID;
const char *password = WIFI_PW;

AsyncWebServer server(80);
Curtains curtain(PIN_ENABLE, PIN_DIR, PIN_STEP, PIN_SLEEP, PIN_RESET, PIN_MS1, PIN_MS2, PIN_MS3, TOTAL_STEPS);

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
String processor(const String &var)
{
  // Serial.println(var);
  if (var == "BUTTONPLACEHOLDER")
  {
    String buttons = "";
    buttons += "<h4>Curtain</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"2\" " + outputState(2) + "><span class=\"slider\"></span></label>";
    // buttons += "<h4>Output - GPIO 4</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"4\" " + outputState(4) + "><span class=\"slider\"></span></label>";
    // buttons += "<h4>Output - GPIO 33</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"33\" " + outputState(33) + "><span class=\"slider\"></span></label>";
    return buttons;
  }
  return String();
}

int isClosed = false;
String outputState(int output)
{
  if (isClosed)
  {
    return "checked";
  }
  else
  {
    return "";
  }
}

void recvMsg(uint8_t *data, size_t len)
{
  WebSerial.println("Received Data...");
  String d = "";
  for (int i = 0; i < len; i++)
  {
    d += char(data[i]);
  }
  Serial.println(d);
  if (d == "open")
  {
    WebSerial.println("opening");
    curtain.open(200 * 15);
    overriden = true;
    isClosed = false;
    WebSerial.print("isClosed - ");
    WebSerial.println(isClosed);
  }
  if (d == "close")
  {
    WebSerial.println("closing");
    curtain.close(200 * 15);
    overriden = true;
    isClosed = true;
    WebSerial.print("isClosed - ");
    WebSerial.println(isClosed);
  }
}
int get_selected_button(int pin_value)
{
  if (pin_value > 3000)
  {
    return 2;
  }
  else if (pin_value > 1000)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}
void spin(int steps)
{
  if (!steps)
    steps = 1;
  for (int i = 0; i < steps; i++)
  {
    ArduinoOTA.handle();
    digitalWrite(PIN_STEP, !digitalRead(PIN_STEP));
    delayMicroseconds(delay_time_us);
  }
}
void setup()
{
  pinMode(PIN_LED, OUTPUT);
  pinMode(1, OUTPUT);

  pinMode(PIN_LIMIT_SWITCH, INPUT);
  pinMode(PIN_BUTTONS, INPUT);

  setCpuFrequencyMhz(80);

  digitalWrite(PIN_LED, LOW);

  Serial.begin(115200);
  setupOTA("LastRoomCurtain", WIFI_SSID, WIFI_PW);
  Serial.println(WiFi.localIP());

  WebSerial.begin(&server);
  WebSerial.msgCallback(recvMsg);
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", index_html, processor); });

  server.on("/open", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    curtain.open(200 * 20);
    overriden = true;
    WebSerial.println("open-ing");
    isClosed = false;
    request->send(200, "text/plain", "OK"); });

  server.on("/close", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    curtain.close(200 * 20);
    overriden = true;
    WebSerial.println("close-ing");
    isClosed = true;
    request->send(200, "text/plain", "OK"); });
  server.begin();
}
void loop()
{
  ArduinoOTA.handle();

  int button_value = analogRead(PIN_BUTTONS),
      pressed_button = get_selected_button(button_value);

  if (digitalRead(PIN_ENABLE))
  {
    if (pressed_button == BUTTON_CLOCKWISE)
    {
      curtain.close();
    }
    else if (pressed_button == BUTTON_ANTI_CLOCKWISE)
    {
      curtain.open();
    }
  }

  if (digitalRead(PIN_LIMIT_SWITCH) || !(digitalRead(PIN_ENABLE) || pressed_button) && !overriden)
  {
    curtain.disable();
  }

  // TODO: Move LED status logic into library
  if (!digitalRead(PIN_ENABLE))
  {
    digitalWrite(PIN_LED, HIGH);
  }
  else
  {
    digitalWrite(PIN_LED, LOW);
  }
}

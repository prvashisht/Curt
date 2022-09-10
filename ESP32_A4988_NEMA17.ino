#include <Credentials.h>
#include <CustomOTA.h>
#include <Stepper_Motor.h>

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

const int TOTAL_STEPS = 200*44;
const int delay_time_us = 1000;
const int pause_time_ms = 1000;

const int BTN_ONE_THRESHOLD = 2750;
const int BTN_TWO_THRESHOLD = 3500;
const int BTN_THRESHOLD_BUFFER = 250;

const int BUTTON_ANTI_CLOCKWISE = 1;
const int BUTTON_CLOCKWISE = 2;

Stepper_Motor motor(PIN_ENABLE, PIN_DIR, PIN_STEP, PIN_SLEEP, PIN_RESET, PIN_MS1, PIN_MS2, PIN_MS3);

bool overriden = false;

const char* ssid     = WIFI_SSID;
const char* password = WIFI_PW;
WiFiServer server(80);

void wifi_client_check() {
  WiFiClient client = server.available();

  if (client) {
    Serial.println("New Client.");
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {             // if there's bytes to read from the client,
        ArduinoOTA.handle();
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("Click <a href=\"/CL\">here</a> to spin clockwise.<br>");
            client.print("Click <a href=\"/ACL\">here</a> to spin anti-clockwise.<br>");
            client.print("Click <a href=\"/END\">here</a> to stop spinning.<br>");

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /CL" or "GET /ACL":
        if (currentLine.endsWith("GET /CL")) {
          motor.clockwise();
          motor.enable();
          overriden= true;
        }
        if (currentLine.endsWith("GET /ACL")) {
          motor.antiClockwise();
          motor.enable();
          overriden= true;
        }
        if (currentLine.endsWith("GET /END")) {
          motor.disable();
          overriden= false;
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}

int get_selected_button(int pin_value) {
  if (pin_value > 3000) {
    return 2;
  } else if (pin_value > 1000) {
    return 1;
  } else {
    return 0;
  }
}
void spin(int steps) {
  if (!steps) steps = 1;
  for (int i = 0; i < steps; i++) {
    ArduinoOTA.handle();
    digitalWrite(PIN_STEP, !digitalRead(PIN_STEP));
    delayMicroseconds(delay_time_us);
  }
}
void open_curtains() {
  motor.antiClockwise();
  motor.enable();
  spin(TOTAL_STEPS);
  motor.disable();
}
void close_curtains() {
  motor.clockwise();
  motor.enable();
  spin(TOTAL_STEPS);
  motor.disable();
}
void setup() {
  Serial.begin(115200);

  pinMode(PIN_LED, OUTPUT);

  pinMode(PIN_LIMIT_SWITCH, INPUT);
  pinMode(PIN_BUTTONS, INPUT);

  digitalWrite(PIN_LED, LOW);

  setupOTA("LastRoomCurtain", WIFI_SSID, WIFI_PW);
  server.begin();
}
void loop() {
  ArduinoOTA.handle();
  wifi_client_check();

  int button_value = analogRead(PIN_BUTTONS),
    pressed_button = get_selected_button(button_value);

  if (digitalRead(PIN_ENABLE)) {
    if (pressed_button == BUTTON_CLOCKWISE) {
      motor.clockwise();
      motor.enable();
    } else if (pressed_button == BUTTON_ANTI_CLOCKWISE) {
      motor.antiClockwise();
      motor.enable();
    }
  }

  if (digitalRead(PIN_LIMIT_SWITCH) || !(digitalRead(PIN_ENABLE) || pressed_button) && !overriden) {
    motor.disable();
  }

  if (!digitalRead(PIN_ENABLE)) {
    digitalWrite(PIN_LED, HIGH);
    motor.takeSteps();
  } else {
    digitalWrite(PIN_LED, LOW);
  }
}

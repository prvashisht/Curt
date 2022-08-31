#include <Credentials.h>
#include <CustomOTA.h>

const int PIN_STEP = 18;
const int PIN_DIR = 19;
const int PIN_ENABLE = 23;
const int PIN_BUTTON_CL = 34;
const int PIN_BUTTON_AC = 35;
const int steps_per_rev = 200;
const int delay_time_us = 1000;
const int pause_time_ms = 1000;

const char* ssid     = WIFI_SSID;
const char* password = WIFI_PW;
WiFiServer server(80);

void connect_to_wifi() {
  delay(10);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  ArduinoOta.setHostname("LastRoomCurtain");
  setupOTA();

  server.begin();
}
bool overriden = false;
void wifi_client_check() {
  WiFiClient client = server.available();

  if (client) {
    Serial.println("New Client.");
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {             // if there's bytes to read from the client,
        ArduinoOta.handle();
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
          digitalWrite(PIN_DIR, LOW);
          digitalWrite(PIN_ENABLE, LOW);
          overriden= true;
          spin(1);
        }
        if (currentLine.endsWith("GET /ACL")) {
          digitalWrite(PIN_DIR, HIGH);
          digitalWrite(PIN_ENABLE, LOW);
          overriden= true;
          spin(1);
        }
        if (currentLine.endsWith("GET /END")) {
          digitalWrite(PIN_ENABLE, HIGH);
          overriden= false;
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}
void spin(int steps) {
  if (!steps) steps = 1;
  for (int i = 0; i < steps; i++) {
    ArduinoOta.handle();
    digitalWrite(PIN_STEP, !digitalRead(PIN_STEP));
    delayMicroseconds(delay_time_us);
  }
}
void setup() {
  Serial.begin(115200);

  pinMode(PIN_STEP, OUTPUT);
  pinMode(PIN_DIR, OUTPUT);

  pinMode(PIN_ENABLE, OUTPUT);
  digitalWrite(PIN_ENABLE, HIGH);

  pinMode(PIN_BUTTON_CL, INPUT);
  pinMode(PIN_BUTTON_AC, INPUT);

  connect_to_wifi();
  
}
void loop() {
  ArduinoOta.handle();
  Serial.println(digitalRead(PIN_ENABLE));

  wifi_client_check();

  if (digitalRead(PIN_ENABLE)) {
    if (digitalRead(PIN_BUTTON_CL)) {
      digitalWrite(PIN_DIR, LOW);
      digitalWrite(PIN_ENABLE, LOW);
    }
    if (digitalRead(PIN_BUTTON_AC)) {
      digitalWrite(PIN_DIR, HIGH);
      digitalWrite(PIN_ENABLE, LOW);
    }
  }

  if (!digitalRead(PIN_ENABLE) && !digitalRead(PIN_BUTTON_CL) && !digitalRead(PIN_BUTTON_AC) && !overriden) digitalWrite(PIN_ENABLE, HIGH);
//  if ((digitalRead(PIN_BUTTON_CL) || digitalRead(PIN_BUTTON_AC)) && !digitalRead(PIN_ENABLE)) spin(1);
  if (!digitalRead(PIN_ENABLE)) spin(1);
}

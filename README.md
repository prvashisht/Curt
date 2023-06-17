# Curtain Controller for ESP32

This project provides code for an ESP32-based system that can control a curtain via a stepper motor. The ESP32 board reads inputs from physical buttons and a web interface and moves the curtain to the requested position.

The project includes functionality for Over-The-Air (OTA) updates, allowing you to update the ESP32's firmware without a physical connection to your PC. Additionally, it includes a web-based serial monitor allowing for remote debugging.

## Prerequisites

- An ESP32 development board
- A stepper motor
- Two physical buttons for manual curtain control
- A limit switch to prevent over-driving the curtain
- Basic electronics components (resistors, capacitors, breadboard, jumper wires, etc.)
- Arduino IDE for programming the ESP32

## Libraries Used

- Credentials.h: This library contains the WiFi SSID and password.
- [CustomOTA.h](https://github.com/SensorsIot/ESP32-OTA): This library allows OTA firmware updates.
- AsyncTCP.h: This library is used to create asynchronous TCP connections.
- ESPAsyncWebServer.h: This library is used to create asynchronous web servers.
- WebSerial.h: This library allows serial communication over a web interface.
- [Stepper_Motor.h](https://github.com/prvashisht/Stepper_Motor): This library is used to control the stepper motor.

## Key Features

- Physical buttons to open or close the curtain.
- A limit switch to prevent over-driving the curtain.
- WiFi connection and a basic web server for remote curtain control.
- Over-The-Air (OTA) firmware updates.
- Web-based serial monitor for debugging.

## Installation

1. Download or clone this repository.
2. Open the script in the Arduino IDE.
3. Make sure to install the necessary libraries (mentioned above).
4. Upload the script to your ESP32.

## Usage

Once the ESP32 is up and running, you can control the curtain using the physical buttons. Press the button once to open or close the curtain.

To control the curtain remotely, connect your device to the same network as the ESP32 and navigate to its IP address in a web browser. Click the 'Open' or 'Close' buttons to control the curtain.

## Future Improvements

- Add a priority list for the different types of curtain control inputs (button, web interface, etc.)
- Implement a more sophisticated web interface with real-time feedback of the curtain position.
- Add security features to the web interface to prevent unauthorized access.

## Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

## License

[MIT](https://choosealicense.com/licenses/mit/)
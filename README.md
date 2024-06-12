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

## Key Features

- Physical buttons to open or close the curtain.
- A limit switch to prevent over-driving the curtain.
- WiFi connection and a basic web server for remote curtain control.
- Over-The-Air (OTA) firmware updates.
- Web-based serial monitor for debugging.

## Installation

1. Download or clone this repository.
2. Open the project/directory in the [PlatformIO](https://platformio.org/).
3. Add the custom libraries mentioned below.
4. Build and upload the project to your ESP32 remotely.

## Custom Libraries

- Credentials.h: This file contains the WiFi SSID and password and other API/Secret credentials.
- [CustomOTA.h](https://github.com/SensorsIot/ESP32-OTA): This library allows OTA firmware updates.
- Store both these files in a folder and add them as a global dependency via env variable

## Usage

Once the ESP32 is up and running, you can control the curtain using the physical buttons. Press the button once to open or close the curtain.

To control the curtain remotely, connect your device to the same network as the ESP32 and navigate to its IP address in a web browser. Click the 'Open' or 'Close' buttons to control the curtain. As the logic to detect current open/close state is not implemented (at least on the hardware side for now for me), I added a second set of buttons to manually mark the current position of the curtain as open or closed.

See the curtain in action [here](https://www.instagram.com/stories/highlights/17961398177593328/).

## Future Improvements

- Add a priority list for the different types of curtain control inputs (button, web interface, etc.)
- Implement a more sophisticated web interface with real-time feedback of the curtain position.
- Add security features to the web interface to prevent unauthorized access.

## Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

## License

The content of this project itself is licensed under the [Creative Commons Attribution 3.0 Unported license](https://creativecommons.org/licenses/by/3.0/), and the underlying source code used to format and display that content is licensed under the [MIT license](LICENSE.md).

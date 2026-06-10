# Smart Home Automation System (ESP32)

An intermediate-level IoT firmware prototype built for the ESP32 DevKit V1 that simulates a modular home automation hub. The architecture features a non-blocking Finite State Machine (FSM), interrupt-driven physical inputs, and rule-based environmental control.

## 🚀 Key Architectural Highlights
* **Finite State Machine:** Built using a explicit structure (`IDLE`, `AUTO`, `MANUAL`, `ALARM`) to ensure deterministic behavior and highly testable state transitions.
* **Non-Blocking Execution:** Replaced all blocking delays with `millis()` tracking intervals, guaranteeing sub-millisecond responsiveness for concurrent UART parsing and button reading.
* **Control Theory (Hysteresis):** Prevents mechanical relay chatter at boundary edges by maintaining independent upper ($28^\circ\text{C}$) and lower ($26^\circ\text{C}$) activation thresholds.
* **Asynchronous ISR Handling:** Captures rapid push-button edge changes cleanly using hardware interrupts combined with a software-enforced 200ms debounce buffer.

## 📊 Pin Mapping Reference
| Component | Peripherals / Pins | Signal Type | Logic / Description |
| :--- | :--- | :--- | :--- |
| **DHT22 Sensor** | GPIO 4 | Single-Wire Digital | Continuous temperature/humidity sampling |
| **SSD1306 OLED** | GPIO 21 (SDA), GPIO 22 (SCL) | Hardware I2C | 128x64 dashboard frame buffer mapping |
| **Relay Channel 1**| GPIO 26 | Digital Output | Fan control (Active-LOW safety state) |
| **Relay Channel 2**| GPIO 27 | Digital Output | Humidifier control (Active-LOW safety state) |
| **Push Button** | GPIO 0 | Digital Input | Page toggling via internal pull-up hardware |

## 🛠️ How to Run the Simulator
You can view and interact with the full execution space, test sensor thresholds, and trigger state overrides live:
👉 [Click here to launch the project simulation on Wokwi](https://wokwi.com)

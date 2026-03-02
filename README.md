# JK BMS Charger Controller for Power Station

 [english](README.md) | [ру](README.ru.md)

This project is designed to monitor a battery pack via **JK BMS** (JiKong) and automatically manage the charging process using a relay.

The module runs on an **Arduino Pro Micro** (ATmega32U4) and uses a hardware UART interface to communicate with the BMS.

Prevents micro-cycling by disconnecting the charger once the battery reaches 100% SOC and the charging current drops below a safe saturation threshold (e.g., 0.8A).

## Features

- **Hardware UART**: Uses the dedicated Serial1 port on the Pro Micro for stable BMS communication.
- **Smart Cut-off**: Monitors real-time current and SOC to trigger a relay for permanent charger disconnection.
- **Self-Charging Loop Protection**: Detects absurdly high current consumption during the charging phase (e.g., if the station is mistakenly plugged into its own inverter) and immediately cuts off the charger via the relay to prevent hardware damage.

---

## Hardware Setup: Arduino Pro Micro

The Arduino Pro Micro is ideal for this project due to its small form factor and dedicated Hardware Serial port (`Serial1`).

### Full Circuit Diagram

The complete schematic of the power station for which this code was designed can be found here:  
**[Power Station Circuit Diagram](https://www.circuit-diagram.org/circuits/85978c4a61154f5b9d4b5069df222145)**

### Wiring Diagram

| Component | Component Pin | Pro Micro Pin | Description |
| :--- | :--- | :--- | :--- |
| **BMS UART** | **GND** | **GND** | Common Ground |
| **BMS UART** | **TX** | **RX (Pin 0)** | Data from BMS to Pro Micro |
| **BMS UART** | **RX** | **TX (Pin 1)** | Commands from Pro Micro to BMS *(Requires voltage divider!)* |
| **BMS UART** | **B+ (VCC)** | **DO NOT CONNECT** | **DANGER: Full Battery Voltage!** |
| **Relay Module** | **Signal / IN** | **Pin 3** | Relay Control Signal |
| **Relay Module** | **GND** | **GND** | Common Ground |
| **Power In** | **+5V** | **RAW** | Main power supply for Arduino |
| **Power In** | **GND** | **GND** | Main power supply ground |

&nbsp;  

> [!WARNING]  
> **⚡ Logic Level Mismatch (5V vs 3V)**  
> 
> The Arduino Pro Micro used in this setup operates at 5V logic, whereas the JK BMS UART module operates at 3V logic. Sending a direct 5V signal from the Arduino's `TX` pin to the BMS's `RX` pin can damage or destroy the BMS UART port.  
> To prevent this, a primitive **voltage divider** is included in the circuit on the Arduino `TX` line to step down the voltage to a safe 3V level for the BMS. Please refer to the full circuit diagram above for proper implementation.

> [!CAUTION]  
> **⚠️ HIGH VOLTAGE on BMS UART Connector!**  
> 
> The BMS communication port (where UART pins are located) includes a pin connected directly to the **Battery Positive (B+)**.  
> This pin carries the **FULL battery voltage** (e.g., 12V, 24V, 48V+).  
> 
> **NEVER connect to Arduino!** Leave disconnected or **cut the wire entirely**.  
> **Instant Arduino death guaranteed!** 💥

---

## Installation & Software Setup

### 1. Arduino IDE Configuration

To upload the code to your Pro Micro:

1. Open **Tools** -> **Board**.
2. Select **Arduino Leonardo** or **Arduino Micro** (the Pro Micro is functionally equivalent to the Leonardo).
3. Select the correct **Port**:
* **macOS**: `/dev/cu.usbmodem...`
* **Ubuntu**: `/dev/ttyACM0` (Ensure you have permissions: `sudo usermod -a -G dialout $USER`).

### 2. Configuration

In the `.ino` file, the `JKBMSInterface` is initialized using `&Serial1`. This ensures that your debug logs go to the USB (`Serial`), while the BMS communication stays on the hardware pins 0 and 1 (`Serial1`).

```cpp
Serial1.begin(115200);
// This line in the .ino file targets the Pro Micro hardware serial pins (0 and 1)
JKBMSInterface bms(&Serial1);

```

---

### Operational Workflow & Power State

It is important to understand that the Arduino receives power **only** when the power station is connected to an external power source (e.g., AC mains) for charging.

Here is the complete step-by-step logic of how the system operates:

1. **Power On:** You connect the station to the grid (or turn on the dedicated charging switch). The Arduino powers up and the main sketch starts. Because the charger's power supply is routed through the **Normally Closed (NC)** contacts of the relay, the charger turns on **immediately and automatically**. The Arduino does not actively initiate the charge; it only intervenes to shut it off later.
2. **Data Acquisition:** Every 10 seconds, the MCU requests real-time data from the BMS via `Serial1`.
3. **Self-Charge Protection:** If the system is charging but an abnormally high current draw is detected (e.g., the station is mistakenly plugged into its own inverter), the relay is triggered immediately, **opening the circuit and stopping the charging process**.
4. **Shutdown Trigger:** Once the safe cut-off conditions are met (`SOC == 100%` AND current drops `<= 0.8A`), the MCU sends a signal to the relay. The relay opens the circuit, physically disconnecting the charger from the battery.
5. **Locked State (`RELAY_LOCKED`):** The relay remains open (charger disconnected), and the Arduino enters a safe `RELAY_LOCKED` state. At this point, the Arduino itself **remains powered on** (since it draws power from the input side, prior to the relay cut-off). It continues to run but will not attempt to close the relay again. The system safely freezes to prevent the charger from micro-cycling on/off as the battery voltage naturally sags over time.

**Result:** The charger will **not** turn back on automatically, even if the battery voltage later drops. To reset the system and start a new charging cycle, you must completely remove the external power input (e.g., by flipping the dedicated charging switch or unplugging the station). This powers down the Arduino, allowing a fresh cycle to start upon the next power-up.


---

## API Reference (`JKBMSInterface`)

| Method | Returns | Description |
| --- | --- | --- |
| `getSOC()` | `uint8_t` | State of Charge in % (0-100) |
| `getVoltage()` | `float` | Total battery pack voltage |
| `getCurrent()` | `float` | Amperes (Positive = Discharge, Negative = Charge) |
| `getCellVoltage(i)` | `float` | Voltage of cell at index `i` |
| `isCharging()` | `bool` | `true` if current is flowing into the battery |

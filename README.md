# JK BMS Charger Controller for Power Station

 [english](README.md) | [ру](README.ru.md)

This project is designed to monitor a battery pack via **JK BMS** (JiKong) and automatically manage the charging process using a relay.

The module runs on an **Arduino Pro Micro** (ATmega32U4) and uses a hardware UART interface to communicate with the BMS.

Prevents micro-cycling by disconnecting the charger once the battery reaches 100% SOC and the charging current drops below a safe saturation threshold (e.g., 0.8A).

## Features

- **Hardware UART**: Uses the dedicated Serial1 port on the Pro Micro for stable BMS communication.
- **Smart Cut-off**: Monitors real-time current and SOC to trigger a relay for permanent charger disconnection.

---

## Hardware Setup: Arduino Pro Micro

The Arduino Pro Micro is ideal for this project due to its small form factor and dedicated Hardware Serial port (`Serial1`).

### Wiring Diagram

| Component | Component Pin | Pro Micro Pin | Description |
| :--- | :--- | :--- | :--- |
| **BMS UART** | **GND** | **GND** | Common Ground |
| **BMS UART** | **TX** | **RX (Pin 0)** | Data from BMS to Pro Micro |
| **BMS UART** | **RX** | **TX (Pin 1)** | Commands from Pro Micro to BMS |
| **BMS UART** | **B+ (VCC)** | **DO NOT CONNECT** | **DANGER: Full Battery Voltage!** |
| **Relay Module** | **Signal / IN** | **Pin 3** | Relay Control Signal |
| **Relay Module** | **GND** | **GND** | Common Ground |
| **Power In** | **+5V** | **RAW** | Main power supply for Arduino |
| **Power In** | **GND** | **GND** | Main power supply ground |

&nbsp;  

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

## Logic Workflow

1. **Data Acquisition**: Every 10 seconds, the MCU requests data from the BMS via `Serial1`.
2. **Peak Tracking**: The system monitors the maximum charging current.
3. **Shutdown Trigger**: If `SOC == 100%` AND current `<= 0.8A` (and current is decreasing), the `RELAY_PIN` (Pin 3) is triggered.
4. **Safety Lock**: Once triggered, the system enters `IDLE` mode to prevent the charger from cycling on/off repeatedly.

---

## API Reference (`JKBMSInterface`)

| Method | Returns | Description |
| --- | --- | --- |
| `getSOC()` | `uint8_t` | State of Charge in % (0-100) |
| `getVoltage()` | `float` | Total battery pack voltage |
| `getCurrent()` | `float` | Amperes (Positive = Discharge, Negative = Charge) |
| `getCellVoltage(i)` | `float` | Voltage of cell at index `i` |
| `isCharging()` | `bool` | `true` if current is flowing into the battery |

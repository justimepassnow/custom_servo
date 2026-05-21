# Bill of Materials (BOM) - MG996R Smart Servo Prototype

## 1. Core Silicon & Adapters (Phase 1 Breadboard Setup)
| Qty | Component | Part Number / Specs | Package | Purpose |
| :--- | :--- | :--- | :--- | :--- |
| 1 | **Microcontroller** | CH32V003J4M6 | SOP-8 | Main logic, PID execution, ADC sampling. |
| 1 | **Motor Driver** | BDR612H | SOP-8 | H-bridge for driving the DC motor (3A peak). |
| 2 | **Prototyping Adapter** | SOP-8 to DIP-8 PCB Adapter | DIP-8 | Allows surface-mount chips to plug into a breadboard. |

## 2. Power Filtering & Transient Protection
| Qty | Component | Part Number / Specs | Package | Purpose |
| :--- | :--- | :--- | :--- | :--- |
| 1 | **TVS Diode** | SMAJ5.0A (Unidirectional) | SMA (DO-214AC)| Clamps inductive voltage spikes/Back-EMF at ~6.4V. |
| 1 | **Bulk Capacitor** | 100µF to 220µF (10V min) | Electrolytic | Absorbs heavy current transients during motor stall/reversal. |
| 1 | **Filter Capacitor** | 10µF | Ceramic (0805) | Pairs with the 10Ω resistor to create an RC low-pass filter for the MCU. |
| 2 | **Decoupling Cap** | 0.1µF (100nF) | Ceramic (0805) | High-frequency noise suppression. Placed directly at VCC/GND of MCU and Driver. |

## 3. Sensing & Logic Passives
| Qty | Component | Part Number / Specs | Package | Purpose |
| :--- | :--- | :--- | :--- | :--- |
| 1 | **Current Shunt** | 50mΩ (0.05Ω), 1W | 1206 | Low-side current sensing (yields 125mV drop at 2.5A). |
| 1 | **Filter Resistor** | 10Ω | 0805 | Forms the RC filter wall to isolate MCU from motor noise. |
| 1 | **Pull-up Resistor** | 4.7kΩ | 0805 | Stabilizes the half-duplex UART communication line. |

## 4. Donor Hardware (Harvested from MG996R)
| Qty | Component | Part Number / Specs | Package | Purpose |
| :--- | :--- | :--- | :--- | :--- |
| 1 | **DC Motor** | Stock MG996R Internal Motor | N/A | Main actuator (wired directly to BDR612H outputs). |
| 1 | **Potentiometer** | Stock 3-pin Rotary Pot | N/A | Absolute joint position feedback (wired to VCC, GND, and MCU ADC). |
| 1 | **Chassis/Gears** | Stock Casing & Metal Gears | N/A | Mechanical structure and torque multiplication. |

## 5. Prototyping Essentials
| Qty | Component | Part Number / Specs | Package | Purpose |
| :--- | :--- | :--- | :--- | :--- |
| 1 | **Programmer** | Raspberry Pi Pico | THT/DIP | Running PicoRVD to flash the MCU. |
| 1 | **Pull-up Resistor** | 1kΩ | THT/0805 | Required on the SWIO programming data line for CH32V003. |
| - | **Heavy Gauge Wire** | 22 AWG or thicker | N/A | Must be soldered directly to driver power pins/shunt to bypass breadboard rail resistance. |
# Project Details: Smart Servo Conversion Project

## Overview
A custom smart servo control board designed to fit inside a gutted TowerPro MG996R casing for a high-performance bipedal robot joint.

## Core Specifications
* **Target Application**: Bipedal Robot Joint
* **Donor Servo**: TowerPro MG996R (or standard metal-gear clone)
* **Max Current Rating**: 2.5A to 3.0A peak (during stall)
* **Control Loop**: Local-first, real-time PID feedback loop for dynamic torque/impedance tracking at 1kHz.

## Hardware & Architecture
* **Power Bus**: Shared single regulated 5V power bus for logic and DC motor (no onboard LDO to save space).
* **Noise Isolation**: RC low-pass filter (10Ω series resistor + 10µF ceramic capacitor) on the MCU supply line.
* **Grounding**: Star grounding topology to isolate motor return paths from analog feedback.
* **Motor Driver**: BDR612H (up to 3A peak stall handling capability).
* **Transient Protection**: SMAJ5.0A Unidirectional TVS Diode.
* **Current Sensing**: Low-side differential using a 50mΩ shunt resistor.

## Microcontroller
* **Selected MCU**: CH32V003 (RISC-V)
* **Current Sensing**: Uses internal Programmable Gain Amplifier (up to 32x) to scale a 125mV drop across the 50mΩ shunt. 
* **Flashing**: Programmed via Raspberry Pi Pico running PicoRVD firmware over a 1-wire interface.

## Firmware Architecture (Phase 1)
* **PWM & Driver**: 20kHz complementary PWM for the BDR612H with deadtime insertion to prevent shoot-through current spikes.
* **ADC & DMA**: Asynchronous background sampling of potentiometer (position) and shunt (current) without blocking the 1kHz loop.
* **PID Loop**: Math block comparing target vs actual position, heavily modulated by current readings to introduce "squishiness". Requires integral windup protection.
* **UART Comms**: Half-duplex single-wire serial communication to receive target angles and configuration without missing bytes during PID calculation.

## Immediate Action Items
1. **Phase 1 Breadboard Pinout**: Map the 8 pins of the CH32V003J4M6 for the SOP-to-DIP test rig (Motor PWM, Potentiometer ADC, Shunt ADC, 1-wire UART).
2. **Firmware Skeleton**: Draft C-code structure using the `ch32v003fun` framework to configure Advanced Timer for 20kHz complementary PWM.
3. **Breadboard Safety Verification**: Ensure the 2.5A stall current routes safely, bypassing high-resistance breadboard traces to protect the 50mΩ shunt readings.
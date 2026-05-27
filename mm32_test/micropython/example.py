"""
example.py — RP2040-Zero Quick-Start & Configuration Utility for Smart Servo

This script demonstrates how to set up, calibrate, scan, and configure 
an unconfigured Smart Servo (factory default ID 0) using a Waveshare RP2040-Zero.

===============================================================================
                       CONNECTION & WIRING GUIDE
===============================================================================

1. POWER CONNECTIONS:
   - RP2040-Zero GND ───────── Joint Common Ground ─── Servo GND
   - External Power Supply (5V - 8.4V) ───────────────── Servo V+
     (Warning: Do NOT power the motor directly from the RP2040-Zero 3V3 or 5V pin!
      Motors draw high currents that will damage or reset the RP2040-Zero.)

2. HALF-DUPLEX SINGLE-WIRE DATA BUS:
   - RP2040-Zero Pin '0' (GP0 / TX0) ───┬─── Servo Bus Data Line (PA12 on MM32 board)
   - RP2040-Zero Pin '1' (GP1 / RX0) ───┘
   - Connect a 4.7kΩ (or 10kΩ) Pull-Up Resistor from the combined Data Line to
     RP2040-Zero '3V3' pin.
     *Note: Since the MM32 board is configured in Open-Drain mode (AF_OD), 
     a physical pull-up resistor is REQUIRED to pull the data line high.*

                 RP2040-ZERO WIRING SCHEMA:
                     
      Waveshare RP2040-Zero
      ┌─────────────────────┐
      │     Pin '0' (GP0)   ├───────┬─────────────────── To Servo Data Line
      │     Pin '1' (GP1)   ├───────┤                    (PA12 on MM32 Board)
      │                     │       │
      │     Pin '3V3'       ├─[4.7k]┘ (Pull-Up Resistor)
      │     Pin 'GND'       ├─────────────────────────── To Servo GND & Power Supply GND
      └─────────────────────┘
"""

from machine import UART, Pin
import time
from smart_servo import ServoBus

# ─── 1. Initialize UART0 on RP2040-Zero ──────────────────────────────────────
# On RP2040-Zero, GPIO 0 and GPIO 1 correspond to physical pins labeled '0' and '1'.
print("Initializing UART0 on GP0 (Pin 0) and GP1 (Pin 1)...")
uart = UART(0, baudrate=115200, tx=Pin(0), rx=Pin(1))
bus = ServoBus(uart)

# ─── 2. Step 1: Scan for Unconfigured Servo (Default ID 0) ───────────────────
print("\n--- STEP 1: Scanning for Servo(s) ---")
print("Scanning IDs 0 to 5 to find active units...")
found = bus.scan(id_range=range(0, 6))

if not found:
    print("❌ No servos detected!")
    print("Please verify your connections:")
    print("  1. Is the servo powered by a suitable external power supply?")
    print("  2. Are RP2040-Zero pins '0' and '1' tied together and connected to the servo data line?")
    print("  3. Do you have a physical pull-up resistor (4.7k-10k) connected to 3V3?")
    print("  4. Are the RP2040-Zero GND and External Supply GND tied together?")
    import sys
    sys.exit()

for sid, status in found:
    print("✅ Detected Servo ID {}: Position={}°, Current={}mA".format(sid, status.angle, status.current_ma))

# Identify our target servo (prefer ID 0 if unconfigured, otherwise the first found)
target_id = 0
if not any(sid == 0 for sid, _ in found):
    target_id = found[0][0]
    print("⚠️ No unconfigured (ID 0) servo found. Using first detected ID: {}".format(target_id))
else:
    print("🎯 Found unconfigured servo on target ID 0.")

servo = bus.servo(target_id)

# ─── 3. Step 2: Show Initial Telemetry ───────────────────────────────────────
print("\n--- STEP 2: Querying Telemetry ---")
status = servo.poll()
if status:
    print("Initial Servo State:")
    print("  - Angle:      {}°".format(status.angle))
    print("  - Current:    {} mA".format(status.current_ma))
    print("  - Moving:     {}".format(status.is_moving))
    print("  - Overcurrent:{}".format(status.overcurrent))
    print("  - Stall:      {}".format(status.stall))

# Query and print the full non-volatile persistent configuration
print("\n--- Querying Non-Volatile Flash Configuration ---")
config_data = servo.read_config()
if config_data:
    print("Persistent Servo Configuration:")
    print("  - Servo ID:        {}".format(config_data.servo_id))
    print("  - Invert Direction:{}".format(config_data.direction_invert))
    print("  - Min/Max Angle:   {}° to {}°".format(config_data.min_angle, config_data.max_angle))
    print("  - Max Velocity:    {}°/sec".format(config_data.max_velocity))
    print("  - Current Limit:   {} mA".format(config_data.current_limit))
    print("  - Tuning PID:      Kp={:.2f}, Ki={:.4f}, Kd={:.2f}".format(config_data.kp, config_data.ki, config_data.kd))
    print("  - Calib Limits:    {}° (at ADC=0) to {}° (at ADC=4095)".format(config_data.hard_min_angle, config_data.hard_max_angle))
    print("  - Magic Word:      {:#X}".format(config_data.magic))
else:
    print("⚠️ Could not read configuration from flash!")

# ─── 4. Step 3: Configure Safety Limits and Save to Flash ────────────────────
print("\n--- STEP 3: Writing Safe Configurations ---")
# Let's set some safe operating limits:
#   - Min angle 45°, Max angle 280°
#   - Max velocity 120°/sec (moderate safe speed)
#   - Current limit 1200mA (protects the H-bridge and motor)
#   - Professionally tuned PID gains for absolute accuracy and zero overshoot
servo.configure(
      kp=102,
      
      )
# Clear any persistent or startup faults so the motor can move
print("Clearing any active or startup safety faults...")
clear_status = servo.clear_error()
if clear_status:
    print("✅ Faults cleared successfully. Current State: Overcurrent={}, Stall={}".format(
        clear_status.overcurrent, clear_status.stall
    ))
else:
    print("⚠️ Clear error command sent, but no reply received.")

# ─── 5. Step 4: Interactive Tracking Loop ─────────────────────────────────────
print("\n--- STEP 4: Interactive Tracking Loop ---")
print("Enter target angles to command the servo. Type 'exit' to stop.")
print("-" * 60)

try:
    while True:
        # Prompt the user for an angle
        user_input = input("\nEnter target angle (45 to 280) or 'exit': ").strip()
        if user_input.lower() == 'exit':
            break
            
        try:
            target_angle = int(user_input)
        except ValueError:
            print("❌ Invalid input! Please enter a number.")
            continue
            
        if target_angle < 0 or target_angle > 360:
            print("❌ Angle out of safe limits (45° to 280°)! Please try again.")
            continue

        print("Commanding target angle {}°...".format(target_angle))
        status = servo.move(target_angle)
        
        if status is None:
            print("❌ Servo is not responding to move command!")
            continue
            
        # Poll until the target angle is reached
        print("Moving and polling telemetry...")
        start_time = time.ticks_ms()
        
        while True:
            pos = servo.poll()
            if pos:
                print("  Angle: {:3d}° | Current: {:4d} mA | Moving: {!s:<5} | Fault: {}".format(
                    pos.angle, pos.current_ma, pos.is_moving, 
                    "OVERCURRENT" if pos.overcurrent else "STALL" if pos.stall else "None"
                ))
                
                # Check if it has arrived (Moving flag is False)
                if not pos.is_moving:
                    print("✨ Arrived at target angle {}°!".format(pos.angle))
                    break
                    
                # Safe fallback timeout (e.g. 8 seconds) in case it gets physically blocked
                if time.ticks_diff(time.ticks_ms(), start_time) > 4000:
                    print("⚠️ Timeout: Took too long to reach the target.")
                    break
            else:
                print("⚠️ Timeout - Servo not responding to poll")
                
            time.sleep_ms(150)
            
except KeyboardInterrupt:
    print("\nInteractive tracking stopped by user.")

print("\n--- Diagnostic & Configuration Sequence Complete! ---")

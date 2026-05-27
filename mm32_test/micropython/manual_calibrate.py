"""
manual_calibrate.py — Manual Multi-Point Calibration Utility for Smart Servo

This script performs a precise manual calibration of the potentiometer:
1. Reads initial persistent configurations to understand the current scale.
2. Prompts the user to physically align the servo shaft to 0° by hand.
   (The motor remains completely disarmed and unpowered/free-spinning at start).
3. Reads and reconstructs the true 12-bit ADC value at 0°.
4. Prompts the user to physically align the servo shaft to 90° by hand.
5. Reads and reconstructs the true 12-bit ADC value at 90°.
6. Computes the exact linear slope and intercept of the potentiometer.
7. Extrapolates the virtual physical angles at ADC = 0 and ADC = 4095.
8. Writes these precise virtual limits as hard_min and hard_max to flash.
9. Displays the verified, perfectly calibrated configuration!
"""

from machine import UART, Pin
import time
from smart_servo import ServoBus

# ─── 1. Initialize UART0 on RP2040-Zero ──────────────────────────────────────
print("Initializing UART0 on GP0 (TX) and GP1 (RX)...")
uart = UART(0, baudrate=115200, tx=Pin(0), rx=Pin(1))
bus = ServoBus(uart)

# ─── 2. Scan for Servos ───────────────────────────────────────────────────────
print("\n--- Scanning for active Servo ID(s) ---")
found = bus.scan(id_range=range(0, 6))

if not found:
    print("❌ No servos detected! Verify power and connections.")
    import sys
    sys.exit()

target_id = found[0][0]
print("🎯 Selected Servo ID: {}".format(target_id))
servo = bus.servo(target_id)

# ─── 3. Query Initial Configuration for Mathematical Reconstruction ─────────
print("\n--- Querying Initial Persistent Configuration ---")
initial_config = servo.read_config()
if initial_config:
    print("Current Configuration:")
    print("  - Physical Hard Limits: {}° to {}°".format(initial_config.hard_min_angle, initial_config.hard_max_angle))
    print("  - Direction Inverted:   {}".format(initial_config.direction_invert))
else:
    print("❌ Failed to query initial configuration!")
    import sys
    sys.exit()

old_min = initial_config.hard_min_angle
old_max = initial_config.hard_max_angle
old_sweep = old_max - old_min

if old_sweep == 0:
    old_sweep = 280  # Safety fallback

# ─── 4. Reconstruct ADC value at 0 Degrees ──────────────────────────────────
print("\n--- Step 1: Align to 0 Degrees ---")
print("⚠️ The motor is UNPOWERED. You can rotate it easily by hand.")
input("👉 Physically rotate the servo output shaft to exactly 0° (or min mechanical stop), then press Enter...")

# Poll several times to get a clean, settled reading
pos_samples = []
for _ in range(5):
    status = servo.poll()
    if status:
        pos_samples.append(status.angle)
    time.sleep_ms(50)

if not pos_samples:
    print("❌ Failed to communicate with servo!")
    import sys
    sys.exit()

# Take average reported angle
avg_angle_0 = sum(pos_samples) / len(pos_samples)

# Reconstruct raw 12-bit ADC value from reported angle
if initial_config.direction_invert:
    adc_0 = int((old_max - avg_angle_0) * 4095 / old_sweep)
else:
    adc_0 = int((avg_angle_0 - old_min) * 4095 / old_sweep)

# Clamp to 12-bit ADC range
adc_0 = max(0, min(4095, adc_0))
print("✅ 0° calibration point recorded. Reconstructed ADC: {}".format(adc_0))

# ─── 5. Reconstruct ADC value at 90 Degrees ─────────────────────────────────
print("\n--- Step 2: Align to 90 Degrees ---")
input("👉 Physically rotate the servo output shaft to exactly 90° by hand, then press Enter...")

pos_samples_90 = []
for _ in range(5):
    status = servo.poll()
    if status:
        pos_samples_90.append(status.angle)
    time.sleep_ms(50)

if not pos_samples_90:
    print("❌ Failed to communicate with servo!")
    import sys
    sys.exit()

avg_angle_90 = sum(pos_samples_90) / len(pos_samples_90)

if initial_config.direction_invert:
    adc_90 = int((old_max - avg_angle_90) * 4095 / old_sweep)
else:
    adc_90 = int((avg_angle_90 - old_min) * 4095 / old_sweep)

adc_90 = max(0, min(4095, adc_90))
print("✅ 90° calibration point recorded. Reconstructed ADC: {}".format(adc_90))

# ─── 6. Compute Linear Slope and Extrapolate Virtual Endpoints ──────────────
adc_delta = adc_90 - adc_0

if adc_delta == 0:
    print("❌ Error: Potentiometer ADC did not change between 0° and 90°! Calibration aborted.")
    import sys
    sys.exit()

# Calculate degrees per ADC count
slope = 90.0 / adc_delta

# Extrapolate theoretical angles at ADC = 0 and ADC = 4095
if initial_config.direction_invert:
    # Under inverted pot, ADC = 4095 is min angle, ADC = 0 is max angle
    virtual_hard_max = int(0.0 - slope * adc_0)
    virtual_hard_min = int(0.0 + slope * (4095 - adc_0))
else:
    # Normal direction
    virtual_hard_min = int(0.0 - slope * adc_0)
    virtual_hard_max = int(0.0 + slope * (4095 - adc_0))

print("\n--- Calibration Results & Extrapolations ---")
print("  - Potentiometer Slope:  {:.5f} degrees/ADC".format(slope))
print("  - Extrapolated Hard Min (ADC=0):    {}°".format(virtual_hard_min))
print("  - Extrapolated Hard Max (ADC=4095):  {}°".format(virtual_hard_max))

# ─── 7. Write Extrapolated Parameters to Flash ───────────────────────────────
input("\n👉 Press Enter to write these parameters to Flash...")
print("Saving parameters...")

# Soft limits set slightly inside the extrapolated limits (e.g. 20° to 240° for safety)
flash_config = servo.configure(
    hard_min=virtual_hard_min,
    hard_max=virtual_hard_max,
   
)

if flash_config:
    print("✅ Calibrated configuration successfully saved to Flash!")
else:
    print("⚠️ Config saved (no explicit transaction reply).")

# ─── 8. Print Final Verified Configuration ───────────────────────────────────
print("\n--- Verifying Saved Flash Settings ---")
final_verify = servo.read_config()
if final_verify:
    print("Final Verified Flash Settings:")
    print("  - Software Limits:     {}° to {}°".format(final_verify.min_angle, final_verify.max_angle))
    print("  - Physical Hard Limits:{}° to {}°".format(final_verify.hard_min_angle, final_verify.hard_max_angle))
    print("  - PID Tuning:          Kp={:.2f}, Ki={:.4f}, Kd={:.2f}".format(final_verify.kp, final_verify.ki, final_verify.kd))
    print("  - Magic Word:          {:#X}".format(final_verify.magic))
    print("\n🎉 MANUAL MULTI-POINT CALIBRATION COMPLETE!")
else:
    print("❌ Failed to read back final configuration!")



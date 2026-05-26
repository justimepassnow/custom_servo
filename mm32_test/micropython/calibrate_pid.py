"""
calibrate_pid.py — Automated PID Calibration Utility for Smart Servo

This script initiates the automated Ziegler-Nichols relay autotuning sequence on the servo:
1. Scans the UART bus for connected servos.
2. Displays the current active PID gains.
3. Prompts the user to confirm calibration and provides a safety warning.
4. Triggers the automated tuning sequence on the MM32 firmware (Instruction 0x06).
5. Monitors the real-time oscillation progress via polling.
6. Reads back and displays the verified, newly calculated gains from Flash memory!
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

# ─── 3. Query Current PID Gains ──────────────────────────────────────────────
print("\n--- Querying Current Persistent Gains ---")
config = servo.read_config()
if config:
    print("Current PID Settings in Flash:")
    print("  - Kp: {:.2f}".format(config.kp))
    print("  - Ki: {:.4f}".format(config.ki))
    print("  - Kd: {:.2f}".format(config.kd))
else:
    print("❌ Failed to query current configuration!")
    import sys
    sys.exit()

# ─── 4. Safety Warning & Confirmation ────────────────────────────────────────
print("\n" + "=" * 55)
print("⚠️  SAFETY AND CALIBRATION WARNING ⚠️")
print("1. Ensure the servo output shaft can rotate freely.")
print("2. Make sure it is at least 15 degrees clear of mechanical limits.")
print("3. During calibration, the motor will oscillate rapidly back-and-forth.")
print("   This sequence will last for approximately 5 to 10 seconds.")
print("=" * 55)

confirm = input("\n👉 Press Enter to start the automated PID calibration (or Ctrl+C to abort)...")

# ─── 5. Trigger Autotuning Sequence ──────────────────────────────────────────
print("\nStarting PID calibration on Servo ID {}...".format(target_id))
# Configure Tuning:
# - duty_percent=20 (20% PWM drive amplitude)
# - hysteresis_tenths=5 (0.5 degree deadband for switching)
# - cycles=5 (Observe 5 stable cycles)
# - midpoint=0 (Calibrate around current physical position)
status = servo.start_tuning(duty_percent=20, hysteresis_tenths=5, cycles=5, midpoint=0)

if status is None:
    print("❌ Failed to trigger tuning! No reply from servo.")
    import sys
    sys.exit()

print("🚀 Autotuning command accepted! Calibration in progress...")
print("Polling motor status during oscillation (polling at 10Hz)...")
print("-" * 55)

# Wait 500ms to let the first oscillation start and flag is_moving
time.sleep_ms(500)

start_time = time.ticks_ms()
timeout_ms = 15000  # 15 seconds safety timeout

# ─── 6. Poll Status & Monitor Oscillations ────────────────────────────────────
while True:
    elapsed = time.ticks_diff(time.ticks_ms(), start_time)
    if elapsed > timeout_ms:
        print("\n❌ Timeout waiting for autotuning to complete!")
        break

    status = servo.poll()
    if status is None:
        print("⚠️ Warning: Missed a status packet...")
        time.sleep_ms(100)
        continue

    # Print real-time state feedback
    print("  Position: {:3d}° | Current Draw: {:4d} mA | Active: {}".format(
        status.angle, status.current_ma, "OSCILLATING" if status.is_moving else "FINISHED"
    ))

    # If is_moving flag clears, the calibration has finished and gains are calculated
    if not status.is_moving:
        print("\n✅ Oscillation cycles complete!")
        break

    time.sleep_ms(100)

# ─── 7. Print Final Verified Configurations ──────────────────────────────────
print("\n--- Reading Back Newly Calculated PID Gains ---")
time.sleep_ms(200)  # brief pause for flash settling
final_verify = servo.read_config()

if final_verify:
    print("New Verified Gains Saved to Flash Memory:")
    print("  - Kp: {:.2f}  (Proportional)".format(final_verify.kp))
    print("  - Ki: {:.4f}  (Integral)".format(final_verify.ki))
    print("  - Kd: {:.2f}  (Derivative)".format(final_verify.kd))
    print("\n🎉 AUTOMATED PID CALIBRATION AND FLASH SAVING COMPLETE!")
else:
    print("❌ Failed to read back newly calibrated configuration!")

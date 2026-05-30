from machine import UART, Pin
import time
from smart_servo import ServoBus

def stream_live_current():
    # 1. Initialize UART0 on RP2040-Zero (Just like the calibration script)
    print("Initializing UART0 on GP0 (TX) and GP1 (RX)...")
    uart = UART(0, baudrate=250000, tx=Pin(0), rx=Pin(1))
    bus = ServoBus(uart)

    # 2. Scan for Servos
    print("\n--- Scanning for active Servo ID(s) ---")
    found = bus.scan(id_range=range(0, 6))

    if not found:
        print("❌ No servos detected! Verify power and connections.")
        return

    target_id = found[0][0]
    print(f"🎯 Selected Servo ID: {target_id}")
    servo = bus.servo(target_id)
    
    print("\n--- Live Current Streaming ---")
    print("Grab the motor to stall it and watch the current rise!")
    print("Compare this value to your Multimeter in series with the power supply.\n")
    
    try:
        while True:
            # Poll the servo state (returns a namedtuple: status.current_ma)
            status = servo.poll()
            
            if status:
                print("\rFirmware Current: {:4d} mA   ".format(status.current_ma), end='')
            else:
                print("\rFailed to read servo.         ", end='')
                
            time.sleep(0.05) # Read 20 times a second
            
    except KeyboardInterrupt:
        print("\n\nStopped.")

if __name__ == "__main__":
    stream_live_current()

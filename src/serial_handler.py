#!/usr/bin/env python3
import serial
import time
import mysql.connector
from datetime import datetime

# Config
SERIAL_PORT = '/dev/ttyACM0'
BAUD_RATE = 9600
DB_CONFIG = {
    'host': 'localhost',
    'user': 'admin',
    'password': 'admin',
    'database': 'iot_individual_project'
}

TEMP_THRESHOLD = 27.0
last_temp_alert = 0

def main():
    print("Starting serial communication with Arduino...")
    
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print(f"Connected to {SERIAL_PORT}")
        time.sleep(2)
        
        conn = mysql.connector.connect(**DB_CONFIG)
        cursor = conn.cursor()
        
        while True:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8').strip()
                print(f"Received: {line}")
                
                timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                
                if "[STATUS]" in line:
                    temp_str = line.split('Temp:')[-1].split('°C')[0].strip()
                    try:
                        temperature = float(temp_str)
                        cursor.execute(
                            "INSERT INTO sensor_data (timestamp, message_type, message, temperature) VALUES (%s, %s, %s, %s)",
                            (timestamp, "STATUS", line, temperature)
                        )
                        conn.commit()
                        
                        check_temperature(temperature, ser, cursor, timestamp)
                        
                    except ValueError:
                        print(f"Could not parse temperature from: {line}")
                else:
                    msg_type = "EVENT" if "[EVENT]" in line else "ALARM" if "[ALARM]" in line else "SYSTEM"
                    cursor.execute(
                        "INSERT INTO sensor_data (timestamp, message_type, message) VALUES (%s, %s, %s)",
                        (timestamp, msg_type, line)
                    )
                    conn.commit()
                    
            time.sleep(0.1)
            
    except Exception as e:
        print(f"Error: {e}")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
        if 'conn' in locals() and conn.is_connected():
            cursor.close()
            conn.close()

def check_temperature(temperature, serial_conn, cursor, timestamp):
    """Simple edge analytics - check if temperature exceeds threshold"""
    global last_temp_alert
    current_time = time.time()
    
    if temperature > TEMP_THRESHOLD and (current_time - last_temp_alert > 30):
        print(f"ALERT: Temperature ({temperature}°C) exceeded threshold ({TEMP_THRESHOLD}°C)")
        
        cursor.execute(
            "INSERT INTO sensor_data (timestamp, message_type, message) VALUES (%s, %s, %s)",
            (timestamp, "ALARM", f"Temperature ({temperature}°C) exceeds threshold ({TEMP_THRESHOLD}°C)")
        )
        
        serial_conn.write(b"RESET_SYSTEM\n")
        print("Sent RESET_SYSTEM command to Arduino")
        
        last_temp_alert = current_time

if __name__ == "__main__":
    main()
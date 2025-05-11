#!/usr/bin/env python3
from flask import Flask, jsonify, request
import mysql.connector
from datetime import datetime
import serial

# Config
SERIAL_PORT = '/dev/ttyACM0'  # Change to 'COM3' or similar on Windows
BAUD_RATE = 9600
DB_CONFIG = {
    'host': 'localhost',
    'user': 'admin',
    'password': 'admin',
    'database': 'iot_individual_project'
}

app = Flask(__name__)

# HTML content directly in the Python file
HTML_CONTENT = '''
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>IoT Security System Dashboard</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 1000px;
            margin: 0 auto;
            padding: 20px;
            background-color: #f5f5f5;
        }
        .header {
            text-align: center;
            margin-bottom: 30px;
        }
        .header h1 {
            color: #2c3e50;
        }
        .dashboard-container {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 20px;
        }
        @media (max-width: 768px) {
            .dashboard-container {
                grid-template-columns: 1fr;
            }
        }
        .panel {
            background-color: white;
            border-radius: 8px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            padding: 20px;
            margin-bottom: 20px;
        }
        .status-panel {
            grid-column: 1;
        }
        .controls-panel {
            grid-column: 2;
        }
        .history-panel {
            grid-column: 1 / span 2;
        }
        .temperature {
            font-size: 48px;
            font-weight: bold;
            text-align: center;
            margin: 20px 0;
        }
        .status-indicator {
            display: inline-block;
            padding: 8px 16px;
            border-radius: 20px;
            font-weight: bold;
            margin: 5px 0;
        }
        .armed {
            background-color: #2ecc71;
            color: white;
        }
        .disarmed {
            background-color: #95a5a6;
            color: white;
        }
        .alarm {
            background-color: #e74c3c;
            color: white;
            animation: blink 1s infinite;
        }
        .normal {
            background-color: #3498db;
            color: white;
        }
        @keyframes blink {
            50% { opacity: 0.5; }
        }
        table {
            width: 100%;
            border-collapse: collapse;
            margin-top: 20px;
        }
        th, td {
            padding: 12px 15px;
            text-align: left;
            border-bottom: 1px solid #ddd;
        }
        th {
            background-color: #f2f2f2;
            font-weight: bold;
        }
        tr:hover {
            background-color: #f5f5f5;
        }
        .button {
            display: block;
            width: 100%;
            padding: 12px;
            border: none;
            border-radius: 4px;
            background-color: #3498db;
            color: white;
            font-size: 16px;
            font-weight: bold;
            cursor: pointer;
            margin: 10px 0;
            transition: background-color 0.3s;
        }
        .button:hover {
            background-color: #2980b9;
        }
        .button.danger {
            background-color: #e74c3c;
        }
        .button.danger:hover {
            background-color: #c0392b;
        }
        .threshold-control {
            display: flex;
            align-items: center;
            margin: 20px 0;
        }
        .threshold-control input {
            flex-grow: 1;
            padding: 10px;
            border: 1px solid #ddd;
            border-radius: 4px;
            margin-right: 10px;
        }
        .refresh-info {
            font-size: 12px;
            color: #7f8c8d;
            text-align: center;
            margin-top: 20px;
        }
    </style>
</head>
<body>
    <div class="header">
        <h1>IoT Individual Project</h1>
        <p>Monitoring Employee Safety in Industrial Freezers</p>
    </div>
    
    <div class="dashboard-container">
        <div class="panel status-panel">
            <h2>System Status</h2>
            <div class="temperature">
                <span id="temperature">--</span>°C
            </div>
            <p>System State: <span id="system-status" class="status-indicator disarmed">DISARMED</span></p>
            <p>Alarm Status: <span id="alarm-status" class="status-indicator normal">OK</span></p>
            <p>Last Status Message:</p>
            <p id="status-text" style="font-family: monospace; background: #f0f0f0; padding: 10px; border-radius: 4px;">Loading...</p>
            <p>Last updated: <span id="timestamp">--</span></p>
        </div>
        
        <div class="panel controls-panel">
            <h2>System Controls</h2>
            <button id="reset-btn" class="button danger">Reset System</button>
            
            <h3>Temperature Threshold</h3>
            <p>Current threshold: <span id="current-threshold">27.0</span>°C</p>
            <p>When temperature exceeds this value, the system will automatically reset.</p>
            
            <div class="threshold-control">
                <input type="number" id="threshold-input" min="0" max="50" step="0.5" placeholder="New threshold">
                <button id="threshold-btn" class="button">Update</button>
            </div>
        </div>
        
        <div class="panel history-panel">
            <h2>Event History</h2>
            <table id="history-table">
                <thead>
                    <tr>
                        <th>Time</th>
                        <th>Type</th>
                        <th>Message</th>
                        <th>Temperature</th>
                    </tr>
                </thead>
                <tbody>
                    <!-- data should show here -->
                    <tr>
                        <td colspan="4" style="text-align: center;">Loading event history...</td>
                    </tr>
                </tbody>
            </table>
        </div>
    </div>
    
    <p class="refresh-info">Status updates automatically every 2 seconds. Event history updates every 10 seconds.</p>

    <script>
        function updateStatus() {
            fetch('/api/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('status-text').textContent = data.latest_status;
                    document.getElementById('temperature').textContent = data.temperature.toFixed(1);
                    
                    const systemStatus = document.getElementById('system-status');
                    systemStatus.textContent = data.system_armed ? 'ARMED' : 'DISARMED';
                    systemStatus.className = 'status-indicator ' + (data.system_armed ? 'armed' : 'disarmed');
                    
                    const alarmStatus = document.getElementById('alarm-status');
                    alarmStatus.textContent = data.alarm_active ? 'TRIGGERED' : 'OK';
                    alarmStatus.className = 'status-indicator ' + (data.alarm_active ? 'alarm' : 'normal');
                    
                    document.getElementById('timestamp').textContent = data.timestamp;
                })
                .catch(error => console.error('Error fetching status:', error));
        }
        
        function loadHistory() {
            fetch('/api/history')
                .then(response => response.json())
                .then(data => {
                    const tbody = document.querySelector('#history-table tbody');
                    tbody.innerHTML = '';
                    
                    if (data.length === 0) {
                        const row = document.createElement('tr');
                        const cell = document.createElement('td');
                        cell.colSpan = 4;
                        cell.style.textAlign = 'center';
                        cell.textContent = 'No events recorded yet';
                        row.appendChild(cell);
                        tbody.appendChild(row);
                        return;
                    }
                    
                    data.forEach(item => {
                        const row = document.createElement('tr');
                        
                        const timeCell = document.createElement('td');
                        timeCell.textContent = item.timestamp;
                        
                        const typeCell = document.createElement('td');
                        typeCell.textContent = item.message_type;
                        
                        const messageCell = document.createElement('td');
                        messageCell.textContent = item.message;
                        
                        const tempCell = document.createElement('td');
                        tempCell.textContent = item.temperature ? item.temperature.toFixed(1) + '°C' : '-';
                        
                        row.appendChild(timeCell);
                        row.appendChild(typeCell);
                        row.appendChild(messageCell);
                        row.appendChild(tempCell);
                        
                        tbody.appendChild(row);
                    });
                })
                .catch(error => console.error('Error fetching history:', error));
        }
        
        function loadThreshold() {
            fetch('/api/threshold')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('current-threshold').textContent = data.threshold.toFixed(1);
                    document.getElementById('threshold-input').placeholder = data.threshold.toFixed(1);
                })
                .catch(error => console.error('Error fetching threshold:', error));
        }
        
        document.getElementById('reset-btn').addEventListener('click', function() {
            if (confirm('Are you sure you want to reset the system?')) {
                fetch('/api/reset', {
                    method: 'POST',
                })
                .then(response => response.json())
                .then(data => {
                    alert(data.message);
                    updateStatus();
                    loadHistory();
                })
                .catch(error => console.error('Error resetting system:', error));
            }
        });
        
        document.getElementById('threshold-btn').addEventListener('click', function() {
            const input = document.getElementById('threshold-input');
            const value = parseFloat(input.value);
            
            if (isNaN(value)) {
                alert('Please enter a valid number');
                return;
            }
            
            fetch('/api/threshold', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ threshold: value }),
            })
            .then(response => response.json())
            .then(data => {
                alert(data.message);
                loadThreshold();
                input.value = '';
            })
            .catch(error => console.error('Error updating threshold:', error));
        });
        
        updateStatus();
        loadHistory();
        loadThreshold();
        
        setInterval(updateStatus, 2000);
        setInterval(loadHistory, 10000);
    </script>
</body>
</html>
'''

@app.route('/')
def index():
    return HTML_CONTENT

@app.route('/api/status')
def get_status():
    try:
        conn = mysql.connector.connect(**DB_CONFIG)
        cursor = conn.cursor(dictionary=True)
        
        cursor.execute(
            "SELECT * FROM sensor_data WHERE message_type='STATUS' ORDER BY timestamp DESC LIMIT 1"
        )
        latest_status = cursor.fetchone()
        
        system_armed = False
        alarm_active = False
        temperature = 0.0
        
        if latest_status:
            message = latest_status['message']
            system_armed = "System:ARMED" in message
            alarm_active = "Alarm:TRIGGERED" in message
            
            if 'temperature' in latest_status and latest_status['temperature']:
                temperature = latest_status['temperature']
        
        cursor.close()
        conn.close()
        
        return jsonify({
            'latest_status': latest_status['message'] if latest_status else "No data",
            'temperature': temperature,
            'system_armed': system_armed,
            'alarm_active': alarm_active,
            'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        })
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/history')
def get_history():
    try:
        conn = mysql.connector.connect(**DB_CONFIG)
        cursor = conn.cursor(dictionary=True)
        
        limit = request.args.get('limit', default=50, type=int)
        
        cursor.execute(
            "SELECT * FROM sensor_data ORDER BY timestamp DESC LIMIT %s",
            (limit,)
        )
        
        history = cursor.fetchall()
        cursor.close()
        conn.close()
        
        for item in history:
            if isinstance(item['timestamp'], datetime):
                item['timestamp'] = item['timestamp'].strftime('%Y-%m-%d %H:%M:%S')
        
        return jsonify(history)
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/reset', methods=['POST'])
def reset_system():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        ser.write(b"RESET_SYSTEM\n")
        ser.close()
        
        conn = mysql.connector.connect(**DB_CONFIG)
        cursor = conn.cursor()
        
        cursor.execute(
            "INSERT INTO sensor_data (timestamp, message_type, message) VALUES (%s, %s, %s)",
            (datetime.now().strftime('%Y-%m-%d %H:%M:%S'), "EVENT", "System reset triggered from web interface")
        )
        
        conn.commit()
        cursor.close()
        conn.close()
        
        return jsonify({'status': 'success', 'message': 'Reset command sent'})
    except Exception as e:
        return jsonify({'status': 'error', 'message': str(e)}), 500

@app.route('/api/threshold', methods=['GET', 'POST'])
def temperature_threshold():
    if request.method == 'POST':
        try:
            data = request.json
            new_threshold = data.get('threshold')
            
            if new_threshold is None:
                return jsonify({'status': 'error', 'message': 'No threshold provided'}), 400
                
            conn = mysql.connector.connect(**DB_CONFIG)
            cursor = conn.cursor()
            
            cursor.execute(
                "INSERT INTO settings (setting_name, setting_value, updated_at) VALUES (%s, %s, %s) " +
                "ON DUPLICATE KEY UPDATE setting_value = %s, updated_at = %s",
                ('temp_threshold', str(new_threshold), datetime.now(), str(new_threshold), datetime.now())
            )
            
            conn.commit()
            cursor.close()
            conn.close()
            
            return jsonify({'status': 'success', 'message': f'Temperature threshold updated to {new_threshold}°C'})
        except Exception as e:
            return jsonify({'status': 'error', 'message': str(e)}), 500
    else:
        try:
            conn = mysql.connector.connect(**DB_CONFIG)
            cursor = conn.cursor(dictionary=True)
            
            cursor.execute(
                "SELECT setting_value FROM settings WHERE setting_name = 'temp_threshold'"
            )
            
            result = cursor.fetchone()
            threshold = 27.0  # Default
            
            if result:
                threshold = float(result['setting_value'])
                
            cursor.close()
            conn.close()
            
            return jsonify({'threshold': threshold})
        except Exception as e:
            return jsonify({'status': 'error', 'message': str(e)}), 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
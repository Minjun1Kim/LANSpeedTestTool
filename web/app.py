# app.py
from flask import Flask, request, jsonify, render_template
import subprocess
import json
import re

app = Flask(__name__)

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/start_test', methods=['POST'])
def start_test():
    try:
        # Extract parameters from the request
        test_type = request.json.get('test_type')
        server_address = request.json.get('server_address')
        port = request.json.get('port', 8000)
        size = request.json.get('size', 1024)
        duration = request.json.get('duration', 10)

        # Run the lan_speed client
        command = ['../lan_speed', '-m', 'client', '-t', test_type,
                   '-a', server_address, '-p', str(port),
                   '-s', str(size), '-d', str(duration)]
        result = subprocess.run(command, capture_output=True, text=True)
        output = result.stdout.strip()  # Full raw output

        print(f"Raw Output from lan_speed: {output}")  # Log the raw output

        # Parse JSON output directly
        data = json.loads(output)  # Parse the JSON from the C output

        # Return success response with parsed data
        return jsonify({'status': 'success', 'rtt_values': data['rtt_values'], 'jitter': data['jitter']})

    except json.JSONDecodeError as e:
        print(f"JSON Decode Error: {e}")
        return jsonify({'status': 'error', 'message': 'Invalid JSON received from server'}), 500
    except Exception as e:
        print(f"Error: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 500


if __name__ == '__main__':
    app.run(debug=True)

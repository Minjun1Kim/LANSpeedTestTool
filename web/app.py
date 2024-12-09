# app.py
from flask import Flask, request, jsonify, render_template, send_from_directory
import subprocess
import json
import re

app = Flask(__name__)

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/favicon.ico')
def favicon():
    return send_from_directory('static', 'favicon.ico')

@app.route('/start_test', methods=['POST'])
def start_test():
    try:
        test_type = request.json.get('test_type')
        server_address = request.json.get('server_address')
        port = request.json.get('port', 8000)
        size = request.json.get('size', 1024)
        duration = request.json.get('duration', 10)
        num_packets = request.json.get('num_packets', 10)

        # Build command differently based on test type (if needed)
        # Or just keep as is and rely on server to ignore irrelevant params
        command = [
            '../lan_speed', '-m', 'client', '-t', test_type,
            '-a', server_address, '-p', str(port),
            '-s', str(size), '-d', str(duration),
            '-n', str(num_packets)
        ]

        result = subprocess.run(command, capture_output=True, text=True)
        output = result.stdout.strip()
        print(f"Raw Output from lan_speed: {output}")

        data = json.loads(output)

        # Return fields based on test_type
        if test_type == 'jitter':
            # Jitter test outputs rtt_values and jitter
            return jsonify({
                'status': 'success',
                'rtt_values': data.get('rtt_values', []),
                'jitter': data.get('jitter', 0)
            })
        elif test_type == 'upload':
            # Upload test outputs bytes_sent, time_microseconds, mbps
            return jsonify({
                'status': 'success',
                'bytes_sent': data.get('bytes_sent', 0),
                'time_microseconds': data.get('time_microseconds', 0),
                'mbps': data.get('mbps', 0.0)
            })
        elif test_type == 'download':
            # If you adjust download to output JSON, handle similarly
            # For now, just return what is provided:
            return jsonify({
                'status': 'success',
                'message': 'Download complete',
                # Add relevant fields if you print them from download test
            })
        elif test_type == 'ping':
            # Similarly handle ping if you produce JSON output for it
            return jsonify({
                'status': 'success',
                'message': 'Ping results',
                # Add relevant fields from your ping test output
            })
        else:
            return jsonify({
                'status': 'error',
                'message': f'Unknown test type: {test_type}'
            }), 400

    except json.JSONDecodeError as e:
        print(f"JSON Decode Error: {e}")
        return jsonify({'status': 'error', 'message': 'Invalid JSON received from server'}), 500
    except Exception as e:
        print(f"Error: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 500


if __name__ == '__main__':
    app.run(debug=True)

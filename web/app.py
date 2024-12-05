# app.py
from flask import Flask, request, jsonify, render_template
import subprocess

app = Flask(__name__)

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/start_test', methods=['POST'])
def start_test():
    test_type = request.json.get('test_type')
    server_address = request.json.get('server_address')
    port = request.json.get('port', 8000)
    size = request.json.get('size', 1024)
    duration = request.json.get('duration', 10)

    try:
        # Run the C program as a subprocess
        command = ['../lan_speed', '-m', 'client', '-t', test_type,
                   '-a', server_address, '-p', str(port),
                   '-s', str(size), '-d', str(duration)]
        result = subprocess.run(command, capture_output=True, text=True)
        output = result.stdout
        rtt_values = [10000, 15000, 25000, 18000, 22000]  # sample data - TODO: fix
        return jsonify({'status': 'success', 'output': output, "rtt_values": rtt_values})
    except Exception as e:
        return jsonify({'status': 'error', 'message': str(e)}), 500

if __name__ == '__main__':
    app.run(debug=True)

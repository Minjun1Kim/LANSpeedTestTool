# LAN Speed Testing Tool

This tool simulates a simple LAN speed test similar to `iperf`. It measures upload, download, ping, and jitter in a simulated or real network environment.

![image](https://github.com/user-attachments/assets/3514f1b3-5fff-46c4-85cc-96a09ee4b204)

## Features
- Upload speed test: measures throughput from client to server.
- Download speed test: measures throughput from server to client.
- Ping test: measures round-trip time (RTT) using UDP.
- Jitter test: measures variations in RTT using TCP.

## Installation
1. **Install Mininet** (for testing):
    - **Using a VM**:
        - Install & Setup the Mininet VM from [Mininet Official Website](http://mininet.org/download/). 
        - Recommended: use Mininet VM image provided by Marcelo.
2. ssh into Mininet VM.
3. Clone the repository and change directory into the project
4. Compile the program using the `Makefile`:

    ```bash
    make
    ```
5. Install the `glib.h` library
```shell
sudo apt-get install libglib2.0-dev
```



## Usage
Run the program with the following options:

```bash
lan_speed [options]
Options:
  -m, --mode       Mode of operation: server or client
  -t, --test       Test type: upload, download, ping, jitter
  -a, --address    Server address (for client mode)
  -p, --port       Port number (default: 8080)
  -s, --size       Packet size in bytes (default: 1024)
  -n, --num        Number of packets (for jitter test, default: 10)
  -d, --duration   Test duration in seconds (default: 10)
  -h, --help       Display this help message
```

## Running Tests in Mininet
Use the provided `custom_topo.py` to create a custom Mininet topology. <br/>
This script sets up a topology with multiple hosts connected to a single switch, allowing you to run concurrent tests. <br/>

1. Run the Custom Topology

In your terminal, execute the custom topology script with sudo:

```shell
sudo python3 custom_topo.py
```

2. Start the Server on h1
Within the Mininet CLI:

```shell
mininet> h1 ./lan_speed -m server -p 8080 &
```

This command starts the server in the background on host h1, listening on port 8080.


3. Run Concurrent Client Tests on h2, h3, etc.
Execute various tests on client hosts. Below are examples of different test scenarios, including concurrent and mixed tests.

- Ping and Upload Concurrently
```shell
mininet> h2 ./lan_speed -m client -t ping -a 10.0.0.1 -p 8080 -s 64 -d 10 -r udp > h2_ping.log &
mininet> h3 ./lan_speed -m client -t upload -a 10.0.0.1 -p 8080 -s 1024 -d 10
```

- Two Ping Tests Concurrently
```shell
mininet> h2 ./lan_speed -m client -t ping -a 10.0.0.1 -p 8080 -s 64 -d 10 -r udp > h2_ping1.log &
mininet> h3 ./lan_speed -m client -t ping -a 10.0.0.1 -p 8080 -s 64 -d 10 -r udp
```

- Concurrent ICMP Ping Tests
```shell
mininet> h2 sudo ./lan_speed -m client -t ping -r icmp -a 10.0.0.1 -p 8080 -s 64 -d 10 -i 1 > h2_ping.log &                                                                                                       │
mininet> h3 sudo ./lan_speed -m client -t ping -r icmp -a 10.0.0.1 -p 8080 -s 64 -d 10 -i 1
```


- Ping and Download Concurrently
```shell
mininet> h2 ./lan_speed -m client -t ping -a 10.0.0.1 -p 8080 -s 128 -d 10 -r udp > h2_ping_concurrent.log &
mininet> h3 ./lan_speed -m client -t download -a 10.0.0.1 -p 8080 -s 1024 -d 10
```

- Multiple Uploads Concurrently
```shell
mininet> h2 ./lan_speed -m client -t upload -a 10.0.0.1 -p 8080 -s 1024 -d 10 > h2_upload1.log &
mininet> h3 ./lan_speed -m client -t upload -a 10.0.0.1 -p 8080 -s 1024 -d 10
```

- Repeated Download Tests
```shell
mininet> h2 ./lan_speed -m client -t download -a 10.0.0.1 -p 8080 -s 1024 -d 10 > h2_download1.log
mininet> h3 ./lan_speed -m client -t download -a 10.0.0.1 -p 8080 -s 1024 -d 10
```

- Large Data Transfer (Long Duration):
```shell
mininet> h2 ./lan_speed -m client -t upload -a 10.0.0.1 -p 8080 -s 1024 -d 60
```

- Small Packet Ping Test on h2:
```shell
mininet> h2 ./lan_speed -m client -t ping -a 10.0.0.1 -p 8080 -s 32 -d 10 -r udp
```

4. View Test Results
After tests complete, retrieve and view log files:
```shell
mininet> h2 cat h2_ping.log
mininet> h3 cat h3_upload_tcp.log
```




    ```bash
    ./lan_speed -m client -t jitter -a 127.0.0.1 -p 8080 -s 1024 -n 50
    ```
    In this example, the jitter test uses a packet size of 1024 bytes and sends 50 packets.

## Running the Web Interface (on branch: web)
A simple Flask-based web UI is provided in the /web directory: <br/>
0. Switch to web branch
``` shell
git fetch origin
git checkout -b web origin/web
```

1. Navigate to the /web directory:

```shell
cd web
```

2. Set up a virtual environment (recommended): <br/>

```shell
python3 -m venv venv
source venv/bin/activate
pip install flask
```

3. Run the Flask server: <br/>

```shell
python3 app.py
```

4. Open your browser and go to: <br/>

http://127.0.0.1:5000 <br/>

Use the web form to run tests. The results will be displayed on the page, and jitter tests can be visualized as a graph. <br/>
Note: use 127:0.0.1 as the server address in the web form. <br/>


## Development Notes
1. Protocols Used <br/>
    TCP: Used for upload, download, and jitter tests. <br/>
    UDP: Used for the ping test to simulate ICMP behavior. <br/> 
2. Port Usage <br/>
    Default port is 8080 for TCP tests. <br/>
    UDP-based Ping: Port 8080 is also used for UDP-based ping tests after dynamic port allocation for each UDP client.  <br/>

3. Concurrent UDP Handling <br/>
    Each UDP client is assigned a unique ephemeral port to handle concurrent UDP tests without packet interleaving. <br/>
    The server dynamically allocates ports and communicates them to clients to ensure isolated communication channels. <br/>

4. Mininet Integration <br/>
    The tool is designed to work within Mininet environments, allowing multiple virtual hosts to perform various tests concurrently. <br/>
    Ensure that Mininet hosts have network connectivity and appropriate routing to communicate with the server host. <br/>
    Use the provided custom_topo.py to create a custom topology that facilitates concurrent testing. <br/>

## License
This project is licensed under the MIT License.

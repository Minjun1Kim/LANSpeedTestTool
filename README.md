# LAN Speed Testing Tool

This tool simulates a simple LAN speed test similar to `iperf`. It measures upload, download, ping, and jitter in a simulated or real network environment.

## Features
- Upload speed test: measures throughput from client to server.
- Download speed test: measures throughput from server to client.
- Ping test: measures round-trip time (RTT) using UDP.
- Jitter test: measures variations in RTT using TCP.

## Installation
1. Clone the repository.
2. Compile the program using the `Makefile`:

    ```bash
    make
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
  -d, --duration   Test duration in seconds (default: 10)
  -h, --help       Display this help message
```

## Example
1. Start the server on the desired port:

    ```bash
    ./lan_speed -m server -p 8080
    ```

2. Run the client:

    ```bash
    ./lan_speed -m client -t upload -a 127.0.0.1 -p 8080 -s 1024 -d 10
    ```

    ```bash
    ./lan_speed -m client -t download -a 127.0.0.1 -p 8080 -s 1024 -d 10
    ```

    ```bash
    ./lan_speed -m client -t ping -a 127.0.0.1 -d 5
    ```

    ```bash
    ./lan_speed -m client -t jitter -a 127.0.0.1 -p 8080 -s 1024 -d 10
    ```

## How it works:

1. Server:

    Listens on the specified port. <br/>
    Waits for incoming client connections. <br/>
    Handles tests based on the test type received from the client. <br/>

2. Client:

    Connects to the server and informs it of the test type (e.g., upload, download, etc.). <br/>
    Executes the specified test and reports results like throughput, RTT, or jitter. <br/>

3. Metrics:

    Throughput: Calculated in Mbps (bits per second). <br/>
    RTT: Round-trip time for ping is reported in microseconds. <br/>
    Jitter: Average variation in RTT between packets. <br/>

## Development Notes
1. Protocols Used <br/>
    TCP: Used for upload, download, and jitter tests. <br/>
    UDP: Used for the ping test to simulate ICMP behavior. <br/> 
2. Port Usage <br/>
    Default port is 8080 for TCP tests. <br/>
    For UDP-based ping, port 12345 is used by default. <br/>


## License
This project is licensed under the MIT License.
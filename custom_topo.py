from mininet.topo import Topo
from mininet.net import Mininet
from mininet.node import Controller
from mininet.cli import CLI
from mininet.log import setLogLevel

class CustomTopo(Topo):
    def build(self):
        # Create 3 hosts
        host1 = self.addHost('h1')
        host2 = self.addHost('h2')
        host3 = self.addHost('h3')
        
        # Create 2 switches
        switch1 = self.addSwitch('s1')
        switch2 = self.addSwitch('s2')
        
        # Add links
        self.addLink(host1, switch1)
        self.addLink(host1, switch2)
        self.addLink(switch1, host2, bw=10)    # 10 Mbps bandwidth
        self.addLink(switch2, host3, bw=10)    # 10 Mbps bandwidth

        self.addLink(switch1, switch2, bw=10)  # 10 Mbps bandwidth

def run():
    topo = CustomTopo()
    net = Mininet(topo=topo, controller=Controller)
    net.start()
    
    # Start the Mininet CLI
    CLI(net)
    
    # Stop the network after the CLI session ends
    net.stop()

if __name__ == '__main__':
    setLogLevel('info')
    run()
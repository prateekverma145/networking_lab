from scapy.all import rdpcap, IP, TCP, UDP, ICMP, ARP
from collections import Counter
import matplotlib.pyplot as plt

def analyze_pcap(file_path):
    packets = rdpcap(file_path)
    
    tcp_to_port = 0
    udp_from_specific = 0
    
    specific_ip = input("Enter the IP address (default '172.16.23.84'): ").strip()
    if specific_ip == "":
        specific_ip = '172.16.23.84'
        
    specific_port_input = input("Enter the port number (default 12345): ").strip()
    if specific_port_input == "":
        specific_port = 12345
    else:
        try:
            specific_port = int(specific_port_input)
        except ValueError:
            print("Invalid port number entered. Using default port 12345.")
            specific_port = 12345
    
    packet_types = Counter()
    dst_hosts = Counter()
    src_host_port = Counter()
    src_host_protocols = {}
    netcat_flows = Counter()
    netcat_udp_flows = set()
    
    for packet in packets:
        if IP in packet:
            src_ip = packet[IP].src
            dst_ip = packet[IP].dst
            dst_hosts[dst_ip] += 1
            if TCP in packet:
                packet_types['TCP'] += 1
                tcp = packet[TCP]
                if tcp.dport == specific_port:
                    tcp_to_port += 1
                src_host_port[(src_ip, tcp.sport)] += 1
                if src_ip not in src_host_protocols:
                    src_host_protocols[src_ip] = Counter()
                src_host_protocols[src_ip]['TCP'] += 1
                
                if tcp.dport == specific_port or tcp.sport == specific_port:
                    netcat_flows[(src_ip, dst_ip, tcp.sport, tcp.dport)] += 1
            elif UDP in packet:
                packet_types['UDP'] += 1
                udp = packet[UDP]
                
                # Debugging print statements
                print(f"Checking UDP packet: src_ip={src_ip}, udp.sport={udp.sport}")
                
                if src_ip == specific_ip and udp.sport == specific_port:
                    udp_from_specific += 1
                src_host_port[(src_ip, udp.sport)] += 1
                if src_ip not in src_host_protocols:
                    src_host_protocols[src_ip] = Counter()
                src_host_protocols[src_ip]['UDP'] += 1
                
                if udp.dport == specific_port or udp.sport == specific_port:
                    netcat_udp_flows.add((src_ip, dst_ip))
            elif ICMP in packet:
                packet_types['ICMP'] += 1
                if src_ip not in src_host_protocols:
                    src_host_protocols[src_ip] = Counter()
                src_host_protocols[src_ip]['ICMP'] += 1
        elif ARP in packet:
            packet_types['ARP'] += 1
            arp = packet[ARP]
            src_ip = arp.psrc
            if src_ip not in src_host_protocols:
                src_host_protocols[src_ip] = Counter()
            src_host_protocols[src_ip]['ARP'] += 1
            
    # Print results
    print(f"i. TCP packets to destination port {specific_port}: {tcp_to_port}")
    print(f"ii. UDP packets from {specific_ip} and port {specific_port}: {udp_from_specific}")
    
    # iii. Draw plot
    plt.figure(figsize=(10, 6))
    plt.bar(packet_types.keys(), packet_types.values(), color='skyblue')
    plt.title('Packet Type Distribution')
    plt.xlabel('Packet Type')
    plt.ylabel('Number of Packets')
    plt.tight_layout()
    
    plt.savefig('packet_distribution.png')
    plt.close()
    print("iii. Packet distribution plot saved as 'packet_distribution.png'")
    
    if dst_hosts:
        top_dst_host = dst_hosts.most_common(1)[0]
        print(f"iv. Destination host receiving highest number of packets: {top_dst_host[0]} ({top_dst_host[1]} packets)")
    else:
        print("iv. No destination hosts found.")
        
    if src_host_port:
        top_src_host_port = src_host_port.most_common(1)[0]
        print(f"v. Source host and port sending highest number of packets: {top_src_host_port[0]} ({top_src_host_port[1]} packets)")
    else:
        print("v. No source hosts found.")
    
    print("vi. Source host, types of protocol packets, and count of each:")
    for src_ip, protocol_counts in src_host_protocols.items():
        print(f"  {src_ip}: {dict(protocol_counts)}")
        
    print("vii. Number of packets in netcat TCP flows:")
    for flow, count in netcat_flows.items():
        print(f"  Flow {flow}: {count} packets")
    
    print(f"viii. Number of netcat TCP flows: {len(netcat_flows)}")
    print(f"      Number of netcat UDP flows: {len(netcat_udp_flows)}")

fp=input("input filename:")
analyze_pcap(fp)

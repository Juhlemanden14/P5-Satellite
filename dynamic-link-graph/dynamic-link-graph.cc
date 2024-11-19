#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"  // Include internet module for IP addresses
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/point-to-point-module.h"

#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/network-module.h"


#include "ns3/netanim-module.h"     // NetAnimator
#include "ns3/mobility-module.h"    // Mobility module to give them position

#include <unordered_map>
#include <vector>
#include <algorithm> // for std::remove
#include <utility>   // for std::make_pair and std::pair
#include <cstdint>   // For uint32_t

using namespace ns3;

struct pair_hash {      // custom hash-function for hashing a pair (not supported in unordered map by default)
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& pair) const {
        // Hash the individual elements of the pair and combine them
        return std::hash<T1>()(pair.first) ^ (std::hash<T2>()(pair.second) << 1);
    }
};

// Type definition for adjacency list (node ID -> list of connected node IDs)
std::unordered_map<uint32_t, std::vector<uint32_t>> adjacencyList;
std::unordered_map<std::pair<uint32_t, uint32_t>, std::pair<NetDeviceContainer, Ipv4InterfaceContainer>, pair_hash> linkMap;

// Function to add a link between two nodes with IP assignment
void AddLink(Ptr<Node> nodeA, Ptr<Node> nodeB, PointToPointHelper &csmaHelper, Ipv4AddressHelper &address) {
    uint32_t idA = nodeA->GetId();
    uint32_t idB = nodeB->GetId();
    
    // Check if link already exists
    if (std::find(adjacencyList[idA].begin(), adjacencyList[idA].end(), idB) == adjacencyList[idA].end()) {
        adjacencyList[idA].push_back(idB);
        adjacencyList[idB].push_back(idA);
        
        // Install CSMA link
        NetDeviceContainer link = csmaHelper.Install(NodeContainer(nodeA, nodeB));
        
        // Assign IP addresses to the devices on the link
        Ipv4InterfaceContainer interfaces = address.Assign(link);
        
        // Store the link with IP interfaces in linkMap
        linkMap[std::make_pair(idA, idB)] = std::make_pair(link, interfaces);
        
        // Move to a new network for the next link
        address.NewNetwork();
    }
}

// Function to remove a link between two nodes
void RemoveLink(uint32_t idA, uint32_t idB) {
    // Remove from adjacency list
    adjacencyList[idA].erase(std::remove(adjacencyList[idA].begin(), adjacencyList[idA].end(), idB), adjacencyList[idA].end());
    adjacencyList[idB].erase(std::remove(adjacencyList[idB].begin(), adjacencyList[idB].end(), idA), adjacencyList[idB].end());
    
    // Find the link in linkMap
    auto linkPair = std::make_pair(idA, idB);
    if (linkMap.find(linkPair) != linkMap.end()) {
        NetDeviceContainer &linkDevices = linkMap[linkPair].first;
        
        // Iterate over devices in the NetDeviceContainer using GetN() and Get()
        for (uint32_t i = 0; i < linkDevices.GetN(); ++i) {
            linkDevices.Get(i)->SetAttribute("IsEnabled", BooleanValue(false));  // Disable each device
        }
        
        // Remove the link from linkMap after disabling it
        linkMap.erase(linkPair);
    }
}

int main(int argc, char *argv[]) {
    // Set up nodes and CSMA helper
    NodeContainer nodes;
    nodes.Create(5);  // 5 nodes (n0, n1, ..., n4)


    // CsmaHelper csmaHelper;
    PointToPointHelper csmaHelper;

    // Enable internet stack on all nodes
    InternetStackHelper internet;
    internet.Install(nodes);

    // Initialize the address helper with a base IP and subnet mask
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");

    // csmaHelper.SetChannelAttribute("DataRate", StringValue("5MBps"));
    // csmaHelper.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));

    // Add initial links with IP address assignment
    AddLink(nodes.Get(0), nodes.Get(1), csmaHelper, address);
    AddLink(nodes.Get(1), nodes.Get(2), csmaHelper, address);
    AddLink(nodes.Get(2), nodes.Get(3), csmaHelper, address);
    AddLink(nodes.Get(3), nodes.Get(4), csmaHelper, address);

    // Example: Remove and add links dynamically during runtime
    // Simulator::Schedule(Seconds(10), &RemoveLink, 0, 3);  // Remove link between n0 and n3 at 10 seconds
    Simulator::Schedule(Seconds(2), &AddLink, nodes.Get(4), nodes.Get(0), csmaHelper, address);  // Add link between n2 and n4 at 20 seconds
    Simulator::Schedule(Seconds(2.01), &Ipv4GlobalRoutingHelper::PopulateRoutingTables);

    Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue(true));

    // TCP Server receiving data
    PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 7777));
    ApplicationContainer serverApps = sink.Install(nodes.Get(0));
    serverApps.Start(Seconds(0));

    // TCP Client
    // NS_LOG_UNCOND("Server IP: " << nodes.Get(0)->GetDevice());
    // OnOffHelper onOffHelper("ns3::TcpSocketFactory", InetSocketAddress(In2n5.GetAddress(1), 7777));
    // onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.0]"));
    // onOffHelper.SetAttribute("PacketSize", UintegerValue(512));
    // ApplicationContainer clientApps = onOffHelper.Install(nodes.Get(6));
    // clientApps.Start(Seconds(0.1));
    // clientApps.Stop(Seconds(10));











    //Simulation NetAnim configuration and node placement
    MobilityHelper mobility;
    mobility.Install(nodes);

    // Create .xml file for NetAnimator (to see simulation visually)
    AnimationInterface anim("dynamic-link-graph.xml");
    anim.EnablePacketMetadata();
    AnimationInterface::SetConstantPosition(nodes.Get(0), 3, 0);
    AnimationInterface::SetConstantPosition(nodes.Get(1), 6, 0);
    AnimationInterface::SetConstantPosition(nodes.Get(2), 7, 3);
    AnimationInterface::SetConstantPosition(nodes.Get(3), 4.5, 6);
    AnimationInterface::SetConstantPosition(nodes.Get(4), 1, 3);






    NS_LOG_UNCOND("[+] Simulation started");
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_UNCOND("[!] Simulation stopped");

    return 0;
}

/*
 * Author: P5
 */

// P5 Blackboard Topology


#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

#include "ns3/netanim-module.h"     // NetAnimator
#include "ns3/mobility-module.h"    // Mobility module to give them position
using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Blackboard");


int receivedPackets = 0;
void PacketReceived(Ptr<const Packet> packet, const Address& address) {
    receivedPackets++;
    NS_LOG_DEBUG("Server received a packet[" << receivedPackets << "] of " << packet->GetSize() << " bytes. From " << address);
}

int main(int argc, char* argv[]) {
    LogComponentEnable("GlobalRouteManager", LOG_LEVEL_ALL);
    // LogComponentEnable("GlobalRouteManagerImpl", LOG_LEVEL_ALL);


    Config::SetDefault("ns3::Ipv4GlobalRouting::RespondToInterfaceEvents", BooleanValue(true));
    Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue(true));
    PacketMetadata::Enable();


    std::string targetIP = "10.1.1.1";
    CommandLine cmd(__FILE__);
    cmd.AddValue("targetIP", "", targetIP);
    cmd.Parse(argc, argv);

    NodeContainer nodes(7);
    InternetStackHelper stack;
    stack.Install(nodes);

    // PointToPointHelper csma;
    CsmaHelper csma;     // Error for some fucking reason
    NetDeviceContainer n0n1 = csma.Install(NodeContainer(nodes.Get(0), nodes.Get(1)));
    NetDeviceContainer n0n3 = csma.Install(NodeContainer(nodes.Get(0), nodes.Get(3)));
    NetDeviceContainer n1n2 = csma.Install(NodeContainer(nodes.Get(1), nodes.Get(2)));
    NetDeviceContainer n3n4 = csma.Install(NodeContainer(nodes.Get(3), nodes.Get(4)));
    NetDeviceContainer n2n4 = csma.Install(NodeContainer(nodes.Get(2), nodes.Get(4)));
    NetDeviceContainer n2n5 = csma.Install(NodeContainer(nodes.Get(2), nodes.Get(5)));

    NetDeviceContainer n0n6 = csma.Install(NodeContainer(nodes.Get(0), nodes.Get(6)));

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer In0n1 = address.Assign(n0n1);
    address.NewNetwork();
    Ipv4InterfaceContainer In0n3 = address.Assign(n0n3);
    address.NewNetwork();
    Ipv4InterfaceContainer In1n2 = address.Assign(n1n2);
    address.NewNetwork();
    Ipv4InterfaceContainer In3n4 = address.Assign(n3n4);
    address.NewNetwork();
    Ipv4InterfaceContainer In2n4 = address.Assign(n2n4);
    address.NewNetwork();
    Ipv4InterfaceContainer In2n5 = address.Assign(n2n5);
    address.NewNetwork();
    Ipv4InterfaceContainer In0n6 = address.Assign(n0n6);


    Ptr<Ipv4> ipv41 = nodes.Get(1)->GetObject<Ipv4>();
    // The first ifIndex is 0 for loopback, then the first p2p is numbered 1,
    // then the next p2p is numbered 2
    uint32_t ipv4ifIndex1 = 2;
    Simulator::Schedule(Seconds(1), &Ipv4::SetDown, ipv41, ipv4ifIndex1);
    Simulator::Schedule(Seconds(2), &Ipv4::SetUp, ipv41, ipv4ifIndex1);


    // TCP Server receiving data
    PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 7777));
    ApplicationContainer serverApps = sink.Install(nodes.Get(5));
    serverApps.Start(Seconds(0));
    serverApps.Get(0)->TraceConnectWithoutContext("Rx", MakeCallback(&PacketReceived));

    // TCP Client
    NS_LOG_UNCOND("Server IP: " << In2n5.GetAddress(1));
    OnOffHelper onOffHelper("ns3::TcpSocketFactory", InetSocketAddress(In2n5.GetAddress(1), 7777));
    onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.0]"));
    onOffHelper.SetAttribute("PacketSize", UintegerValue(512));
    ApplicationContainer clientApps = onOffHelper.Install(nodes.Get(6));
    clientApps.Start(Seconds(0));
    clientApps.Stop(Seconds(5));


    
    // Populate every nodes routing table using God Routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();



    //Simulation NetAnim configuration and node placement
    MobilityHelper mobility;
    mobility.Install(nodes);

    // Create .xml file for NetAnimator (to see simulation visually)
    AnimationInterface anim("blackboard.xml");
    anim.EnablePacketMetadata();
    AnimationInterface::SetConstantPosition(nodes.Get(0), 0, 1);
    AnimationInterface::SetConstantPosition(nodes.Get(1), 5.0, 1);
    AnimationInterface::SetConstantPosition(nodes.Get(2), 10, 1);
    AnimationInterface::SetConstantPosition(nodes.Get(3), 2.5, 5);
    AnimationInterface::SetConstantPosition(nodes.Get(4), 8, 5);
    AnimationInterface::SetConstantPosition(nodes.Get(5), 12, 1);
    AnimationInterface::SetConstantPosition(nodes.Get(6), -2, 1);


    // Generate .pcap files from each node
    csma.EnablePcapAll("blackboard", true);

    NS_LOG_UNCOND("[+] Simulation started");
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_UNCOND("[!] Simulation stopped");

    NS_LOG_UNCOND("Server received " << receivedPackets << " packets");


    return 0;
}


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

// Trace congestion window
static void CwndTracer(uint32_t oldval, uint32_t newval) {
    NS_LOG_UNCOND( "CWND: " << Simulator::Now().GetSeconds() << " > \t" << oldval << " --> " << newval);
}
// static void CwndChange(uint32_t oldCwnd, uint32_t newCwnd)
// {
//     NS_LOG_UNCOND(Simulator::Now().GetSeconds() << "\t" << newCwnd);
// }

void RWNDTracer(uint32_t oldval, uint32_t newval) {
    NS_LOG_UNCOND( "RWND: " << Simulator::Now().GetSeconds() << " > \t" << oldval << " --> " << newval);
}

void AdvWNDTracer(uint32_t oldval, uint32_t newval) {
    NS_LOG_UNCOND( "AdvWND: " << Simulator::Now().GetSeconds() << " > \t" << oldval << " --> " << newval);
}

void TraceCwnd(NodeContainer n, uint32_t nodeId, uint32_t socketId) {

    NS_LOG_UNCOND("CW Window traced Node id: " << nodeId << " socketid: " << socketId);
    Config::ConnectWithoutContext("/NodeList/" + std::to_string(nodeId) + "/$ns3::TcpL4Protocol/SocketList/" + std::to_string(socketId) + "/CongestionWindow", MakeBoundCallback(&CwndTracer));
    Config::ConnectWithoutContext("/NodeList/" + std::to_string(nodeId) + "/$ns3::TcpL4Protocol/SocketList/" + std::to_string(socketId) + "/RWND", MakeBoundCallback(&RWNDTracer));
    Config::ConnectWithoutContext("/NodeList/" + std::to_string(nodeId) + "/$ns3::TcpL4Protocol/SocketList/" + std::to_string(socketId) + "/AdvWND", MakeBoundCallback(&AdvWNDTracer));
    
    for (uint32_t i = 0; i < n.GetN(); i++){
        NS_LOG_UNCOND("Node " << i);
        
        ObjectMapValue m_sockets;
        n.Get(i)->GetObject<TcpL4Protocol>()->GetAttribute("SocketList", m_sockets);
        

        if (m_sockets.GetN() >= 1) {
            NS_LOG_UNCOND("     Socket count: " << m_sockets.GetN());

            NS_LOG_UNCOND(m_sockets.Get(0)->GetTypeId().GetName());
            Ptr<TcpSocketBase> socket = DynamicCast<TcpSocketBase>(m_sockets.Get(0));
            NS_LOG_UNCOND(socket->GetTypeId().GetName());
            socket->SetMinRto(Time(Seconds(2)));
            
            NS_LOG_UNCOND(socket->GetMinRto() << " | " << socket->GetSocketType());
        }


        // std::pair<const uint64_t, ns3::Ptr<ns3::TcpSocketBase>> &socketItem
        // for (auto& socketItem : m_sockets) {
        //     NS_LOG_UNCOND("Hej");
        // }

        
        // for (ObjectPtrContainerValue::Iterator it = m_sockets.Begin(); it != m_sockets.End(); ++it) {
        // }
    }
}


int main(int argc, char* argv[]) {
    // LogComponentEnable("GlobalRouteManager", LOG_LEVEL_ALL);
    // LogComponentEnable("GlobalRouteManagerImpl", LOG_LEVEL_ALL);

    // Auto recompute routes at link changes
    // Config::SetDefault("ns3::Ipv4GlobalRouting::RespondToInterfaceEvents", BooleanValue(true));
    Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue(true));

    PacketMetadata::Enable();

    std::string targetIP = "10.1.1.1";
    CommandLine cmd(__FILE__);
    cmd.AddValue("targetIP", "", targetIP);
    cmd.Parse(argc, argv);

    NodeContainer nodes(7);
    InternetStackHelper stack;
    stack.Install(nodes);

    // PointToPointHelper csmaHelper;
    CsmaHelper csmaHelper;     // Error for some fucking reason
    NetDeviceContainer n0n1 = csmaHelper.Install(NodeContainer(nodes.Get(0), nodes.Get(1)));
    NetDeviceContainer n0n3 = csmaHelper.Install(NodeContainer(nodes.Get(0), nodes.Get(3)));
    NetDeviceContainer n1n2 = csmaHelper.Install(NodeContainer(nodes.Get(1), nodes.Get(2)));
    NetDeviceContainer n2n4 = csmaHelper.Install(NodeContainer(nodes.Get(2), nodes.Get(4)));
    NetDeviceContainer n2n5 = csmaHelper.Install(NodeContainer(nodes.Get(2), nodes.Get(5)));
    NetDeviceContainer n3n4 = csmaHelper.Install(NodeContainer(nodes.Get(3), nodes.Get(4)));
    // Fuck one of the links in the lower region
    csmaHelper.SetChannelAttribute("DataRate", StringValue("1KBps"));
    csmaHelper.SetChannelAttribute("Delay", StringValue("10ms"));
    NetDeviceContainer n0n6 = csmaHelper.Install(NodeContainer(nodes.Get(0), nodes.Get(6)));


    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0"); // This subnet can be 255.255.255.252 to reduce the host range to only 2 bits!
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

    Ptr<Ipv4> ipv11 = nodes.Get(1)->GetObject<Ipv4>();
    // The first ifIndex is 0 for loopback, then the first p2p is numbered 1, then the next p2p is numbered 2
    // uint32_t ipv4Index = 2;
    // Simulator::Schedule(Seconds(1), &Ipv4::SetDown, ipv11, ipv4Index);
    // Simulator::Schedule(Seconds(2), &Ipv4GlobalRoutingHelper::PopulateRoutingTables);

    // Simulator::Schedule(Seconds(6), &Ipv4::SetUp, ipv11, ipv4Index);
    // Simulator::Schedule(Seconds(7), &Ipv4GlobalRoutingHelper::PopulateRoutingTables);

    // Ptr<Ipv4> ipv41 = nodes.Get(4)->GetObject<Ipv4>();
    // Simulator::Schedule(Seconds(3), &Ipv4::SetDown, ipv41, 2);
    // Simulator::Schedule(Seconds(4), &Ipv4GlobalRoutingHelper::PopulateRoutingTables);


    Simulator::Schedule(Seconds(0.653), [nodes](){
        NS_LOG_UNCOND("[!] SECOND 0.653");
        Ptr<Ipv4> ipv4_0 = nodes.Get(0)->GetObject<Ipv4>();
        Ptr<Ipv4> ipv4_1 = nodes.Get(1)->GetObject<Ipv4>();
        NS_LOG_UNCOND("Second4:" << ipv4_0->GetAddress(1, 0).GetAddress());
        NS_LOG_UNCOND("Second4:" << ipv4_1->GetAddress(1, 0).GetAddress());
        // Optional, but required for our simulation
        ipv4_0->RemoveAddress(1, 0);
        ipv4_1->RemoveAddress(1, 0);
        // Required for Recomputing Routing Tables, such that these interfaces are not included in the populating of routing tables
        ipv4_0->SetDown(1);
        ipv4_1->SetDown(1);


        // Ptr<Ipv4> ipv4_3 = nodes.Get(3)->GetObject<Ipv4>();
        // NS_LOG_UNCOND("Second4:" << ipv4_0->GetAddress(2, 0).GetAddress());
        // NS_LOG_UNCOND("Second4:" << ipv4_3->GetAddress(1, 0).GetAddress());
        // ipv4_0->RemoveAddress(2, 0);
        // ipv4_3->RemoveAddress(1, 0);
        // ipv4_0->SetDown(2);
        // ipv4_3->SetDown(1);
        
        // RECOMPUTE routes instead of populating, which now works
        Ipv4GlobalRoutingHelper::RecomputeRoutingTables();
        NS_LOG_UNCOND("Routes populated");
    });

    Simulator::Schedule(Seconds(0.8), [nodes](){
        NS_LOG_UNCOND("[!] SECOND 0.8");
        Ptr<Ipv4> ipv4_0 = nodes.Get(0)->GetObject<Ipv4>();
        Ptr<Ipv4> ipv4_3 = nodes.Get(3)->GetObject<Ipv4>();

        Ipv4InterfaceAddress newAddress1 = Ipv4InterfaceAddress(Ipv4Address("192.168.1.1"), Ipv4Mask("255.255.255.0"));
        Ipv4InterfaceAddress newAddress2 = Ipv4InterfaceAddress(Ipv4Address("192.168.1.2"), Ipv4Mask("255.255.255.0"));
        ipv4_0->AddAddress(2, newAddress1);
        bool hej = ipv4_3->AddAddress(1, newAddress2);
        NS_LOG_UNCOND("Was addresses added? " << hej);
        ipv4_0->SetUp(2);
        ipv4_3->SetUp(1);
        NS_LOG_UNCOND("Hej");
        
        Ipv4GlobalRoutingHelper::RecomputeRoutingTables();
    });


    Config::SetDefault("ns3::TcpSocketBase::MinRto", TimeValue(Seconds(1)));


    // TCP Server receiving data
    PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 7777));
    ApplicationContainer serverApps = sink.Install(nodes.Get(2));
    serverApps.Start(Seconds(0));
    serverApps.Get(0)->TraceConnectWithoutContext("Rx", MakeCallback(&PacketReceived));

    // TCP Client
    Ipv4Address serverIP = nodes.Get(2)->GetObject<Ipv4>()->GetAddress(3, 0).GetAddress();
    NS_LOG_UNCOND("Sending to Server IP: " << serverIP);
    OnOffHelper onOffHelper("ns3::TcpSocketFactory", InetSocketAddress(serverIP, 7777));
    // onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.2]"));
    // onOffHelper.SetAttribute("PacketSize", UintegerValue(512));
    onOffHelper.SetConstantRate(DataRate("1Kbps"), 100);
    ApplicationContainer clientApps = onOffHelper.Install(nodes.Get(6));
    clientApps.Start(Seconds(0.1));
    clientApps.Stop(Seconds(10));


    // Config::ConnectWithoutContext("/NodeList/6/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",
    //                               MakeCallback(&CwndTracer));

    // std::unordered_map<uint64_t, Ptr<TcpSocketBase>> m_sockets;
    Simulator::Schedule(Seconds(0.1) + MilliSeconds(1), &TraceCwnd, nodes, 6, 0);


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
    csmaHelper.EnablePcapAll("scratch/P5-Satellite/Prototype/blackboard", true);

    Ptr<OutputStreamWrapper> routingStream =  Create<OutputStreamWrapper>("scratch/P5-Satellite/Prototype/blackboardS2.routes", std::ios::out);
    Ipv4RoutingHelper::PrintRoutingTableAllAt(Seconds(2), routingStream);
    Ptr<OutputStreamWrapper> routingStream2 =  Create<OutputStreamWrapper>("scratch/P5-Satellite/Prototype/blackboardS6.routes", std::ios::out);
    Ipv4RoutingHelper::PrintRoutingTableAllAt(Seconds(6), routingStream2);



    NS_LOG_UNCOND("[+] Simulation started");
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_UNCOND("[!] Simulation stopped");

    NS_LOG_UNCOND("Server received " << receivedPackets << " packets");


    return 0;
}


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
            NS_LOG_UNCOND(socket->GetMinRto());
            socket->SetMinRto(Time(Seconds(2)));
            
            NS_LOG_UNCOND(socket->GetMinRto());

            NS_LOG_UNCOND(socket->GetSocketType());
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
    NetDeviceContainer n0n6 = csmaHelper.Install(NodeContainer(nodes.Get(0), nodes.Get(6)));
    // Fuck one of the links in the lower region
    csmaHelper.SetChannelAttribute("DataRate", StringValue("1KBps"));
    NetDeviceContainer n3n4 = csmaHelper.Install(NodeContainer(nodes.Get(3), nodes.Get(4)));


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


    Ptr<Ipv4> ipv11 = nodes.Get(1)->GetObject<Ipv4>();
    // The first ifIndex is 0 for loopback, then the first p2p is numbered 1, then the next p2p is numbered 2
    uint32_t ipv4Index = 2;
    Simulator::Schedule(Seconds(1), &Ipv4::SetDown, ipv11, ipv4Index);
    Simulator::Schedule(Seconds(2), &Ipv4GlobalRoutingHelper::PopulateRoutingTables);

    Simulator::Schedule(Seconds(6), &Ipv4::SetUp, ipv11, ipv4Index);
    Simulator::Schedule(Seconds(7), &Ipv4GlobalRoutingHelper::PopulateRoutingTables);

    Ptr<Ipv4> ipv41 = nodes.Get(4)->GetObject<Ipv4>();
    Simulator::Schedule(Seconds(3), &Ipv4::SetDown, ipv41, 2);
    Simulator::Schedule(Seconds(4), &Ipv4GlobalRoutingHelper::PopulateRoutingTables);


    Config::SetDefault("ns3::TcpSocketBase::MinRto", TimeValue(Seconds(1)));



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
    clientApps.Start(Seconds(0.1));
    clientApps.Stop(Seconds(10));


    // Config::ConnectWithoutContext("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",
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
    csmaHelper.EnablePcapAll("blackboard", true);

    NS_LOG_UNCOND("[+] Simulation started");
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_UNCOND("[!] Simulation stopped");

    NS_LOG_UNCOND("Server received " << receivedPackets << " packets");


    return 0;
}


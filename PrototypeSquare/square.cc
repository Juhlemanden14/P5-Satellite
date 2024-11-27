#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/animation-interface.h"
#include "ns3/csma-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SquareTopologyExample");

int main(int argc, char *argv[])
{
    Time::SetResolution(Time::NS);
    LogComponentEnable("SquareTopologyExample", LOG_LEVEL_INFO);

    // Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue(false));

    NodeContainer nodes;
    nodes.Create(6);

    CsmaHelper pointToPoint;
    // pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    // pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    // pointToPoint.SetChannelAttribute("DataRate", StringValue("1KBps"));
    // pointToPoint.SetChannelAttribute("Delay", StringValue("10ms"));

    // border links
    NetDeviceContainer n0n1 = pointToPoint.Install(NodeContainer(nodes.Get(0), nodes.Get(1)));
    NetDeviceContainer n1n2 = pointToPoint.Install(NodeContainer(nodes.Get(1), nodes.Get(2)));
    NetDeviceContainer n2n3 = pointToPoint.Install(NodeContainer(nodes.Get(2), nodes.Get(3)));
    NetDeviceContainer n3n0 = pointToPoint.Install(NodeContainer(nodes.Get(3), nodes.Get(0)));
    // Cross links
    // NetDeviceContainer n0n2 = pointToPoint.Install(NodeContainer(nodes.Get(0), nodes.Get(2));
    // NetDeviceContainer n1n3 = pointToPoint.Install(NodeContainer(nodes.Get(1), nodes.Get(3));
    // Source and sink
    NetDeviceContainer n0n4 = pointToPoint.Install(NodeContainer(nodes.Get(0), nodes.Get(4)));
    NetDeviceContainer n2n5 = pointToPoint.Install(NodeContainer(nodes.Get(2), nodes.Get(5)));


    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    // Borders
    Ipv4InterfaceContainer in0n1 = address.Assign( n0n1 );
    address.NewNetwork();
    Ipv4InterfaceContainer in1n2 = address.Assign( n1n2 );
    address.NewNetwork();
    Ipv4InterfaceContainer in2n3 = address.Assign( n2n3 );
    address.NewNetwork();
    Ipv4InterfaceContainer in3n0 = address.Assign( n3n0 );
    address.NewNetwork();

    // Cross
    // Ipv4InterfaceContainer in0n2 = address.Assign( n0n2 );
    // address.NewNetwork();
    // Ipv4InterfaceContainer in1n3 = address.Assign( n1n3 );
    // address.NewNetwork();

    // Source and sink
    Ipv4InterfaceContainer in0n4 = address.Assign( n0n4 );
    address.NewNetwork();
    Ipv4InterfaceContainer in2n5 = address.Assign( n2n5 );
    address.NewNetwork();



    Ipv4GlobalRoutingHelper::PopulateRoutingTables();


    // Set up UDP client on the sink
    UdpClientHelper udpClient(Ipv4Address("10.1.6.2"), 7777);
    udpClient.SetAttribute("MaxPackets", UintegerValue(10));
    udpClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    ApplicationContainer clientApps = udpClient.Install(nodes.Get(4));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(10.0));


    AnimationInterface::SetConstantPosition(nodes.Get(0), 0, 0);
    AnimationInterface::SetConstantPosition(nodes.Get(1), 5, 0);
    AnimationInterface::SetConstantPosition(nodes.Get(2), 5, 5);
    AnimationInterface::SetConstantPosition(nodes.Get(3), 0, 5);
    // Source and sink
    AnimationInterface::SetConstantPosition(nodes.Get(4), -2, 0);
    AnimationInterface::SetConstantPosition(nodes.Get(5), 7, 5);

    AnimationInterface anim("square.xml");
    // for (uint32_t i = 0; i < nodes.GetN(); i++) {
    //     anim.UpdateNodeSize(i, 5, 5);
    // }


    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
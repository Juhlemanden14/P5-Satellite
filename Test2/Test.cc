#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"

#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"

#include "lib.h"

// Add the ns3 namespaces so we do not have to write ns3:: in front of every method call.
using namespace ns3;

// Name the logging component for this file
NS_LOG_COMPONENT_DEFINE("TestLog");

int main(int argc, char* argv[]) {
    
    // Only LogComponentEnable seems to work, the disable version just disables everything.
    LogComponentEnable("TestLog", LOG_INFO);

    NS_LOG( LOG_DEBUG, "WHAT1");
    NS_LOG( LOG_INFO, "HELLO1");

    LogComponentEnable("TestLog", LOG_LEVEL_ALL);

    NS_LOG( LOG_DEBUG, "WHAT2");
    NS_LOG( LOG_INFO, "HELLO2");

    // Enable UDP client logs
    //LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_FUNCTION);

    // Set the time resolution of the simulation to nanoseconds. Can only be set once.
    Time::SetResolution(Time::NS);

    // Create nodes using a container.
    NodeContainer nodes;
    // Add n new nodes.
    int node_amount = 9;
    nodes.Create(node_amount);

    // Set the attributes for the channel.
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("20kbps"));
    csma.SetChannelAttribute("Delay", StringValue("10ns"));

    // Install the devices on the nodes.
    NetDeviceContainer devices;
    devices = csma.Install(nodes);

    // Install the internet stack on the nodes.
    InternetStackHelper stack;
    stack.Install(nodes);

    // Set the ipv4 address of all the nodes using a base address.
    Ipv4AddressHelper address;
    address.SetBase("197.189.0.0","255.255.0.0");

    // Assign the generated ipv4 addresses to the nodes' devices.
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // Setup a udp echo server, with port 1000;
    UdpEchoServerHelper echoServer(1000);
    // Set the new port for the server to 5509.
    echoServer.SetAttribute("Port", UintegerValue(5509));

    // Install the echo server at the nodes index 1.
    ApplicationContainer serverApps = echoServer.Install(nodes.Get(1));

    // Tell the simulator in which time interval during the simulation that the echo server should run.
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(10.0));

    // Setup the UDP client.

    // Create the UDP client, that sends to address 1 with port 9000.
    UdpEchoClientHelper echoClient(interfaces.GetAddress(1), 9000);

    // Set client attributes
    echoClient.SetAttribute("MaxPackets", UintegerValue(20));
    echoClient.SetAttribute("Interval", TimeValue(MilliSeconds(5)));
    echoClient.SetAttribute("PacketSize", UintegerValue(64));

    // Create another UDP client.
    UdpEchoClientHelper echoClient2(interfaces.GetAddress(1), 5509);
    echoClient2.SetAttribute("MaxPackets", UintegerValue(10));
    echoClient2.SetAttribute("Interval", TimeValue(MilliSeconds(10)));
    echoClient2.SetAttribute("PacketSize", UintegerValue(128));

    // Install the echo client 1 & 2 on node with index 3 and 4.
    ApplicationContainer clientApps;
    clientApps = echoClient.Install(nodes.Get(2));
    clientApps = echoClient2.Install(nodes.Get(3));

    // Start the client in the interval 7-8 seconds in the simulation.
    clientApps.Start(Seconds(7.0));
    clientApps.Stop(Seconds(8.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();


    // Setup the mobility helper for node position.
    MobilityHelper mobility;
    mobility.Install(nodes);

    // Setup the animation output.
    AnimationInterface anim("test-1.xml");
    anim.EnablePacketMetadata();

    // Set position for nodes.
    int m = 0;
    for (int n = 0; n < 2; ++n) {
        for (int j = 0; j < 2; ++j) {
            anim.SetConstantPosition(nodes.Get(m), 360*n-180, 180*j-90);
            
            anim.UpdateNodeSize(m, 10, 10);
            ++m;
        }
    }
    // Set desc for server and client in the animation.
    anim.UpdateNodeDescription(nodes.Get(1), "Server");
    anim.UpdateNodeDescription(nodes.Get(2), "Client");
    anim.UpdateNodeDescription(nodes.Get(3), "Client2");

    anim.SetConstantPosition(nodes.Get(4), 9.5504, -57.0311); // 57°03'11"N 9°55'04"E
    anim.UpdateNodeDescription(nodes.Get(4), "Aalborg-bridge");

    anim.SetConstantPosition(nodes.Get(5), 15.3754, -78.1328); // 78°13'28"N 15°37'54"E
    anim.UpdateNodeDescription(nodes.Get(5), "Longyearbyen");

    anim.SetConstantPosition(nodes.Get(6), -82.2222, -23.0723); // 23°07'23"N 82°22'22"W
    anim.UpdateNodeDescription(nodes.Get(6), "Havana");

    anim.SetConstantPosition(nodes.Get(7), 151.1108, 33.5655); // 33°56'55"S 151°11'08"E
    anim.UpdateNodeDescription(nodes.Get(7), "Sydney");

    anim.SetConstantPosition(nodes.Get(8), -2.038273, -4.7529124);
    anim.UpdateNodeDescription(nodes.Get(8), "Fort Dorothea");


    // NodeContainer dummyNodes = NodeContainer(4);
    // anim.SetConstantPosition(nodes.Get(0), 1, 1);
    int number1 = 1231;
    NS_LOG( LOG_DEBUG, "test: " << testFunction(number1));


    // Scale to 360/2048 (360 degrees for 2048 pixels)
    anim.SetBackgroundImage("scratch/P5-Satellite/resources/earth-bw.jpeg", -180, -90, 0.17578125, 0.17578125, 1);

    // Generate .pcap files from each node.
    csma.EnablePcapAll("test-1", true);

    Simulator::Run();
    Simulator::Destroy();
    return 0;

}



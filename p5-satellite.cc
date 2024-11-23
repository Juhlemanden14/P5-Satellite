#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
// #include "ns3/satellite-module.h"
#include "ns3/csma-module.h"    // needed for dynamic cast to csmaNetDevice

#include "tleHandler.h"
#include "constellationHandler.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("P5-Satellite");
// Correct Net Animator folder path from P5-Satellite: './../../../netanim-3.109/NetAnim'
// Call in directory: 
// $ cd ns-3.42 


// // Function to schedule a position update. If verbose is set to true, it prints the new position
// void SchedulePositionUpdate(Ptr<SatSGP4MobilityModel> satMobility, Time t, bool verbose) {
//     Simulator::Schedule(t, [satMobility, t, verbose](){
//         // Retrieve the satellite's position at this time
//         GeoCoordinate pos = satMobility->GetGeoPosition();
//         if (verbose) {
//             std::cout << "Time: " << t.GetSeconds() << " sec, Position: ("
//                       << pos.GetLongitude() << ", " << pos.GetLatitude() << ", "
//                       << pos.GetAltitude() << ")" << std::endl;
//         }
//     });
// }


// Function to be scheduled periodically in the ns3 simulator.
void simulationPhase(NodeContainer &satellites, std::vector<Ptr<SatSGP4MobilityModel>> &satelliteMobilityModels, std::vector<Ptr<SatConstantPositionMobilityModel>> &groundStationsMobilityModels) {
    // NS_LOG_DEBUG("[->] Simulation at second " << Simulator::Now().GetSeconds());    // Gets the elapsed seconds in the simulation

    // Set the new positions of the satellites and update their position in Net Animator.
    for (uint32_t n = 0; n < satellites.GetN(); ++n) {
        GeoCoordinate satPos = satelliteMobilityModels[n]->GetGeoPosition();
        AnimationInterface::SetConstantPosition(satellites.Get(n), satPos.GetLongitude(), -satPos.GetLatitude());

        //NS_LOG_DEBUG(satPos);
    }
}

// DEBUG
static void CwndTracer(uint32_t oldval, uint32_t newval) {
    NS_LOG_UNCOND( "CWND at sim time:" << Simulator::Now().GetSeconds() << "s, CWND:" << oldval << " --> " << newval);
}


void TraceCwnd(NodeContainer n, uint32_t nodeId, uint32_t socketId) {

    NS_LOG_UNCOND("CW Window traced Node id: " << nodeId << " socketid: " << socketId);

    Config::ConnectWithoutContext("/NodeList/" + std::to_string(nodeId) + "/$ns3::TcpL4Protocol/SocketList/" + std::to_string(socketId) + "/CongestionWindow", MakeBoundCallback(&CwndTracer));
}

void SendData (Ptr<Socket> &socket) {
    static const uint32_t writeSize = 1040;
    uint8_t data[writeSize];
    for (uint32_t i = 0; i < writeSize; ++i)
    {
        char m = toascii(97 + i % 26);
        data[i] = m;
    }
    socket->Send(&data[0], writeSize, 0);
}

void ConnectSocket (Ptr<Socket> &socket, Ipv4Address destAddr, uint16_t destPort) {
    // Connect source to sink.
    socket->Connect(InetSocketAddress(destAddr, destPort));
}

int main(int argc, char* argv[]) {
    LogComponentEnable("P5-Satellite", LOG_LEVEL_ALL);
    LogComponentEnable("P5-Constellation-Handler", LOG_LEVEL_ALL);
    
    Time::SetResolution(Time::NS);
    
    // ========================================= Setup default commandline parameters  =========================================
    std::string tleDataPath = "scratch/P5-Satellite/resources/starlink_13-11-2024_tle_data.txt";
    std::string tleOrbitsPath = "scratch/P5-Satellite/resources/starlink_13-11-2024_orbits.txt";
    uint32_t satelliteCount = 2;

    CommandLine cmd(__FILE__);
    cmd.AddValue("tledata", "TLE Data path", tleDataPath);
    cmd.AddValue("tleorbits", "TLE Orbits path", tleOrbitsPath);
    cmd.AddValue("satCount", "The amount of satellites", satelliteCount);
    cmd.Parse(argc, argv);
    NS_LOG_INFO("[+] CommandLine arguments parsed succesfully");
    // ==========================================================================================================================
    

    // ========================================= Constellation handling and node Setup =========================================

    // Ground station coordinates:
    std::vector<GeoCoordinate> groundStationsCoordinates;

    // -25.8872, 27.7077, 1540 -- Hartebeesthoek, South Africa
    groundStationsCoordinates.emplace_back(GeoCoordinate(-25.8872, 27.7077, 1540));
    // -32.5931930, 152.1042000, 71 -- Tea Gardens, New South Wales Australia
    groundStationsCoordinates.emplace_back(GeoCoordinate(-32.5931930, 152.1042000, 71));

    // Setup constellation.
    Constellation LEOConstellation(satelliteCount, tleDataPath, tleOrbitsPath, 2, groundStationsCoordinates);



    // ----- REMOVE LATER -----
    // trying to make a new link between the GS's
    Ptr<CsmaChannel> testChannel = CreateObject<CsmaChannel>();
    testChannel->SetAttribute("DataRate", StringValue("1MBps"));
    double delayVal = 3000000.0 / 299792458.0;  // seconds
    testChannel->SetAttribute("Delay", TimeValue(Seconds(delayVal)));
    
    DynamicCast<CsmaNetDevice>(LEOConstellation.groundStationNodes.Get(0)->GetDevice(1))->Attach(testChannel);
    DynamicCast<CsmaNetDevice>(LEOConstellation.groundStationNodes.Get(1)->GetDevice(1))->Attach(testChannel);

    NS_LOG_DEBUG("[E] Check of testChannel ptr value " << testChannel);
    NS_LOG_DEBUG("[E] Check of GS 0 conn to testChannel value " << LEOConstellation.groundStationNodes.Get(0)->GetDevice(1)->GetChannel());
    NS_LOG_DEBUG("[E] Check of GS 1 conn to testChannel value " << LEOConstellation.groundStationNodes.Get(1)->GetDevice(1)->GetChannel());

    // Ptr<CsmaChannel> chan = DynamicCast<CsmaChannel>(groundStations.Get(0)->GetDevice(1)->GetChannel());
    // for (size_t dv = 0; dv < chan->GetNDevices(); ++dv) {
        
    //     Ptr<CsmaNetDevice> currCsmaNetDevice = DynamicCast<CsmaNetDevice>(chan->GetDevice(dv));
        
    //     // Attach the netdevice of the node to a null channel, and delete the channel represents the actual link.
    //     currCsmaNetDevice->Attach(nullChannel);
    // }
    // chan->Dispose();
    
    // NS_LOG_DEBUG("[E] Check of theBannedChanel ptr value " << nullChannel);
    // NS_LOG_DEBUG("[E] Check of GS 0 conn to testChannel value " << groundStations.Get(0)->GetDevice(1)->GetChannel());
    // NS_LOG_DEBUG("[E] Check of GS 1 conn to testChannel value " << groundStations.Get(1)->GetDevice(1)->GetChannel());

    // ----- REMOVE LATER -----

    // Testing purposes
    int lookupIndex = 0;
    Ptr<Node> t = Names::Find<Node>(LEOConstellation.TLEVector[lookupIndex].name);
    NS_LOG_DEBUG(LEOConstellation.TLEVector[lookupIndex].name << " coords " << LEOConstellation.satelliteMobilityModels[lookupIndex]->GetGeoPosition());
    NS_LOG_INFO("[+] SatSGP4 Mobilty installed on " << LEOConstellation.satelliteNodes.GetN() << " satellites");

    // Testing purposes
    NS_LOG_DEBUG("GS-0 coords " << LEOConstellation.groundStationsMobilityModels[0]->GetGeoPosition());
    double gs_sat_dist = LEOConstellation.groundStationsMobilityModels[0]->GetDistanceFrom(LEOConstellation.satelliteMobilityModels[lookupIndex]);
    NS_LOG_DEBUG("Distance between GS 0 and sat 1 is -> " << gs_sat_dist/1000 << " km");



    // ----- Testing of TCP between GSs -----
    
    // // TCP Server receiving data
    // PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 7777));
    // ApplicationContainer serverApps = sink.Install(groundStations.Get(0));
    // serverApps.Start(Seconds(0));
    
    // TCP Client
    NS_LOG_UNCOND("Server IP: " << LEOConstellation.groundStationNodes.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetAddress());
    //OnOffHelper onOffHelper("ns3::TcpSocketFactory", InetSocketAddress(groundStations.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetAddress(), 7777));
    //onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.5]"));
    //onOffHelper.SetAttribute("PacketSize", UintegerValue(512));
    
    //ApplicationContainer clientApps = onOffHelper.Install(groundStations.Get(1));

    // ========================= TCP CWND TRACE TEST ========================
    uint16_t servPort = 7777;

    // Create a sink and install it on the node with index 1.
    PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), servPort));
    
    ApplicationContainer serverApps = sink.Install(LEOConstellation.groundStationNodes.Get(1));
    serverApps.Start(Seconds(0.0));
    serverApps.Stop(Seconds(12.0));

    // Create a source
    Ipv4Address serverAddr = LEOConstellation.groundStationNodes.Get(1)->GetObject<Ipv4>()->GetAddress(1, 0).GetAddress();
    BulkSendHelper source("ns3::TcpSocketFactory", InetSocketAddress(serverAddr, servPort));
    source.SetAttribute("MaxBytes", UintegerValue(4096));
    source.SetAttribute("SendSize", UintegerValue(1024));


    NS_LOG_DEBUG("[E] Created source socket!");
    // Get socket object
    Ptr<Socket> clientSocket = Socket::CreateSocket(LEOConstellation.groundStationNodes.Get(0), TcpSocketFactory::GetTypeId());
    clientSocket->Bind();

    // Simulator::Schedule(Seconds(0.001), ConnectSocket, clientSocket, serverAddr, servPort);
    // Simulator::Schedule(Seconds(1), SendData, clientSocket);
    // Simulator::Schedule(Seconds(6), SendData, clientSocket); // This should fail, however it sends the message after the channel has been connected to the two netdevices again, see wireshark.
    // Simulator::Schedule(Seconds(9), SendData, clientSocket);
    

    //sourceApps.Start(Seconds(0.0));
    //sourceApps.Stop(Seconds(12.0));


    // ========================= TCP CWND TRACE TEST ========================
    // Simulator::Schedule(MilliSeconds(1), &TraceCwnd, groundStations, 1 + satelliteCount, 0);

    // *-*-*-*-* VERY IMPORTANT *-*-*-*-*
    /* The tracer's "NodeList" is the total amount of nodes, because the satellite,
       nodes are setup before the ground stations. Therefore to trace a specific socket,
       we need to add the satellite count ot the index of the node.
    */

    // --------------------------------------


    Simulator::Schedule(Seconds(5), [&LEOConstellation](){
        Ptr<CsmaChannel> chan = DynamicCast<CsmaChannel>(LEOConstellation.groundStationNodes.Get(0)->GetDevice(1)->GetChannel());
        for (size_t device = 0; device < chan->GetNDevices(); ++device) {
            
            Ptr<CsmaNetDevice> currCsmaNetDevice = DynamicCast<CsmaNetDevice>(chan->GetDevice(device));
            
            Ptr<CsmaChannel> nullChannel = CreateObject<CsmaChannel>();
            // Attach the netdevice of the node to a null channel, and delete the channel represents the actual link.
            currCsmaNetDevice->Attach(nullChannel);
            LEOConstellation.groundStationNodes.Get(device)->GetObject<Ipv4>()->SetDown(1);
        }
        chan->Dispose();
        
        NS_LOG_DEBUG("[E] Check of GS 0 conn to testChannel value " << LEOConstellation.groundStationNodes.Get(0)->GetDevice(1)->GetChannel());
        NS_LOG_DEBUG("[E] Check of GS 1 conn to testChannel value " << LEOConstellation.groundStationNodes.Get(1)->GetDevice(1)->GetChannel());

        //Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    });

    Simulator::Schedule(Seconds(7), [&LEOConstellation](){
        Ptr<CsmaChannel> testChannel = CreateObject<CsmaChannel>();
        testChannel->SetAttribute("DataRate", StringValue("1MBps"));
        double delayVal = 3000000.0 / 299792458.0;  // seconds
        testChannel->SetAttribute("Delay", TimeValue(Seconds(delayVal)));

        DynamicCast<CsmaNetDevice>(LEOConstellation.groundStationNodes.Get(0)->GetDevice(1))->Attach(testChannel);
        DynamicCast<CsmaNetDevice>(LEOConstellation.groundStationNodes.Get(1)->GetDevice(1))->Attach(testChannel);

        LEOConstellation.groundStationNodes.Get(0)->GetObject<Ipv4>()->SetUp(1);
        LEOConstellation.groundStationNodes.Get(1)->GetObject<Ipv4>()->SetUp(1);

        // Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    });


    // ========================================= Setup of NetAnimator mobility =========================================
    // Give each ground station a constant position model, and set the location from the satellite mobility model!
    for (uint32_t n = 0; n < LEOConstellation.groundStationNodes.GetN(); n++) {
        GeoCoordinate gsNpos = LEOConstellation.groundStationsMobilityModels[n]->GetGeoPosition();
        AnimationInterface::SetConstantPosition(LEOConstellation.groundStationNodes.Get(n), gsNpos.GetLongitude(), -gsNpos.GetLatitude());
    }

    // Run simulationphase at time 0
    simulationPhase(LEOConstellation.satelliteNodes, LEOConstellation.satelliteMobilityModels, LEOConstellation.groundStationsMobilityModels);
    // Run simulation phase at i intervals
    int interval = 60*0.25;
    for (int i = 1; i < 200; ++i) {
        Time t = Seconds(i * interval);
        Simulator::Schedule(t, simulationPhase, LEOConstellation.satelliteNodes, LEOConstellation.satelliteMobilityModels, LEOConstellation.groundStationsMobilityModels);
    }
    
    // Run NetAnim from the P5-Satellite folder
    AnimationInterface anim("scratch/P5-Satellite/out/p5-satellite.xml");
    // anim.EnablePacketMetadata();
    anim.SetBackgroundImage("scratch/P5-Satellite/resources/earth-map.jpg", -180, -90, 0.17578125, 0.17578125, 1);
    // Pretty Satellites :)
    for (uint32_t n = 0; n < LEOConstellation.satelliteNodes.GetN(); n++){
        anim.UpdateNodeDescription(n, LEOConstellation.TLEVector[n].name.substr(LEOConstellation.TLEVector[n].name.size() - 4,  4));  // Only works for starlink
        anim.UpdateNodeSize(n, 5, 5);
    }
    // Pretty Ground stations
    for (uint32_t n = 0; n < LEOConstellation.groundStationNodes.GetN(); n++){
        anim.UpdateNodeColor(LEOConstellation.groundStationNodes.Get(n), 0, 255, 255);
        anim.UpdateNodeSize(LEOConstellation.groundStationNodes.Get(n), 3, 3);
    }
    anim.UpdateNodeDescription(LEOConstellation.groundStationNodes.Get(0), "Hartebeesthoek");
    anim.UpdateNodeDescription(LEOConstellation.groundStationNodes.Get(1), "Tea Gardens");
    // ==================================================================================================================


    // PointToPointHelper pointToPoint;
    // pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    // pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    // NetDeviceContainer devices;
    // devices = pointToPoint.Install(nodes);

    // InternetStackHelper stack;
    // stack.Install(nodes);

    // Ipv4AddressHelper address;
    // address.SetBase("10.1.1.0", "255.255.255.0");
    // Ipv4InterfaceContainer interfaces = address.Assign(devices);


    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    NS_LOG_UNCOND("");
    NS_LOG_UNCOND("\x1b[31;1m[!]\x1b[37m Simulation is running!\x1b[0m");
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}



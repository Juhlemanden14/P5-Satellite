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
// Correct Net Animator folder path from P5-Satellite: './../../netanim-3.109/NetAnim'
// Call in directory: 
// $ cd ns-3.42


// CongestionWindow Trace methods
static void CwndTracer(uint32_t oldval, uint32_t newval) {
    NS_LOG_UNCOND( "CWND at sim time:" << Simulator::Now().GetSeconds() << "s, CWND:" << oldval << " --> " << newval);
}
void TraceCwnd(NodeContainer n, uint32_t nodeId, uint32_t socketId) {
    NS_LOG_UNCOND("CW Window traced Node id: " << nodeId << " socketid: " << socketId);
    Config::ConnectWithoutContext("/NodeList/" + std::to_string(nodeId) + "/$ns3::TcpL4Protocol/SocketList/" + std::to_string(socketId) + "/CongestionWindow", MakeBoundCallback(&CwndTracer));
}

// Custom socket functions. Delete?
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
    //groundStationsCoordinates.emplace_back(GeoCoordinate(-32.5931930, 152.1042000, 71));
    groundStationsCoordinates.emplace_back(GeoCoordinate(40.5931930, 152.1042000, 71));
    // Temporary
    //groundStationsCoordinates.emplace_back(GeoCoordinate(-10, 100.1042000, 500));

    // Setup constellation.
    Constellation LEOConstellation(satelliteCount, tleDataPath, tleOrbitsPath, groundStationsCoordinates.size(), groundStationsCoordinates, StringValue("20Mbps"), StringValue("20Mbps"));


    // ============================== TESTING ==============================
    
    // Attach SAT-SAT
    // Simulator::Schedule(Seconds(0.5), [&LEOConstellation](){
    //     Ptr<CsmaChannel> SatSatChannel = CreateObject<CsmaChannel>();
    //     SatSatChannel->SetAttribute("DataRate", StringValue("1MBps"));
    //     double delayVal = 3000000.0 / 299792458.0;  // seconds
    //     SatSatChannel->SetAttribute("Delay", TimeValue(Seconds(delayVal)));


    //     Ptr<Node> sat0 = Names::Find<Node>("STARLINK-30159");
    //     Ptr<Node> sat1 = Names::Find<Node>("STARLINK-5748");

    //     // Attach NORTH and NORTH, just an example
    //     DynamicCast<CsmaNetDevice>(sat0->GetDevice(1))->Attach(SatSatChannel);
    //     DynamicCast<CsmaNetDevice>(sat1->GetDevice(1))->Attach(SatSatChannel);

    //     Ptr<Ipv4> satIpv4_0 = sat0->GetObject<Ipv4>();
    //     Ptr<Ipv4> satIpv4_1 = sat1->GetObject<Ipv4>();
        
    //     // Ipv4InterfaceAddress satNewAddr0 = Ipv4InterfaceAddress(Ipv4Address("2.0.0.1"), Ipv4Mask("255.255.255.0"));
    //     // Ipv4InterfaceAddress satNewAddr1 = Ipv4InterfaceAddress(Ipv4Address("2.0.0.2"), Ipv4Mask("255.255.255.0"));
    //     Ipv4AddressHelper tmp("2.0.0.0", "255.255.255.0");
    //     tmp.Assign(NetDeviceContainer(sat0->GetDevice(1), sat1->GetDevice(1)));


    //     // satIpv4_0->AddAddress(1, satNewAddr0);
    //     // satIpv4_1->AddAddress(1, satNewAddr1);
    //     // satIpv4_0->SetUp(1);
    //     // satIpv4_1->SetUp(1);

    //     Ipv4GlobalRoutingHelper::RecomputeRoutingTables();
    // });
    // ===========================================

    
    // ============================== LINK HANDOVER ==============================
    // Simulator::Schedule(Seconds(5), [&LEOConstellation](){
    //     // BREAK LINK =======================
    //     Ptr<Ipv4> satIpv4_0 = LEOConstellation.satelliteNodes.Get(0)->GetObject<Ipv4>();
    //     Ptr<Ipv4> gsIpv4_0 = LEOConstellation.groundStationNodes.Get(0)->GetObject<Ipv4>();
    //     // SAT0
    //     satIpv4_0->RemoveAddress(5, 0);
    //     satIpv4_0->SetDown(5);
    //     Ptr<CsmaNetDevice> satCsmaNetDevice0 = DynamicCast<CsmaNetDevice>(satIpv4_0->GetNetDevice(5));
    //     Ptr<CsmaChannel> nullChannel = CreateObject<CsmaChannel>();
    //     satCsmaNetDevice0->Attach(nullChannel);
    //     // GROUND0
    //     gsIpv4_0->SetDown(1);
    //     Ptr<CsmaNetDevice> gsCsmaNetDevice0 = DynamicCast<CsmaNetDevice>(gsIpv4_0->GetNetDevice(1));
    //     Ptr<CsmaChannel> nullChannel2 = CreateObject<CsmaChannel>();
    //     gsCsmaNetDevice0->Attach(nullChannel2);
    //     gsIpv4_0->GetNetDevice(1)->GetChannel()->Dispose();
        
    //     Ipv4GlobalRoutingHelper::RecomputeRoutingTables();
       
    //     // ESTABLISH NEW LINK ==========================
    //     // Ptr<Ipv4> gsIpv4_0 = groundStations.Get(0)->GetObject<Ipv4>();
    //     Ptr<Ipv4> satIpv4_1 = LEOConstellation.satelliteNodes.Get(1)->GetObject<Ipv4>();

    //     Ptr<CsmaChannel> SatGroundChannel = CreateObject<CsmaChannel>();
    //     SatGroundChannel->SetAttribute("DataRate", StringValue("1MBps"));
    //     double delayVal = 3000000.0 / 299792458.0;  // seconds
    //     SatGroundChannel->SetAttribute("Delay", TimeValue(Seconds(delayVal)));
        
    //     // Attach SAT-GROUND nodes together, groundstation has 1 netdevice, satellite's fifth is always to the groundstation
    //     DynamicCast<CsmaNetDevice>(LEOConstellation.groundStationNodes.Get(0)->GetDevice(1))->Attach(SatGroundChannel);

    //     // GetDevice(2) should be 5 as the satellite only has one gs link, however for testing purposes device 2 is used.
    //     DynamicCast<CsmaNetDevice>(LEOConstellation.satelliteNodes.Get(1)->GetDevice(2))->Attach(SatGroundChannel);

    //     Ipv4InterfaceAddress satNewAddr = Ipv4InterfaceAddress(Ipv4Address("1.0.0.2"), Ipv4Mask("255.255.255.0"));
    //     satIpv4_1->AddAddress(2, satNewAddr);
    //     satIpv4_1->SetUp(2);
    //     gsIpv4_0->SetUp(1);

    //     Ipv4GlobalRoutingHelper::RecomputeRoutingTables();
    // });

    // Simulator::Schedule(Seconds(8), [&LEOConstellation](){
    //     // DEBUGGING For each node, print the MAC addresses of all its NetDevices (also the loopback)
    //     for (uint32_t i = 0; i < LEOConstellation.satelliteNodes.GetN(); i++){
    //         NS_LOG_UNCOND("Sat " << i);
    //         Ptr<Ipv4> ip = LEOConstellation.satelliteNodes.Get(i)->GetObject<Ipv4>();
    //         for (uint32_t j = 0; j < ip->GetNInterfaces(); j++){
    //             NS_LOG_UNCOND("     Interface " << j << " with adresses " << ip->GetNAddresses(j));
    //             for (uint32_t k = 0; k < ip->GetNAddresses(j); k++)
    //                 NS_LOG_UNCOND("     Node[" << i << "] Interface[" << j << "] - IP: " << ip->GetAddress(j, k).GetAddress());
    //         }
    //     }
    //     for (uint32_t i = 0; i < LEOConstellation.groundStationNodes.GetN(); i++){
    //         NS_LOG_UNCOND("GS " << i);
    //         Ptr<Ipv4> ip = LEOConstellation.groundStationNodes.Get(i)->GetObject<Ipv4>();
    //         for (uint32_t j = 0; j < ip->GetNInterfaces(); j++){
    //             NS_LOG_UNCOND("     Interface " << j << " with adresses " << ip->GetNAddresses(j));
    //             for (uint32_t k = 0; k < ip->GetNAddresses(j); k++)
    //                 NS_LOG_UNCOND("     Node[" << i << "] Interface[" << j << "] - IP: " << ip->GetAddress(j, k).GetAddress());
    //         }
    //     }
    // });


    // ========================= TCP CWND TRACE TEST ========================
    // Simulator::Schedule(MilliSeconds(1), &TraceCwnd, groundStations, 1 + satelliteCount, 0);

    // *-*-*-*-* VERY IMPORTANT *-*-*-*-*
    /* The tracer's "NodeList" is the total amount of nodes, because the satellite,
       nodes are setup before the ground stations. Therefore to trace a specific socket,
       we need to add the satellite count ot the index of the node.
    */
    // --------------------------------------




    // ============================== APPLICATIONS ==============================
    UdpClientHelper udp(LEOConstellation.groundStationNodes.Get(1)->GetObject<Ipv4>()->GetAddress(1, 0).GetAddress(), 7777); // Add remote addr and port
    udp.SetAttribute("Interval", StringValue("15s"));
    ApplicationContainer app = udp.Install(LEOConstellation.groundStationNodes.Get(0));
    app.Start(Seconds(1.0));
    app.Stop(Seconds(60*10));

    
    Ptr<OutputStreamWrapper> routingStream =  Create<OutputStreamWrapper>("scratch/P5-Satellite/out/sat.routes", std::ios::out);
    Ipv4RoutingHelper::PrintRoutingTableAllAt(Seconds(2), routingStream);


    // Run simulationphase for x minutes with y second intervals. Includes an initial update at time 0.
    LEOConstellation.simulationLoop(10, 15);


    // ========================================= Setup of NetAnimator mobility =========================================
    // Give each ground station a constant position model, and set the location from the satellite mobility model!
    for (uint32_t n = 0; n < LEOConstellation.groundStationNodes.GetN(); n++) {
        GeoCoordinate gsNpos = LEOConstellation.groundStationsMobilityModels[n]->GetGeoPosition();
        AnimationInterface::SetConstantPosition(LEOConstellation.groundStationNodes.Get(n), gsNpos.GetLongitude(), -gsNpos.GetLatitude());
    }
    
    // Run NetAnim from the P5-Satellite folder
    AnimationInterface anim("scratch/P5-Satellite/out/p5-satellite.xml");
    // anim.EnablePacketMetadata();
    anim.SetBackgroundImage("scratch/P5-Satellite/resources/earth-map.jpg", -180, -90, 0.17578125, 0.17578125, 1);
    // Pretty Satellites :)
    for (uint32_t n = 0; n < LEOConstellation.satelliteNodes.GetN(); n++){
        anim.UpdateNodeDescription(n, LEOConstellation.TLEVector[n].name);
        anim.UpdateNodeSize(n, 3, 3);
    }
    // Pretty Ground stations
    for (uint32_t n = 0; n < LEOConstellation.groundStationNodes.GetN(); n++){
        anim.UpdateNodeColor(LEOConstellation.groundStationNodes.Get(n), 0, 255, 255);
        anim.UpdateNodeSize(LEOConstellation.groundStationNodes.Get(n), 4, 4);
    }
    anim.UpdateNodeDescription(LEOConstellation.groundStationNodes.Get(0), "Hartebeesthoek");
    anim.UpdateNodeDescription(LEOConstellation.groundStationNodes.Get(1), "Tea Gardens");
    // ==================================================================================================================


    NS_LOG_UNCOND("");
    NS_LOG_UNCOND("\x1b[31;1m[!]\x1b[37m Simulation is running!\x1b[0m");
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}



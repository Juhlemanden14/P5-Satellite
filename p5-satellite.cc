#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
// #include "ns3/satellite-module.h"
// #include "ns3/csma-module.h" // Important when DynamicCasting
#include "ns3/socket.h"

// P5 Self-written files
#include "constellationHandler.h"
#include "tleHandler.h"
#include "traceHandler.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("P5-Satellite");

int main(int argc, char* argv[]) {
    // This option will make packets be randomly routed between two equally costed paths
    // instead of always picking the same one. Is good for modelling load balancing!
    // Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue(true));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448)); // Packet size for the TCP socket

    LogComponentEnable("P5-Satellite", LOG_LEVEL_ALL);
    LogComponentEnable("P5-Constellation-Handler", LOG_LEVEL_INFO);
    LogComponentEnable("P5TraceHandler", LOG_LEVEL_DEBUG);

    Time::SetResolution(Time::NS);

    // ========================================= Setup default commandline parameters  =========================================
    std::string tleDataPath = "scratch/P5-Satellite/resources/starlink_13-11-2024_tle_data.txt";
    std::string tleOrbitsPath = "scratch/P5-Satellite/resources/starlink_13-11-2024_orbits.txt";
    uint32_t satelliteCount = 0;
    int simTime = 10;
    int updateInterval = 15;

    double bitErrorRate = 10e-7;
    std::string satSatDataRate("100Mbps");
    std::string gsSatDataRate("100Mbps");
    std::string linkAcqTime("2s");
    std::string congestionCA = "TcpBbr";

    CommandLine cmd(__FILE__);
    cmd.AddValue("tledata", "TLE Data path", tleDataPath);
    cmd.AddValue("tleorbits", "TLE Orbits path", tleOrbitsPath);
    cmd.AddValue("satCount", "The amount of satellites", satelliteCount);
    cmd.AddValue("simTime", "Time in minutes the simulation will run for", simTime);
    cmd.AddValue("updateInterval", "Time in seconds between intervals in the simulation", updateInterval);
    cmd.AddValue("CCA",
                 "Congestion Control Algorithm: TcpNewReno, TcpLinuxReno, "
                 "TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, TcpScalable, TcpVeno, "
                 "TcpBic, TcpYeah, TcpIllinois, TcpWestwoodPlus, TcpLedbat, "
                 "TcpLp, TcpDctcp, TcpCubic, TcpBbr",
                 congestionCA);

    cmd.AddValue("BER", "Bit Error Rate", bitErrorRate);
    cmd.AddValue("satSatDataRate", "DataRate from SAT-SAT", satSatDataRate);
    cmd.AddValue("gsSatDataRate", "DataRate from GS-SAT", gsSatDataRate);
    cmd.AddValue("linkAcqTime", "Link acquisition time", linkAcqTime);
    cmd.Parse(argc, argv);
    NS_LOG_INFO("[+] CommandLine arguments parsed succesfully");

    congestionCA = std::string("ns3::") + congestionCA;
    // ========================================================================

    // ============ Constellation handling and node Setup ============
    // Ground station coordinates:
    std::vector<GeoCoordinate> groundStationsCoordinates;

    // Hartebeesthoek, South Africa
    // groundStationsCoordinates.emplace_back(GeoCoordinate(-25.8872, 27.7077, 1540));

    // Atlantic Ocean (left mid)
    // groundStationsCoordinates.emplace_back(GeoCoordinate(8.965719363937712, -31.654778506938765, 50));

    // Tea Gardens, New South Wales Australia
    // groundStationsCoordinates.emplace_back(GeoCoordinate(-32.5931930, 152.1042000, 71));

    // Ocean (right top)
    // groundStationsCoordinates.emplace_back(GeoCoordinate(40.5931930, 152.1042000, 71));
    // =============== World trade centers=================================================

    // // World trade center, New York
    groundStationsCoordinates.emplace_back(GeoCoordinate(40.711394051407254, -74.01147005959824, 20));

    // // Trade Center, Los Angeles
    // groundStationsCoordinates.emplace_back(GeoCoordinate(33.768177717872234, -118.19913975532677, 20));

    // // Trade center, Tokyo
    // groundStationsCoordinates.emplace_back(GeoCoordinate(35.654709301980525, 139.75638631219815, 20));

    // // Trade center Europe, Belgium
    // groundStationsCoordinates.emplace_back(GeoCoordinate(51.213129420062295, 4.4227015398076786, 20));

    // // Trade center, Dubai
    groundStationsCoordinates.emplace_back(GeoCoordinate(25.217273781972715, 55.28287038973016, 20));

    // // Trade center, Sao Paulo
    // groundStationsCoordinates.emplace_back(GeoCoordinate(-23.6089096493764, -46.697137303281885, 20));


    // ======================== Setup constellation ========================
    Constellation LEOConstellation(satelliteCount, 
                                   tleDataPath, 
                                   tleOrbitsPath, 
                                   groundStationsCoordinates.size(), 
                                   groundStationsCoordinates,
                                   DataRate(gsSatDataRate),
                                   DataRate(satSatDataRate),
                                   bitErrorRate,
                                   bitErrorRate,
                                   Time(linkAcqTime));

    // Run simulationphase for x minutes with y second intervals. Includes an initial update at time 0.
    LEOConstellation.scheduleSimulation(simTime, updateInterval);


    // ============================== APPLICATIONS ==============================
    // Inspiration from --> examples/tcp/tcp-variants-comparison.cc
    Ptr<Node> gsNode0 = LEOConstellation.groundStationNodes.Get(0);
    Ptr<Node> gsNode1 = LEOConstellation.groundStationNodes.Get(1);

    // Specifying the Congestion Control Algorithm is done by finding its TypeID
    TypeId tcpTid = TypeId::LookupByName(congestionCA);
    // Version 1 - Setting for every node
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue( tcpTid ));
    // Version 2 - Setting for individual nodes
    gsNode0->GetObject<TcpL4Protocol>()->SetAttribute("SocketType", TypeIdValue(tcpTid));
    gsNode1->GetObject<TcpL4Protocol>()->SetAttribute("SocketType", TypeIdValue(tcpTid));

    uint16_t port = 7777;
    // Create a packetSink application receiving the traffic in GS 1
    Address sinkLocalAddr(InetSocketAddress(Ipv4Address::GetAny(), port));
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", sinkLocalAddr);
    sinkHelper.SetAttribute("EnableSeqTsSizeHeader", BooleanValue(true)); // Enable packet tracking, good for testing
    // We can now enable tracing of each packet using the trace source "RxWithSeqTsSize"
    ApplicationContainer appSink = sinkHelper.Install(gsNode1);
    // appSink.Get(0)->TraceConnectWithoutContext("RxWithSeqTsSize", MakeCallback(&ReceiveWithSeqTsSize));

    // Create a OnOff application on GS 0, streaming to GS 1
    Ipv4Address targetIP = gsNode1->GetObject<Ipv4>()->GetAddress(1, 0).GetAddress();

    // Voice call scenario for checking RTT
    OnOffHelper onoffHelper("ns3::TcpSocketFactory", InetSocketAddress(targetIP, port));
    onoffHelper.SetAttribute("EnableSeqTsSizeHeader", BooleanValue(true));
    onoffHelper.SetAttribute("DataRate", StringValue("1Mbps"));
    onoffHelper.SetAttribute("PacketSize", UintegerValue(1448)); // Value of the actual data size of the packet size for the application
    ApplicationContainer appSource = onoffHelper.Install(gsNode0);

    // File download scenario for checking CWND
    // BulkSendHelper bulkSendHelper ("ns3::TcpSocketFactory", InetSocketAddress(targetIP, port));
    // bulkSendHelper.SetAttribute("MaxBytes", UintegerValue(0));
    // bulkSendHelper.SetAttribute("EnableSeqTsSizeHeader", BooleanValue(true));
    // bulkSendHelper.SetAttribute("SendSize", UintegerValue(1448)); // Value of the actual data size of the packet size for the application
    // ApplicationContainer appSource = bulkSendHelper.Install(gsNode0);

    appSource.Start(Seconds(0));
    appSource.Stop(Seconds(simTime * 60));


    // ========================= TCP CWND TRACE TEST ========================
    // Each ground stations gets their traced set up!
    Simulator::Schedule(MilliSeconds(1), &SetupTracing, gsNode0);
    Simulator::Schedule(MilliSeconds(1), &SetupTracing, gsNode1);
    // ======================================================================


    // @Marcus TODO: Use this somehow for the graphs
    // Simulator::Schedule(Time("3s"), [gsNode0, gsNode1]() {
    //     printCompleteRoute(gsNode0, gsNode1);
    // });
   


    // ========================================= Setup of NetAnimator mobility =========================================
    // Correct Net Animator folder path from P5-Satellite: './../../netanim-3.109/NetAnim'
    // Call in directory: 
    // $ cd ns-3.42

    // Give each ground station a constant position model, and set the location from the satellite mobility model!
    for (uint32_t n = 0; n < LEOConstellation.groundStationNodes.GetN(); n++) {
        GeoCoordinate gsNpos = LEOConstellation.groundStationsMobilityModels[n]->GetGeoPosition();
        AnimationInterface::SetConstantPosition(LEOConstellation.groundStationNodes.Get(n),
                                                gsNpos.GetLongitude(),
                                                -gsNpos.GetLatitude());
    }
    // Run NetAnim from the ns3-find (ns3 root)
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
    anim.UpdateNodeDescription(LEOConstellation.groundStationNodes.Get(0), "World trade center");
    anim.UpdateNodeDescription(LEOConstellation.groundStationNodes.Get(1), "Trade center");
    // ==================================================================================================================



    // ============== Routing Tables Output ==============
    // IS VERY BUGGY, might hang the application, or generate gigabytes of files on your computer or something
    // Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>("scratch/P5-Satellite/out/sat.routes", std::ios::out);
    // Ipv4RoutingHelper::PrintRoutingTableAllAt(Seconds(20), routingStream);
    // Ipv4RoutingHelper::PrintRoutingTableEvery(Seconds(20), gsNode0, routingStream);
    // Ipv4RoutingHelper::PrintRoutingTableEvery(Seconds(14), gsNode0, routingStream);


    NS_LOG_UNCOND("");
    NS_LOG_UNCOND("\x1b[31;1m[!]\x1b[37m Simulation is running!\x1b[0m");
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}

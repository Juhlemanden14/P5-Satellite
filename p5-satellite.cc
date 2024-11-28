#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
// #include "ns3/satellite-module.h"
#include "ns3/csma-module.h"


#include "tleHandler.h"
#include "constellationHandler.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("P5-Satellite");
// Correct Net Animator folder path from P5-Satellite: './../../netanim-3.109/NetAnim'
// Call in directory: 
// $ cd ns-3.42

// ============= CongestionWindow Trace methods =============
static void CwndTraceSink(Ptr<OutputStreamWrapper> logStream, uint32_t nodeID, size_t socketIndex, uint32_t oldval, uint32_t newval) {
    // uint32_t roundedTime = std::round(Simulator::Now().GetSeconds()*10)/10;
    // NS_LOG_DEBUG("[" << roundedTime << "s] Node:" << nodeID << " Socket:" << socketIndex << " | CWND:" << oldval << " --> " << newval);
    *logStream->GetStream() << Simulator::Now().GetSeconds() << "," << newval << std::endl;
}

// A supreme method for enabling varius Tracings, logging everything into log files!
void SetupTracing(Ptr<Node> node) {

    // Get the list of sockets on the specified node
    ObjectMapValue socketList;
    node->GetObject<TcpL4Protocol>()->GetAttribute("SocketList", socketList);

    NS_LOG_DEBUG("[!] SetupTracing() for Node " << node->GetId() << " has " << socketList.GetN() << " socket(s)");
    // For each socket on the node, enable whatever tracing is relevant!
    for (size_t socketIndex = 0; socketIndex < socketList.GetN(); socketIndex++) {

        Ptr<TcpSocketBase> socketBase = DynamicCast<TcpSocketBase>(socketList.Get(socketIndex));

        // --- CONGESTION WINDOW ---
        std::string logName = "scratch/P5-Satellite/out/CongestionWindow_Node" + std::to_string(node->GetId()) + "_Socket" + std::to_string(socketIndex) + ".txt";
        // C++ can not make use of an ofstream through a bound callback, as noted by the CreateFileStream() method. Therefore we make use of the ns3 AsciiTraceHelper to pass around a stream which the callback methods can write to!
        AsciiTraceHelper asciiTraceHelper;
        Ptr<OutputStreamWrapper> logStream = asciiTraceHelper.CreateFileStream(logName);
        // Row 1 should be the names of the columns
        *logStream->GetStream() << "time(s),CongestionWindow" << std::endl;
        // At time 0 the CWND=0, which is missing unless we just add it here
        *logStream->GetStream() << "0,0" << std::endl;

        
        // Trace a source to the congestion window, but do so with a Make*Bound*Callback!
        // This allows us to pass in additional method parameters beyond the one that CongestionWindow supports!
        // Genius for logging!
        socketBase->TraceConnectWithoutContext("CongestionWindow", MakeBoundCallback(&CwndTraceSink, logStream, node->GetId(), socketIndex));
        // TODO, PLOT THIS ALSO
        // socketBase->TraceConnectWithoutContext("RTT")

        // ======== ADDITIONAL SHIT IF NEEDED LATER ========
        // Setting socket MinRTO
        // socketBase->SetMinRto(Time(Seconds(2)));

        // Setting socket Congestion control algorithm AND Recovery type?!
        // socketBase->SetCongestionControlAlgorithm();
        // socketBase->SetRecoveryAlgorithm();
    }
}

// Test function for utilizing SeqHeaders (see where it is called)
void ReceiveWithSeqTsSize(Ptr<const Packet> pkt, const Address& from, const Address& dst, const SeqTsSizeHeader& header) {
    NS_LOG_DEBUG("Sink received SetTsSize pkt -> " << header);
}



int main(int argc, char* argv[]) {
    // This option will make packets be randomly routed between two equally costed paths
    // instead of always picking the same one. Is good for modelling load balancing!
    // Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue(true));
    
    LogComponentEnable("P5-Satellite", LOG_LEVEL_ALL);
    LogComponentEnable("P5-Constellation-Handler", LOG_LEVEL_INFO);
    
    Time::SetResolution(Time::NS);

    // ========================================= Setup default commandline parameters  =========================================
    std::string tleDataPath = "scratch/P5-Satellite/resources/starlink_13-11-2024_tle_data.txt";
    std::string tleOrbitsPath = "scratch/P5-Satellite/resources/starlink_13-11-2024_orbits.txt";
    uint32_t satelliteCount = 2;
    int simTime = 10;
    int updateInterval = 15;
    
    double bitErrorRate = 10e-9;
    std::string satSatDataRate("20Mbps");
    std::string gsSatDataRate("20Mbps");
    std::string linkAcqTime("2s");
    std::string congestionCA = "TcpNewReno";
    
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
    groundStationsCoordinates.emplace_back(GeoCoordinate(8.965719363937712, -31.654778506938765, 50));

    // Tea Gardens, New South Wales Australia
    // groundStationsCoordinates.emplace_back(GeoCoordinate(-32.5931930, 152.1042000, 71));

    // Ocean (right top)
    groundStationsCoordinates.emplace_back(GeoCoordinate(40.5931930, 152.1042000, 71));
    // =============== World trade centers=================================================

    // // World trade center, New York
    // groundStationsCoordinates.emplace_back(GeoCoordinate(40.711394051407254, -74.01147005959824, 20));

    // // Trade Center, Los Angeles
    // groundStationsCoordinates.emplace_back(GeoCoordinate(33.768177717872234, -118.19913975532677, 20));

    // // Trade center, Tokyo
    // groundStationsCoordinates.emplace_back(GeoCoordinate(35.654709301980525, 139.75638631219815, 20));

    // // Trade center Europe, Belgium
    // groundStationsCoordinates.emplace_back(GeoCoordinate(51.213129420062295, 4.4227015398076786, 20));

    // // Trade center, Dubai
    // groundStationsCoordinates.emplace_back(GeoCoordinate(25.217273781972715, 55.28287038973016, 20));

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
                                   Time(linkAcqTime)
                                   );

    // Run simulationphase for x minutes with y second intervals. Includes an initial update at time 0.
    LEOConstellation.simulationLoop(simTime, updateInterval);


    // ============================== APPLICATIONS ==============================
    // Inspiration from --> examples/tcp/tcp-variants-comparison.cc
    Ptr<Node> gsNode0 = LEOConstellation.groundStationNodes.Get(0);
    Ptr<Node> gsNode1 = LEOConstellation.groundStationNodes.Get(1);

    // How to specify a congestion control, first find its TypeID
    
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
    OnOffHelper onoffHelper("ns3::TcpSocketFactory", InetSocketAddress(targetIP, port));
    onoffHelper.SetAttribute("EnableSeqTsSizeHeader", BooleanValue(true));
    onoffHelper.SetAttribute("DataRate", StringValue("1Mbps"));
    ApplicationContainer appSource = onoffHelper.Install(gsNode0);
    appSource.Start(Seconds(0));
    appSource.Stop(Seconds(60*2));


    // ========================= TCP CWND TRACE TEST ========================
    // Each ground stations gets their traced set up!
    Simulator::Schedule(MilliSeconds(1), &SetupTracing, gsNode0);
    Simulator::Schedule(MilliSeconds(1), &SetupTracing, gsNode1);
    // ======================================================================
    
   


    // ========================================= Setup of NetAnimator mobility =========================================
    // Give each ground station a constant position model, and set the location from the satellite mobility model!
    for (uint32_t n = 0; n < LEOConstellation.groundStationNodes.GetN(); n++) {
        GeoCoordinate gsNpos = LEOConstellation.groundStationsMobilityModels[n]->GetGeoPosition();
        AnimationInterface::SetConstantPosition(LEOConstellation.groundStationNodes.Get(n), gsNpos.GetLongitude(), -gsNpos.GetLatitude());
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
    anim.UpdateNodeDescription(LEOConstellation.groundStationNodes.Get(0), "Hartebeesthoek");
    anim.UpdateNodeDescription(LEOConstellation.groundStationNodes.Get(1), "Tea Gardens");
    // ==================================================================================================================



    // ============== Routing Tables Output ==============
    Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>("scratch/P5-Satellite/out/sat.routes", std::ios::out);
    // Ipv4RoutingHelper::PrintRoutingTableAllAt(Seconds(20), routingStream);
    // Ipv4RoutingHelper::PrintRoutingTableEvery(Seconds(20), gsNode0, routingStream);
    // Ipv4RoutingHelper::PrintRoutingTableEvery(Seconds(14), gsNode0, routingStream);


    NS_LOG_UNCOND("");
    NS_LOG_UNCOND("\x1b[31;1m[!]\x1b[37m Simulation is running!\x1b[0m");
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}



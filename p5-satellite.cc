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



// CongestionWindow Trace methods
static void CwndTraceSink(uint32_t nodeID, size_t socketIndex, uint32_t oldval, uint32_t newval) {
    uint32_t roundedTime = std::round(Simulator::Now().GetSeconds()*1000)/1000;
    NS_LOG_DEBUG("[" << roundedTime << "s] Node:" << nodeID << " Socket:" << socketIndex << " | CWND:" << oldval << " --> " << newval);
}
// void DoTraceCwnd(uint32_t nodeID, uint32_t socketId) {
//     NS_LOG_DEBUG("CW Window traced Node id: " << nodeID << " socketid: " << socketId);
//     Config::ConnectWithoutContext("/NodeList/" + std::to_string(nodeID) + "/$ns3::TcpL4Protocol/SocketList/" + std::to_string(socketId) + "/CongestionWindow", MakeCallback(&CwndTraceSink)); // Or MakeBoundCallback, dont know the difference and cant see one
// }

// A supreme Trace logger, logging everything needed for a specified node!
void DoTraceCwndSmart(Ptr<Node> node) {
    // Get the list of sockets on the specified node
    ObjectMapValue socketList;
    node->GetObject<TcpL4Protocol>()->GetAttribute("SocketList", socketList);

    NS_LOG_DEBUG("Node " << node->GetId() << " has " << socketList.GetN() << " sockets");
    // For each socket on the node, enable whatever tracing is relevant!
    for (size_t socketIndex = 0; socketIndex < socketList.GetN(); socketIndex++) {
        Ptr<TcpSocketBase> socketBase = DynamicCast<TcpSocketBase>(socketList.Get(socketIndex));
        
        // Trace a source to the congestion window, but do so with a Make*Bound*Callback!
        // This allows us to pass in additional method parameters beyond the one that CongestionWindow supports!
        // Genius for logging!
        socketBase->TraceConnectWithoutContext("CongestionWindow", MakeBoundCallback(&CwndTraceSink, node->GetId(), socketIndex));

        // ======== ADDITIONAL SHIT IF NEEDED LATER ========
        // Setting socket MinRTO
        // socketBase->SetMinRto(Time(Seconds(2)));

        // Setting socket Congestion control algorithm AND Recovery type?!
        // socketBase->SetCongestionControlAlgorithm();
        // socketBase->SetRecoveryAlgorithm();

    }
}


void ReceiveWithSeqTsSize(Ptr<const Packet> pkt, const Address& from, const Address& dst, const SeqTsSizeHeader& header) {
    NS_LOG_DEBUG("Sink received SetTsSize pkt -> " << header);
}




int main(int argc, char* argv[]) {
    // This option will make packets be randomly routed between two equally costed paths
    // instead of always picking the same one. Is good for modelling load balancing!
    // Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue(true));
    
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
    // ================================================================



    // ======================== Setup constellation ========================
    Constellation LEOConstellation(satelliteCount, tleDataPath, tleOrbitsPath, groundStationsCoordinates.size(), groundStationsCoordinates, StringValue("20Mbps"), StringValue("20Mbps"));

    // Run simulationphase for x minutes with y second intervals. Includes an initial update at time 0.
    LEOConstellation.simulationLoop(0, 15);





    // ============================== APPLICATIONS ==============================
    // Inspiration from --> examples/tcp/tcp-variants-comparison.cc
    Ptr<Node> gsNode0 = LEOConstellation.groundStationNodes.Get(0);
    Ptr<Node> gsNode1 = LEOConstellation.groundStationNodes.Get(1);

    // How to specify a congestion control, first find its TypeID
    TypeId tcpTid = TypeId::LookupByName("ns3::TcpBbr");
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
    ApplicationContainer appSource = onoffHelper.Install(gsNode0);
    appSource.Start(Seconds(0));
    appSource.Stop(Seconds(3));


     // ========================= TCP CWND TRACE TEST ========================
    // Simulator::Schedule(MilliSeconds(1), &DoTraceCwnd, gsNode0->GetId(), 0);
    Simulator::Schedule(MilliSeconds(1), &DoTraceCwndSmart, gsNode0);
    Simulator::Schedule(MilliSeconds(1), &DoTraceCwndSmart, gsNode1);
    /* *-*-*-*-* VERY IMPORTANT *-*-*-*-*
        The tracer's "NodeList" is the total amount of nodes. As the satellites are created before the ground stations, the first 2 ground stations would have ID --> satCount+1 and satCount+2 in the NodeList!
    */

    

   


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
    Ptr<OutputStreamWrapper> routingStream =  Create<OutputStreamWrapper>("scratch/P5-Satellite/out/sat.routes", std::ios::out);
    Ipv4RoutingHelper::PrintRoutingTableAllAt(Seconds(2), routingStream);



    NS_LOG_UNCOND("");
    NS_LOG_UNCOND("\x1b[31;1m[!]\x1b[37m Simulation is running!\x1b[0m");
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}



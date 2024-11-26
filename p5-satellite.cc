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



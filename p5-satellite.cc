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
    NS_LOG_DEBUG("[->] Simulation at second " << Simulator::Now().GetSeconds());    // Gets the elapsed seconds in the simulation

    // Set the new positions of the satellites and update their position in Net Animator.
    for (uint32_t n = 0; n < satellites.GetN(); ++n) {
        GeoCoordinate satPos = satelliteMobilityModels[n]->GetGeoPosition();
        AnimationInterface::SetConstantPosition(satellites.Get(n), satPos.GetLongitude(), -satPos.GetLatitude());

        //NS_LOG_DEBUG(satPos);
    }
}


int main(int argc, char* argv[]) {
    LogComponentEnable("P5-Satellite", LOG_LEVEL_ALL);
    Time::SetResolution(Time::NS);
    
    // ========================================= Setup default commandline parameters  =========================================
    std::string tleDataPath = "scratch/P5-Satellite/TLE-handling/starlink_13-11-2024_tle_data.txt";
    int satelliteCount = 2;

    CommandLine cmd(__FILE__);
    cmd.AddValue("tledata", "TLE Data path", tleDataPath);
    cmd.AddValue("satCount", "The amount of satellites", satelliteCount);
    cmd.Parse(argc, argv);
    NS_LOG_INFO("[+] CommandLine arguments parsed succesfully");
    // ==========================================================================================================================
    

    // ========================================= TLE handling and node Setup =========================================
    std::vector<TLE> TLEVector;
    Ptr<CsmaChannel> theBannedChannel = CreateObject<CsmaChannel>();   // a global channel used for netdevices that are not in use. They point to this unusable channel istead of having a dangling pointer

    std::vector<Ptr<SatSGP4MobilityModel>> satelliteMobilityModels;
    NodeContainer satellites = createSatellitesFromTLE(satelliteCount, satelliteMobilityModels, tleDataPath, TLEVector);

    std::vector<Ptr<SatConstantPositionMobilityModel>> groundStationsMobilityModels;
    std::vector<GeoCoordinate> groundStationsCoordinates;


    // -25.8872, 27.7077, 1540 -- Hartebeesthoek, South Africa
    groundStationsCoordinates.emplace_back(GeoCoordinate(-25.8872, 27.7077, 1540));

    // -32.5931930, 152.1042000, 71 -- Tea Gardens, New South Wales Australia
    groundStationsCoordinates.emplace_back(GeoCoordinate(-32.5931930, 152.1042000, 71));

    NodeContainer groundStations = createGroundStations(2, groundStationsMobilityModels, groundStationsCoordinates, theBannedChannel);
    NS_LOG_DEBUG("[E] MAC: " << groundStations.Get(1)->GetDevice(1)->GetAddress());

    // ----- REMOVE LATER -----
    // trying to make a new link between the GS's
    Ptr<CsmaChannel> testChannel = CreateObject<CsmaChannel>();
    testChannel->SetAttribute("DataRate", StringValue("1MBps"));
    double delayVal = 3000000.0 / 299792458.0;  // seconds
    testChannel->SetAttribute("Delay", TimeValue(Seconds(delayVal)));
    
    DynamicCast<CsmaNetDevice>(groundStations.Get(0)->GetDevice(1))->Attach(testChannel);
    DynamicCast<CsmaNetDevice>(groundStations.Get(1)->GetDevice(1))->Attach(testChannel);


    NS_LOG_DEBUG("[E] Check of testChannel ptr value " << testChannel);
    NS_LOG_DEBUG("[E] Check of GS 0 conn to testChannel value " << groundStations.Get(0)->GetDevice(1)->GetChannel());
    NS_LOG_DEBUG("[E] Check of GS 1 conn to testChannel value " << groundStations.Get(1)->GetDevice(1)->GetChannel());


    // Ptr<CsmaChannel> chan = DynamicCast<CsmaChannel>(groundStations.Get(0)->GetDevice(1)->GetChannel());
    // for (size_t dv = 0; dv < chan->GetNDevices(); ++dv) {
        
    //     Ptr<CsmaNetDevice> currCsmaNetDevice = DynamicCast<CsmaNetDevice>(chan->GetDevice(dv));
        
    //     // Attach the netdevice of the node to a null channel, and delete the channel represents the actual link.
    //     currCsmaNetDevice->Attach(theBannedChannel);
    // }
    // chan->Dispose();
    
    // NS_LOG_DEBUG("[E] Check of theBannedChanel ptr value " << theBannedChannel);
    // NS_LOG_DEBUG("[E] Check of GS 0 conn to testChannel value " << groundStations.Get(0)->GetDevice(1)->GetChannel());
    // NS_LOG_DEBUG("[E] Check of GS 1 conn to testChannel value " << groundStations.Get(1)->GetDevice(1)->GetChannel());

    // ----- REMOVE LATER -----

    // Testing purposes
    int lookupIndex = 0;
    Ptr<Node> t = Names::Find<Node>(TLEVector[lookupIndex].name);
    NS_LOG_DEBUG(TLEVector[lookupIndex].name << " coords " << satelliteMobilityModels[lookupIndex]->GetGeoPosition());
    NS_LOG_INFO("[+] SatSGP4 Mobilty installed on " << satellites.GetN() << " satellites");

    // Testing purposes
    NS_LOG_DEBUG("GS-0 coords " << groundStationsMobilityModels[0]->GetGeoPosition());
    double gs_sat_dist = groundStationsMobilityModels[0]->GetDistanceFrom(satelliteMobilityModels[lookupIndex]);
    NS_LOG_DEBUG("Distance between GS 0 and sat 1 is -> " << gs_sat_dist/1000 << " km");



    // ----- Testing of TCP between GSs -----
    
    // // TCP Server receiving data
    // PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 7777));
    // ApplicationContainer serverApps = sink.Install(groundStations.Get(0));
    // serverApps.Start(Seconds(0));
    
    // TCP Client
    NS_LOG_UNCOND("Server IP: " << groundStations.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetAddress());
    //OnOffHelper onOffHelper("ns3::TcpSocketFactory", InetSocketAddress(groundStations.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetAddress(), 7777));
    //onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.5]"));
    //onOffHelper.SetAttribute("PacketSize", UintegerValue(512));
    
    //ApplicationContainer clientApps = onOffHelper.Install(groundStations.Get(1));

    UdpEchoServerHelper echoServer(7777);

    ApplicationContainer serverApps = echoServer.Install(groundStations.Get(0));
    serverApps.Start(Seconds(0.0));
    serverApps.Stop(Seconds(15.0));

    UdpEchoClientHelper echoClient(InetSocketAddress(groundStations.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetAddress(), 7777));
    echoClient.SetAttribute("MaxPackets", UintegerValue(5));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(3.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps = echoClient.Install(groundStations.Get(1));

    clientApps.Start(Seconds(0.0));
    clientApps.Stop(Seconds(15.0));

    // --------------------------------------

    // ----- scheduling link break and link creation ------
    Simulator::Schedule(Seconds(5), [&groundStations, theBannedChannel](){
        Ptr<CsmaChannel> chan = DynamicCast<CsmaChannel>(groundStations.Get(0)->GetDevice(1)->GetChannel());
        for (size_t device = 0; device < chan->GetNDevices(); ++device) {
            
            Ptr<CsmaNetDevice> currCsmaNetDevice = DynamicCast<CsmaNetDevice>(chan->GetDevice(device));
            
            // Attach the netdevice of the node to a null channel, and delete the channel represents the actual link.
            currCsmaNetDevice->Attach(theBannedChannel);
            groundStations.Get(device)->GetObject<Ipv4>()->SetDown(1);
        }
        chan->Dispose();
        
        NS_LOG_DEBUG("[E] Check of theBannedChanel ptr value " << theBannedChannel);
        NS_LOG_DEBUG("[E] Check of GS 0 conn to testChannel value " << groundStations.Get(0)->GetDevice(1)->GetChannel());
        NS_LOG_DEBUG("[E] Check of GS 1 conn to testChannel value " << groundStations.Get(1)->GetDevice(1)->GetChannel());

        //Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    });

    Simulator::Schedule(Seconds(7), [&groundStations](){
        Ptr<CsmaChannel> testChannel = CreateObject<CsmaChannel>();
        testChannel->SetAttribute("DataRate", StringValue("1MBps"));
        double delayVal = 3000000.0 / 299792458.0;  // seconds
        testChannel->SetAttribute("Delay", TimeValue(Seconds(delayVal)));
        
        DynamicCast<CsmaNetDevice>(groundStations.Get(0)->GetDevice(1))->Attach(testChannel);
        DynamicCast<CsmaNetDevice>(groundStations.Get(1)->GetDevice(1))->Attach(testChannel);

        groundStations.Get(0)->GetObject<Ipv4>()->SetUp(1);
        groundStations.Get(1)->GetObject<Ipv4>()->SetUp(1);

        // Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    });
    // ----------------------------------------------------


    // ===============================================================================================================


    // ========================================= Setup of NetAnimator mobility =========================================
    // Give each ground station a constant position model, and set the location from the satellite mobility model!
    for (uint32_t n = 0; n < groundStations.GetN(); n++) {
        GeoCoordinate gsNpos = groundStationsMobilityModels[n]->GetGeoPosition();
        AnimationInterface::SetConstantPosition(groundStations.Get(n), gsNpos.GetLongitude(), -gsNpos.GetLatitude());
    }

    // Run simulationphase at time 0
    simulationPhase(satellites, satelliteMobilityModels, groundStationsMobilityModels);
    // Run simulation phase at i intervals
    int interval = 60*0.1;
    for (int i = 1; i < 2; ++i) {
        Time t = Seconds(i * interval);
        Simulator::Schedule(t, simulationPhase, satellites, satelliteMobilityModels, groundStationsMobilityModels);
    }

    // Run NetAnim from the P5-Satellite folder
    AnimationInterface anim("scratch/P5-Satellite/p5-satellite.xml");
    // anim.EnablePacketMetadata();
    anim.SetBackgroundImage("resources/earth-map.jpg", -180, -90, 0.17578125, 0.17578125, 1);
    // Pretty Satellites :)
    for (uint32_t n = 0; n < satellites.GetN(); n++){
        anim.UpdateNodeDescription(n, TLEVector[n].name.substr(TLEVector[n].name.size() - 4,  4));  // Only works for starlink
        anim.UpdateNodeSize(n, 5, 5);
    }
    // Pretty Ground stations
    for (uint32_t n = 0; n < groundStations.GetN(); n++){
        anim.UpdateNodeColor(groundStations.Get(n), 0, 255, 255);
        anim.UpdateNodeSize(groundStations.Get(n), 3, 3);
    }
    anim.UpdateNodeDescription(groundStations.Get(0), "Hartebeesthoek");
    anim.UpdateNodeDescription(groundStations.Get(1), "Tea Gardens");
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

    NS_LOG_UNCOND("[!] Simulation is running!");
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}



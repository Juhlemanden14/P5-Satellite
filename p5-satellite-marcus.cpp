#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
// #include "ns3/satellite-module.h"
# include "ns3/csma-module.h"

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
    std::string tleDataPath = "scratch/P5-Satellite/resources/starlink_13-11-2024_tle_data.txt";
    int satelliteCount = 2;

    CommandLine cmd(__FILE__);
    cmd.AddValue("tledata", "TLE Data path", tleDataPath);
    cmd.AddValue("satCount", "The amount of satellites", satelliteCount);
    cmd.Parse(argc, argv);
    NS_LOG_INFO("[+] CommandLine arguments parsed succesfully");
    // ==========================================================================================================================


    // ========================================= TLE handling and node Setup =========================================
    std::vector<TLE> TLEVector;

    std::vector<Ptr<SatSGP4MobilityModel>> satelliteMobilityModels;
    NodeContainer satellites = createSatellitesFromTLE(satelliteCount, satelliteMobilityModels, tleDataPath, TLEVector);


    std::vector<Ptr<SatConstantPositionMobilityModel>> groundStationsMobilityModels;
    std::vector<GeoCoordinate> groundStationsCoordinates;

    // -25.8872, 27.7077, 1540 -- Hartebeesthoek, South Africa
    groundStationsCoordinates.emplace_back(GeoCoordinate(-25.8872, 27.7077, 1540));

    // -32.5931930, 152.1042000, 71 -- Tea Gardens, New South Wales Australia
    groundStationsCoordinates.emplace_back(GeoCoordinate(-32.5931930, 152.1042000, 71));

    NodeContainer groundStations = createGroundStations(2, groundStationsMobilityModels, groundStationsCoordinates);

    // Testing purposes
    int lookupIndex = 0;
    Ptr<Node> t = Names::Find<Node>(TLEVector[lookupIndex].name);
    NS_LOG_DEBUG(TLEVector[lookupIndex].name << " coords " << satelliteMobilityModels[lookupIndex]->GetGeoPosition());
    NS_LOG_INFO("[+] SatSGP4 Mobilty installed on " << satellites.GetN() << " satellites");

    // Testing purposes
    NS_LOG_DEBUG("GS-0 coords " << groundStationsMobilityModels[0]->GetGeoPosition());
    double gs_sat_dist = groundStationsMobilityModels[0]->GetDistanceFrom(satelliteMobilityModels[lookupIndex]);
    NS_LOG_DEBUG("Distance between GS 0 and sat 1 is -> " << gs_sat_dist/1000 << " km");
    // ===============================================================================================================


    // ========================================= Link handling =========================================
    // params
    StringValue csmaDataRate = StringValue("20MBps");
    // setup
    CsmaHelper csmaHelper;
    csmaHelper.SetChannelAttribute("DataRate", csmaDataRate);
    
    // ---------- GS links -----------
    for (int gs_index = 0; gs_index < 2; gs_index++) {  // Loop over each GS and create a CsmaNetDevice for each of them
        Ptr<Node> currentGS = groundStations.Get(gs_index);
        csmaHelper.Install(currentGS);   // Installs both a CsmaNetDevice and a Channel for each GS


        Ptr<Channel> GSChannel = currentGS->GetDevice(1)->GetChannel(); // THERE IS ALWAYS A 0'th NETDEVICE FOR SOME FUCKING REASON. maybe loopback?
        NS_LOG_DEBUG("[E] GS channel: " << GSChannel);   // debugging print before deleting channel
        if (GSChannel != nullptr) {
            // currentGS->GetDevice(1)->GetChannel()->Dispose();    // delete unused channels if exists
            Ptr<CsmaNetDevice> currCsmaNetDevice = DynamicCast<CsmaNetDevice>(currentGS->GetDevice(1));
            NS_LOG_DEBUG("[E] currCsmaNetDevice: " << currCsmaNetDevice);
            NS_LOG_DEBUG("[E] GS channel ID: " << currentGS->GetDevice(1));
        }
        NS_LOG_DEBUG("[E] GS channel: " << currentGS->GetDevice(1)->GetChannel()); // debugging print after deleting channel
        NS_LOG_DEBUG("[E] Mac shit: " << currentGS->GetDevice(1)->GetAddress());
        
    }

    
    // -------------------------------



    // do same for ISLs
    NS_LOG_DEBUG("[E] Satellite count: " << satelliteCount);
    for (int sat_index = 0; sat_index < satelliteCount; sat_index++) {  // Loop over each sat and create 4 CsmaNetDevice for each
        Ptr<Node> currentSat = satellites.Get(sat_index);
        for (int i = 1; i <= 4; i++) {                   // one inner loop for each satellite to install 4 Laser terminals (CsmaNetDevices)
            csmaHelper.Install(currentSat);             // Installs both a CsmaNetDevice and a Channel.
            currentSat->GetDevice(i)->GetChannel()->Dispose();   // delete unused channel in each iteration
            
        }
        int numChannels = currentSat->GetNDevices();
        NS_LOG_DEBUG("[E] Sat Num Channels: " << numChannels);

        Ptr<Channel> SatChannel = currentSat->GetDevice(1)->GetChannel();
        NS_LOG_DEBUG("[E] SatChannel 1: " << SatChannel);
        SatChannel = currentSat->GetDevice(2)->GetChannel();
        NS_LOG_DEBUG("[E] SatChannel 2: " << SatChannel);
        SatChannel = currentSat->GetDevice(3)->GetChannel();
        NS_LOG_DEBUG("[E] SatChannel 3: " << SatChannel);
        SatChannel = currentSat->GetDevice(4)->GetChannel();
        NS_LOG_DEBUG("[E] SatChannel 4: " << SatChannel);
    }

    // Try to see what happens when I install more channels (like after n seconds)
    Ptr<Channel> SatChannel = satellites.Get(0)->GetDevice(4)->GetChannel();
    NS_LOG_DEBUG("[E] SatChannel 4: " << SatChannel);

    // Ptr<NetDevice> sat0Net1 = satellites.Get(0)->GetDevice(1);
    // Ptr<NetDevice> sat1Net3 = satellites.Get(1)->GetDevice(3);
    // csmaHelper.Install(sat0Net1, sat1Net3->GetChannel());
    // NS_LOG_DEBUG("[E] Sat0Net1 channel: " << sat0Net1->GetChannel());
    // NS_LOG_DEBUG("[E] Sat1Net3 channel: " << sat1Net3->GetChannel());

    // =================================================================================================



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
    for (int i = 1; i < 20; ++i) {
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

    NS_LOG_UNCOND("[!] Simulation is running!");
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}



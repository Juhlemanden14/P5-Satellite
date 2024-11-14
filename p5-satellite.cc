#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
// #include "ns3/satellite-module.h"

#include "tle-handler.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("P5-Satellite");

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
    NS_LOG_DEBUG("[->] Simulation phase:");

    // Set the new positions of the satellites and update their position in Net Animator.
    for (uint32_t n = 0; n < satellites.GetN(); ++n) {

        GeoCoordinate satPos = satelliteMobilityModels[n]->GetGeoPosition();
        AnimationInterface::SetConstantPosition(satellites.Get(n), satPos.GetLongitude(), -satPos.GetLatitude());
    }
    // Gets the time in the simulation
    NS_LOG_DEBUG(Simulator::Now().GetSeconds() << " seconds have elapsed");
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


    

    // ========================================= Import TLE data =========================================
    std::string TLEAge;
    std::vector<TLE> TLEVector = ReadTLEFile(tleDataPath, TLEAge);
    NS_LOG_INFO("[+] Imported TLE data for " << TLEVector.size() << " satellites, with age " << TLEAge);
    NS_ASSERT_MSG(TLEVector.size() != 0, "No satellites were imported?");

    // If no amount of sallites are specified, set equal to the amount of satellites in the TLE data
    if (satelliteCount == 0) {
        satelliteCount = TLEVector.size();
    }
    // =================================================================================================== 
    


    
    // ========================================= Node Setup =========================================
    NodeContainer satellites(satelliteCount);
    std::vector<Ptr<SatSGP4MobilityModel>> satelliteMobilityModels;

    NodeContainer groundStations(2);
    std::vector<Ptr<SatConstantPositionMobilityModel>> groundStationsMobilityModels;
    std::vector<GeoCoordinate> groundStationsCoordinates;

    groundStationsCoordinates.emplace_back(GeoCoordinate(57.0311, 9.5504, 0));
    groundStationsCoordinates.emplace_back(GeoCoordinate(52.4405, 16.3008, 0));

    NS_LOG_INFO("[+] " << satelliteCount << " satellite nodes have been created");

    // Install the internet stack on the satellites and the ground stations.
    InternetStackHelper stackHelper;
    stackHelper.Install(satellites);
    stackHelper.Install(groundStations);
    NS_LOG_INFO("[+] Internet stack installed");
    
    // Create and aggregate the Satellite SGP4 mobility model to each satellite
    std::string formatted_TLE;
    for (int n = 0; n < satelliteCount; ++n) {
        Ptr<SatSGP4MobilityModel> satMobility = CreateObject<SatSGP4MobilityModel>();
        
        // Format the two lines into a single string for NS-3 compatibility - IT MUST BE line1\nline2 WITH NO SPACES!!!
        formatted_TLE = TLEVector[n].line1 + "\n" + TLEVector[n].line2; 
        satMobility->SetTleInfo(formatted_TLE);
        // Set the simulation absolute start time in string format.
        satMobility->SetStartDate(TLEAge);

        // satellites.Get(n)->AggregateObject(satMobility);
        // keep nodes and mobility models seperated - works better with netanimator later on this way.
        satelliteMobilityModels.emplace_back(satMobility);

        // Give each satellite a name equal to the one specified in the TLE data
        Names::Add(TLEVector[n].name, satellites.Get(n));
    }

    // Testing purposes
    int lookupIndex = 1;
    Ptr<Node> t = Names::Find<Node>(TLEVector[lookupIndex].name);
    NS_LOG_DEBUG(TLEVector[lookupIndex].name << " coords " << satelliteMobilityModels[lookupIndex]->GetGeoPosition());
    NS_LOG_INFO("[+] SatSGP4 Mobilty installed on " << satellites.GetN() << " satellites");


    for (size_t n = 0; n < groundStationsCoordinates.size(); ++n) {
        Ptr<Node> gs = groundStations.Get(n);
        // make a similar mobility model for GS's even though they don't move. It just allows use of methods like .GetDistanceFrom(GS) etc.
        Ptr<SatConstantPositionMobilityModel> GSMobility = CreateObject<SatConstantPositionMobilityModel>();
        GSMobility->SetGeoPosition(groundStationsCoordinates[n]);
        groundStationsMobilityModels.emplace_back(GSMobility);

        // Give each ground station a constant position model, and set the location from the satellite mobility model!
        GeoCoordinate gsNpos = groundStationsMobilityModels[n]->GetGeoPosition();
        AnimationInterface::SetConstantPosition(groundStations.Get(n), gsNpos.GetLongitude(), -gsNpos.GetLatitude());
    }
    NS_LOG_DEBUG("[+] SatConstantPositionMobilityModel installed on " << groundStations.GetN() << " ground stations");

    // Testing purposes
    NS_LOG_DEBUG("GS-0 coords " << groundStationsMobilityModels[0]->GetGeoPosition());
    double gs_sat_dist = groundStationsMobilityModels[0]->GetDistanceFrom(satelliteMobilityModels[1]);
    NS_LOG_DEBUG("Distance between GS 0 and sat 1 is -> " << gs_sat_dist/1000 << " km");
    // ==============================================================================================




    // ========================================= Setup of NetAnimator mobility =========================================
    // Run simulationphase at time 0
    simulationPhase(satellites, satelliteMobilityModels, groundStationsMobilityModels);
    // Run simulation phase at i intervals
    int interval = 60*0.1;
    for (int i = 1; i < 2000; ++i) {
        Time t = Seconds(i * interval);
        Simulator::Schedule(t, simulationPhase, satellites, satelliteMobilityModels, groundStationsMobilityModels);
    }
    


    AnimationInterface anim("p5-satellite.xml");
    // anim.EnablePacketMetadata();
    anim.SetBackgroundImage("scratch/P5-Satellite/resources/earth-map.jpg", -180, -90, 0.17578125, 0.17578125, 1);
    for (uint32_t n = 0; n < satellites.GetN(); n++){
        anim.UpdateNodeDescription(n, TLEVector[n].name);
        anim.UpdateNodeSize(n, 5, 5);
    }
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



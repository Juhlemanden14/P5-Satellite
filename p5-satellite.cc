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
    NodeContainer groundStations(2);
    std::vector<GeoCoordinate> GSCoordinates;
    GSCoordinates.emplace_back(GeoCoordinate(57.0311, 9.5504, 0));
    GSCoordinates.emplace_back(GeoCoordinate(52.4405, 16.3008, 0));

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
        satellites.Get(n)->AggregateObject(satMobility);

        // Give each satellite a name equal to the one specified in the TLE data
        Names::Add(TLEVector[n].name, satellites.Get(n));
    }
    // Testing purposes
    Ptr<Node> t = Names::Find<Node>(TLEVector[1].name);
    NS_LOG_DEBUG(TLEVector[1].name << " coords " << t->GetObject<SatMobilityModel>()->GetGeoPosition());
    NS_LOG_INFO("[+] SatSGP4 Mobilty installed on " << satellites.GetN() << " satellites");


    for (size_t n = 0; n < GSCoordinates.size(); ++n) {
        Ptr<Node> gs = groundStations.Get(n);
        // make a similar mobility model for GS's even though they don't move. It just allows use of methods like .GetDistanceFrom(GS) etc.
        Ptr<SatConstantPositionMobilityModel> GSMobility = CreateObject<SatConstantPositionMobilityModel>();
        GSMobility->SetGeoPosition(GSCoordinates[n]);
        gs->AggregateObject(GSMobility);
    }
    NS_LOG_DEBUG("[+] SatConstantPositionMobilityModel installed on " << groundStations.GetN() << " ground stations");
    // Testing purposes
    NS_LOG_DEBUG("GS-0 coords " << groundStations.Get(1)->GetObject<SatMobilityModel>()->GetGeoPosition());
    double gs_sat_dist = groundStations.Get(1)->GetObject<SatMobilityModel>()->GetDistanceFrom(satellites.Get(1)->GetObject<SatMobilityModel>());
    NS_LOG_DEBUG("Distance between GS 0 and sat 0 is -> " << gs_sat_dist/1000 << " km");
    // ==============================================================================================




    // ========================================= Setup of NetAnimator mobility =========================================
    AnimationInterface anim("p5-satellite.xml");
    // anim.EnablePacketMetadata();
    anim.SetBackgroundImage("scratch/P5-Satellite/resources/earth-map.jpg", -180, -90, 0.17578125, 0.17578125, 1);
    


    // ==================================================================================================================
    exit(0);





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



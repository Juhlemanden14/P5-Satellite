#include "constellationHandler.h"

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
#include "ns3/csma-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("P5-Constellation-Handler");

NodeContainer createSatellitesFromTLEAndOrbits(uint32_t satelliteCount, std::vector<Ptr<SatSGP4MobilityModel>> &satelliteMobilityModels, std::string tleDataPath, std::string orbitsDataPath, std::vector<TLE> &TLEVector, std::vector<Orbit> &OrbitVector) {
    LogComponentEnable("P5-Constellation-Handler", LOG_LEVEL_ALL);

    // Read orbit data
    OrbitVector = ReadOrbitFile(orbitsDataPath);
    NS_LOG_INFO("[+] Imported orbit data for " << OrbitVector.size() << " orbits");

    std::string TLEAge;
    TLEVector = ReadTLEFile(tleDataPath, TLEAge);

    // For each satellite in the orbits, only grab that from the TLE data
    std::vector<TLE> tmp;
    for (auto& orbit : OrbitVector) {
        for (auto& name : orbit.satellites) {
            for (auto& tle : TLEVector){
                if (name == tle.name)
                    tmp.push_back(tle);
            }
        }
    }
    TLEVector = tmp;
    NS_LOG_INFO("[+] Imported TLE data for " << TLEVector.size() << " satellites, with age " << TLEAge);
    NS_ASSERT_MSG(TLEVector.size() != 0, "No satellites were imported?");


    // If no amount of satellites is specified, set it to the number of satellites in the orbit TLE data
    if (satelliteCount == 0 || satelliteCount > TLEVector.size()) {
        satelliteCount = TLEVector.size(); // Change this if you want to include all satellites from TLE data!
    }
    // LIGE HER

    // Create satellite nodes
    NodeContainer satellites(satelliteCount);
    NS_LOG_INFO("[+] " << satelliteCount << " satellite nodes have been created");

    // Install the internet stack on the satellites
    InternetStackHelper stackHelper;
    stackHelper.Install(satellites);
    NS_LOG_INFO("[+] Internet stack installed on satellites");
    
    // Create and aggregate the Satellite SGP4 mobility model to each satellite
    std::string formatted_TLE;
    for (uint32_t n = 0; n < satelliteCount; ++n) {
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

    return satellites;
}


NodeContainer createGroundStations(int groundStationCount, std::vector<Ptr<SatConstantPositionMobilityModel>> &groundStationsMobilityModels, std::vector<GeoCoordinate> groundStationsCoordinates, Ptr<CsmaChannel> &nullChannel) {
    LogComponentEnable("P5-Constellation-Handler", LOG_LEVEL_ALL);
    
    NodeContainer groundStations(groundStationCount);

    InternetStackHelper stackHelper;
    stackHelper.Install(groundStations);

    CsmaHelper csmaHelper;

    Ipv4AddressHelper gsAddressHelper;
    gsAddressHelper.SetBase("10.0.0.0", "255.0.0.0");
    
    NetDeviceContainer gsNetDevices;

    NS_LOG_INFO("[+] Internet stack installed on groundstations");

    for (size_t n = 0; n < groundStationsCoordinates.size(); ++n) {
        // make a similar mobility model for GS's even though they don't move. It just allows use of methods like .GetDistanceFrom(GS) etc.
        Ptr<SatConstantPositionMobilityModel> GSMobility = CreateObject<SatConstantPositionMobilityModel>();
        GSMobility->SetGeoPosition(groundStationsCoordinates[n]);
        groundStationsMobilityModels.emplace_back(GSMobility);

        Ptr<Node> currentGS = groundStations.Get(n);
        csmaHelper.Install(currentGS);   // Installs both a CsmaNetDevice and a Channel for each GS
        if (currentGS->GetDevice(1)->GetChannel() != nullptr) {
            currentGS->GetDevice(1)->GetChannel()->Dispose();       // delete unused channels if exists
        }
        // NS_LOG_DEBUG("[E] MAC: " << currentGS->GetDevice(1)->GetAddress());

        // make sure that channel pointer is pointing to the banned channel, since channel is now destroyed
        Ptr<CsmaNetDevice> currCsmaNetDevice = DynamicCast<CsmaNetDevice>(currentGS->GetDevice(1));
        currCsmaNetDevice->Attach(nullChannel);

        gsNetDevices.Add(currentGS->GetDevice(1));
    }
    NS_LOG_DEBUG("[+] SatConstantPositionMobilityModel installed on " << groundStations.GetN() << " ground stations");
    
    gsAddressHelper.Assign(gsNetDevices);

    csmaHelper.EnablePcapAll("scratch/P5-Satellite/out/ground-station", true);

    return groundStations;
}

/*
void updateGroundStationLinks(std::vector<Ptr<SatConstantPositionMobilityModel>> &groundStationsMobilityModels, NodeContainer &groundStations, std::vector<Ptr<SatSGP4MobilityModel>> &satelliteMobilityModels, NodeContainer &satellites) {
    
    if (checkGSLink() == false) {
        Do stuff to get new link:
        SearchNewLink(max_dist, someOtherParams) -> new_link
        Establish link
    }
    else {
        do nothing
    }
}
*/

bool checkGSLink(int gsIndex, std::vector<Ptr<SatConstantPositionMobilityModel>> &groundStationsMobilityModels, NodeContainer &groundStations, std::vector<Ptr<SatSGP4MobilityModel>> &satelliteMobilityModels, NodeContainer &satellites, double maxDistanceKM = 3000) {
    
    Ptr<NetDevice> gsNetDevice = groundStations.Get(gsIndex)->GetDevice(0);
    int attachedChannelNetDevices = groundStations.Get(gsIndex)->GetDevice(0)->GetChannel()->GetNDevices();

    if (attachedChannelNetDevices != 2) {
        return false;
    }

    for (int devices = 0; devices < attachedChannelNetDevices; ++devices) {

        Ptr<NetDevice> attachedChannelDevice = groundStations.Get(gsIndex)->GetDevice(0)->GetChannel()->GetDevice(devices);

        // Check if the first netdevice of the channel is the satellite one.
        if (gsNetDevice != attachedChannelDevice) { // If we enter we know that the 'attachedChannelDevice' is a satellite device.
            
            // Get the satellite index id, as it is created before the ground stations, we can use its id.
            int satId = attachedChannelDevice->GetNode()->GetId();
            
            // Check distance between the satellite and the ground station
            double gsSatDist = groundStationsMobilityModels[gsIndex]->GetDistanceFrom(satelliteMobilityModels[satId]);
            if ( (gsSatDist/1000) < maxDistanceKM) {
                // Distance to satellite is in reach to keep the link.
                return true;
            } else {
                // Distance to satellite is too great to keep the connection.
                return false;
            }
        }
    }

    return false;
}


void establishLink(Ptr<Node> node1, Ptr<Node> node2, double distanceKM, StringValue dataRate) {

}
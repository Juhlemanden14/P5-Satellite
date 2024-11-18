#include "constellationHandler.h"

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("P5-Constellation-Handler");

NodeContainer createSatellitesFromTLE(int satelliteCount, std::vector<Ptr<SatSGP4MobilityModel>> &satelliteMobilityModels, std::string tleDataPath, std::vector<TLE> &TLEDataVector) {
    LogComponentEnable("P5-Constellation-Handler", LOG_LEVEL_ALL);
    
    std::string TLEAge;
    TLEDataVector = ReadTLEFile(tleDataPath, TLEAge);
    NS_LOG_INFO("[+] Imported TLE data for " << TLEDataVector.size() << " satellites, with age " << TLEAge);
    NS_ASSERT_MSG(TLEDataVector.size() != 0, "No satellites were imported?");

    // If no amount of sallites are specified, set equal to the amount of satellites in the TLE data
    if (satelliteCount == 0) {
        satelliteCount = TLEDataVector.size();
    }

    // Create satellite nodes
    NodeContainer satellites(satelliteCount);
    NS_LOG_INFO("[+] " << satelliteCount << " satellite nodes have been created");

    // Install the internet stack on the satellites
    InternetStackHelper stackHelper;
    stackHelper.Install(satellites);
    NS_LOG_INFO("[+] Internet stack installed on satellites");
    
    // Create and aggregate the Satellite SGP4 mobility model to each satellite
    std::string formatted_TLE;
    for (int n = 0; n < satelliteCount; ++n) {
        Ptr<SatSGP4MobilityModel> satMobility = CreateObject<SatSGP4MobilityModel>();
        
        // Format the two lines into a single string for NS-3 compatibility - IT MUST BE line1\nline2 WITH NO SPACES!!!
        formatted_TLE = TLEDataVector[n].line1 + "\n" + TLEDataVector[n].line2; 
        satMobility->SetTleInfo(formatted_TLE);
        // Set the simulation absolute start time in string format.
        satMobility->SetStartDate(TLEAge);

        // satellites.Get(n)->AggregateObject(satMobility);
        // keep nodes and mobility models seperated - works better with netanimator later on this way.
        satelliteMobilityModels.emplace_back(satMobility);

        // Give each satellite a name equal to the one specified in the TLE data
        Names::Add(TLEDataVector[n].name, satellites.Get(n));
    }

    return satellites;
}


NodeContainer createGroundStations(int groundStationCount, std::vector<Ptr<SatConstantPositionMobilityModel>> &groundStationsMobilityModels, std::vector<GeoCoordinate> groundStationsCoordinates) {
    LogComponentEnable("P5-Constellation-Handler", LOG_LEVEL_ALL);
    
    NodeContainer groundStations(groundStationCount);

    InternetStackHelper stackHelper;
    stackHelper.Install(groundStations);
    NS_LOG_INFO("[+] Internet stack installed on groundstations");

    for (size_t n = 0; n < groundStationsCoordinates.size(); ++n) {
        // make a similar mobility model for GS's even though they don't move. It just allows use of methods like .GetDistanceFrom(GS) etc.
        Ptr<SatConstantPositionMobilityModel> GSMobility = CreateObject<SatConstantPositionMobilityModel>();
        GSMobility->SetGeoPosition(groundStationsCoordinates[n]);
        groundStationsMobilityModels.emplace_back(GSMobility);

        CsmaHelper csma;
        NetDeviceContainer deviceContainer = csma.Install(groundStations.Get(n));
        /*
        deviceContainer.Get(0)->
        
        Ptr<CsmaChannel> csmaChannel = DynamicCast<CsmaChannel>(groundStations.Get(n)->GetDevice(0)->GetChannel());
        csmaChannel->Detach(0);
        Ptr<CsmaNetDevice> csmaDevice = DynamicCast<CsmaNetDevice>(deviceContainer.Get(0));
        */
    }
    NS_LOG_DEBUG("[+] SatConstantPositionMobilityModel installed on " << groundStations.GetN() << " ground stations");

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

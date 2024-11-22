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

    CsmaHelper csmaHelper;
    
    // Create and aggregate the Satellite SGP4 mobility model to each satellite
    std::string formatted_TLE;
    for (uint32_t n = 0; n < satelliteCount; ++n) {
        Ptr<SatSGP4MobilityModel> satMobility = CreateObject<SatSGP4MobilityModel>();
        
        // Format the two lines into a single string for NS-3 compatibility - IT MUST BE line1\nline2 WITH NO SPACES!!!
        formatted_TLE = TLEVector[n].line1 + "\n" + TLEVector[n].line2; 
        satMobility->SetTleInfo(formatted_TLE);
        // Set the simulation absolute start time in string format.
        satMobility->SetStartDate(TLEAge);

        // Ignore device with index 0, as it is the loopback interface.
        for (int i = 1; i <= 5; ++i) {
            Ptr<Node> currentSat = satellites.Get(n);
            Ptr<NetDevice> device = csmaHelper.Install(currentSat).Get(0);   // Installs both a CsmaNetDevice and a Channel for each GS
            device->GetChannel()->Dispose();   // Delete the channel.
            // NS_LOG_DEBUG("[E] MAC: " << currentGS->GetDevice(1)->GetAddress());

            // make sure that channel pointer is pointing to the null channel, since channel is now destroyed
            Ptr<CsmaNetDevice> currCsmaNetDevice = DynamicCast<CsmaNetDevice>(device);
            Ptr<CsmaChannel> nullChannel = CreateObject<CsmaChannel>();
            currCsmaNetDevice->Attach(nullChannel);
        }
        csmaHelper.EnablePcap("scratch/P5-Satellite/out/satellite", satellites, true);

        // keep nodes and mobility models seperated - works better with netanimator later on this way.
        satelliteMobilityModels.emplace_back(satMobility);

        // Give each satellite a name equal to the one specified in the TLE data
        Names::Add(TLEVector[n].name, satellites.Get(n));
    }

    return satellites;
}


NodeContainer createGroundStations(int groundStationCount, std::vector<Ptr<SatConstantPositionMobilityModel>> &groundStationsMobilityModels, std::vector<GeoCoordinate> groundStationsCoordinates, Ptr<CsmaChannel> &nullChannel) {
    
    NodeContainer groundStations(groundStationCount);

    InternetStackHelper stackHelper;
    stackHelper.Install(groundStations);
    NS_LOG_INFO("[+] Internet stack installed on groundstations");
    
    CsmaHelper csmaHelper;
    Ipv4AddressHelper gsAddressHelper;
    gsAddressHelper.SetBase("1.0.0.0", "255.255.255.0");

    // For each ground station
    for (size_t n = 0; n < groundStationsCoordinates.size(); ++n) {
        // Create the single netdevice on each ground station
        NetDeviceContainer gsNetDevice = csmaHelper.Install(groundStations.Get(n));
        // Assign an ip to the ground station.
        gsAddressHelper.Assign(gsNetDevice);
        // Migrate to a new subnet for future ground stations
        gsAddressHelper.NewNetwork();

        Ptr<CsmaNetDevice> currCsmaNetDevice = DynamicCast<CsmaNetDevice>(gsNetDevice.Get(0));
        Ptr<CsmaChannel> nullChannel = CreateObject<CsmaChannel>();
        currCsmaNetDevice->Attach(nullChannel);
        gsNetDevice.Get(0)->GetChannel()->Dispose();       // delete unused channels

        // Ground station mobility
        // make a similar mobility model for GS's even though they don't move. It just allows use of methods like .GetDistanceFrom(GS) etc.
        Ptr<SatConstantPositionMobilityModel> GSMobility = CreateObject<SatConstantPositionMobilityModel>();
        GSMobility->SetGeoPosition(groundStationsCoordinates[n]);
        groundStationsMobilityModels.emplace_back(GSMobility);
    }
    NS_LOG_DEBUG("[+] SatConstantPositionMobilityModel installed on " << groundStations.GetN() << " ground stations");
    csmaHelper.EnablePcap("scratch/P5-Satellite/out/ground-station", groundStations, true);

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


int establishLink(Ptr<Node> node1, int node1NetDeviceIndex, Ptr<Node> node2, int node2NetDeviceIndex, double distanceKM, StringValue dataRate, Ipv4Address networkAddress) {
    if (node1NetDeviceIndex > 5 || node2NetDeviceIndex > 5) {   // check if any indexes are out of bounds. GS's must be handled seperately
        return -1;
    }
    
    Ipv4AddressHelper addrHelper;
    addrHelper.SetBase(networkAddress, "255.255.255.0");
    //addrHelper.NewNetwork();

    Ptr<CsmaChannel> linkChannel = CreateObject<CsmaChannel>();             // Create a new channel
    linkChannel->SetAttribute("DataRate", dataRate);                        // set datarate and channel delay (based on speed of light)
    double c = 299792.4580;             // speed of light in km/s      
    double delayVal = distanceKM / c;   // seconds
    linkChannel->SetAttribute("Delay", TimeValue(Seconds(delayVal)));

    DynamicCast<CsmaNetDevice>(node1->GetDevice(node1NetDeviceIndex))->Attach(linkChannel); // attach node's x'th NetDevice to the new channel
    DynamicCast<CsmaNetDevice>(node2->GetDevice(node2NetDeviceIndex))->Attach(linkChannel);

    Ipv4InterfaceContainer interface1 = addrHelper.Assign(node1->GetDevice(node1NetDeviceIndex));
    Ipv4InterfaceContainer interface2 = addrHelper.Assign(node2->GetDevice(node2NetDeviceIndex));

    NS_LOG_DEBUG("IP1: " << interface1.Get(0).first->GetAddress(1, 0).GetAddress());
    NS_LOG_DEBUG("IP2: " << interface2.Get(0).first->GetAddress(1, 0).GetAddress());
    
    node1->GetObject<Ipv4>()->SetUp(node1NetDeviceIndex);   // Enable its IPv4 interface again. Using same index variable indicates  
    node2->GetObject<Ipv4>()->SetUp(node2NetDeviceIndex);   // a correlation between NetDevice and IPv4 interface. If this is false, fuck...
    node1->GetDevice(1)->Dispose();
    return 0;       // indicating no error
}


int destroyLink(Ptr<Node> node1, int node1NetDeviceIndex) {
    if (node1NetDeviceIndex > 5) {  // check if any indexes are out of bounds. GS's must be handled seperately
        return -1;
    }
    Ptr<CsmaChannel> channelLink = DynamicCast<CsmaChannel>(node1->GetDevice(node1NetDeviceIndex)->GetChannel());  // Get the channel

    Ptr<CsmaNetDevice> currCsmaNetDevice = DynamicCast<CsmaNetDevice>(channelLink->GetDevice(0));     // Get connected netDevice
    Ptr<Node> node = currCsmaNetDevice->GetNode();

    Ptr<CsmaChannel> currNullChannel = CreateObject<CsmaChannel>();
    
    for (size_t nodeDevice = 1; nodeDevice < node->GetNDevices() - 1; ++nodeDevice) {

        // Ptr<CsmaChannel> nullChannel = CreateObject<CsmaChannel>();

        // If we have selected the link that is currently connected.
        if (node->GetDevice(nodeDevice)->GetChannel() == channelLink) {
            // We create a pointer to the net device at the other end of the link
            Ptr<CsmaNetDevice> otherCsmaNetDevice = DynamicCast<CsmaNetDevice>(node->GetDevice(nodeDevice));
            // We attach it to the null channel
            // otherCsmaNetDevice->Attach(nullChannel);
            // nullChannel->Detach(otherCsmaNetDevice);
            // And set down the interface
            node->GetObject<Ipv4>()->SetDown(nodeDevice);
        }
    }
    
    // Attach the netdevice of the node to a null channel, and delete the channel represents the actual link.
    // currCsmaNetDevice->Attach(currNullChannel);
    // currNullChannel->Detach(currCsmaNetDevice);
    node1->GetObject<Ipv4>()->SetDown(node1NetDeviceIndex);  // TODO: How do we handle this??? Which interface are we disabling on the opposite end???
    channelLink->Dispose();

    return 0;   // indicating no error
}
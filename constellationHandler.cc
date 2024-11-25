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

// Class constructor.
Constellation::Constellation(uint32_t satCount, std::string tleDataPath, std::string orbitsDataPath, uint32_t gsCount, std::vector<GeoCoordinate> groundStationsCoordinates) {

    // Needed to avoid address collision. (Simulator issue, not real address collision)
    Ipv4AddressGenerator::TestMode();

    this->satelliteCount = satCount;
    this->groundStationCount = gsCount;

    // Create the satellites in the constellation.
    this->satelliteNodes = this->createSatellitesFromTLEAndOrbits(tleDataPath, orbitsDataPath);

    // Create the ground stations in the constellation.
    this->groundStationNodes = this->createGroundStations(groundStationsCoordinates);
}

NodeContainer Constellation::createSatellitesFromTLEAndOrbits(std::string tleDataPath, std::string orbitsDataPath) {

    // Read orbit data
    this->OrbitVector = ReadOrbitFile(orbitsDataPath);
    NS_LOG_INFO("[+] Imported orbit data for " << this->OrbitVector.size() << " orbits");

    std::string TLEAge;
    this->TLEVector = ReadTLEFile(tleDataPath, TLEAge);

    // For each satellite in the orbits, only grab that from the TLE data
    std::vector<TLE> tmp;
    for (Orbit orbit : this->OrbitVector) {
        for (std::string name : orbit.satellites) {
            for (TLE tle : this->TLEVector){
                if (name == tle.name)
                    tmp.push_back(tle);
            }
        }
    }
    this->TLEVector = tmp;
    NS_LOG_INFO("[+] Imported TLE data for " << this->TLEVector.size() << " satellites, with age " << TLEAge);
    NS_ASSERT_MSG(this->TLEVector.size() != 0, "No satellites were imported?");


    // If no amount of satellites is specified, set it to the number of satellites in the orbit TLE data
    if ( (this->satelliteCount == 0 || this->satelliteCount) > this->TLEVector.size()) {
        this->satelliteCount = this->TLEVector.size(); // Change this if you want to include all satellites from TLE data!
    }
    // LIGE HER

    // Create satellite nodes
    NodeContainer satellites(this->satelliteCount);
    NS_LOG_INFO("[+] " << this->satelliteCount << " satellite nodes have been created");

    // Install the internet stack on the satellites
    InternetStackHelper stackHelper;
    stackHelper.Install(satellites);
    NS_LOG_INFO("[+] Internet stack installed on satellites");

    CsmaHelper csmaHelper;
    
    // Create and aggregate the Satellite SGP4 mobility model to each satellite
    std::string formatted_TLE;
    for (uint32_t n = 0; n < this->satelliteCount; ++n) {
        Ptr<SatSGP4MobilityModel> satMobility = CreateObject<SatSGP4MobilityModel>();
        
        // Format the two lines into a single string for NS-3 compatibility - IT MUST BE line1\nline2 WITH NO SPACES!!!
        formatted_TLE = this->TLEVector[n].line1 + "\n" + this->TLEVector[n].line2; 
        satMobility->SetTleInfo(formatted_TLE);
        // Set the simulation absolute start time in string format.
        satMobility->SetStartDate(TLEAge);


        // Assign a dummy address to all satellites to give them an interface.
        Ipv4AddressHelper tmpAddrHelper;
        tmpAddrHelper.SetBase("1.0.0.0", "255.255.255.0");

        // Ignore device with index 0, as it is the loopback interface.
        for (int i = 1; i <= 5; ++i) {
            Ptr<Node> currentSat = satellites.Get(n);
            Ptr<Ipv4> satIpv4 = currentSat->GetObject<Ipv4>();
            // Installs both a CsmaNetDevice and a Channel for each GS
            Ptr<NetDevice> device = csmaHelper.Install(currentSat).Get(0);
            // Temporarly assigns IP address to the netdevice, to also get a Ipv4Interface installed
            tmpAddrHelper.Assign(device);
            tmpAddrHelper.NewAddress();
            
            // Set the Ipv4Interface for this NetDevice down and remove its address
            satIpv4->SetDown(i);
            satIpv4->RemoveAddress(i, 0);

            // make sure that channel pointer is pointing to the null channel, since channel is now destroyed
            device->GetChannel()->Dispose();   // Delete the channel.
            Ptr<CsmaNetDevice> currCsmaNetDevice = DynamicCast<CsmaNetDevice>(device);
            Ptr<CsmaChannel> nullChannel = CreateObject<CsmaChannel>();
            currCsmaNetDevice->Attach(nullChannel);
        }
        csmaHelper.EnablePcap("scratch/P5-Satellite/out/satellite", satellites, true);

        // keep nodes and mobility models seperated - works better with netanimator later on this way.
        this->satelliteMobilityModels.emplace_back(satMobility);

        // Give each satellite a name equal to the one specified in the TLE data
        Names::Add(this->TLEVector[n].name, satellites.Get(n));
    }

    return satellites;
}


NodeContainer Constellation::createGroundStations(std::vector<GeoCoordinate> groundStationsCoordinates) {
    
    NodeContainer groundStations(this->groundStationCount);

    InternetStackHelper stackHelper;
    stackHelper.Install(groundStations);
    NS_LOG_INFO("[+] Internet stack installed on groundstations");
    
    CsmaHelper csmaHelper;
    Ipv4AddressHelper gsAddressHelper;
    gsAddressHelper.SetBase("10.0.0.0", "255.255.255.0");

    // For each ground station
    for (size_t n = 0; n < groundStationsCoordinates.size(); ++n) {
        // Create the single netdevice on each ground station
        NetDeviceContainer gsNetDevice = csmaHelper.Install(groundStations.Get(n));
        // Assign an ip to the ground station but turn the interface down!
        gsAddressHelper.Assign(gsNetDevice);
        groundStations.Get(n)->GetObject<Ipv4>()->SetDown(1);
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
        this->groundStationsMobilityModels.emplace_back(GSMobility);
    }
    NS_LOG_DEBUG("[+] SatConstantPositionMobilityModel installed on " << groundStations.GetN() << " ground stations");
    csmaHelper.EnablePcap("scratch/P5-Satellite/out/ground-station", groundStations, true);

    return groundStations;
}
// --------------------------------------------------------



// Function to be scheduled periodically in the ns3 simulator.
void Constellation::simulationLoop(int totalMinutes, int updateIntervalSeconds) {
    
    // Run simulation phase at i intervals
    int loops = int(60*totalMinutes / updateIntervalSeconds);
    NS_LOG_DEBUG("[+] Simulation scheduled to loop " << loops << " times");

    // Update constellation for time 0
    this->updateConstellation();
    for (int i = 1; i < loops; ++i) {
        Time t = Seconds(i * updateIntervalSeconds);

        Simulator::Schedule(t, [this](){
            this->updateConstellation();
        });
    }
}

void Constellation::updateConstellation() {
    // Set the new positions of the satellites and update their position in Net Animator.
    for (uint32_t n = 0; n < this->satelliteNodes.GetN(); ++n) {
        GeoCoordinate satPos = this->satelliteMobilityModels[n]->GetGeoPosition();
        // latitude is inverted due to NetAnim growing the y-axis downward
        AnimationInterface::SetConstantPosition(this->satelliteNodes.Get(n), satPos.GetLongitude(), -satPos.GetLatitude());
    }

    // NS_LOG_DEBUG("[->] Simulation at second " << Simulator::Now().GetSeconds());    // Gets the elapsed seconds in the simulation
    
    

}



void Constellation::updateGroundStationLinks() {
    for (uint32_t gsIndex = 0; gsIndex < this->groundStationNodes.GetN(); gsIndex++) {
        Ptr<Node> gs = this->groundStationNodes.Get(gsIndex);
        Ptr<SatConstantPositionMobilityModel> gsMobModel = this->groundStationsMobilityModels[gsIndex];
        bool linkFound = false;                             // keep track of if any GS does not get a link. That is bad

        if (this->GS_existing_link(gs)) {                         // if GS has an existing link, get the connected satellite
            Ptr<Node> connectedSat = this->get_conn_sat(gs);      // get connected satellites mobility model
            uint32_t satMobModelIndex = connectedSat->GetId();      // get the index of the satellite for use in the mobility model vector
            Ptr<SatSGP4MobilityModel> satMobModel = this->satelliteMobilityModels[satMobModelIndex];
            if (this->GS_link_valid(gsMobModel, satMobModel)) {   // check if the link is still valid
                NS_LOG_INFO("[+] Link maintained between GS " << gsIndex << " and satellite index " << Names::FindName(connectedSat));
                linkFound = true;
                continue;                                   // if still valid, continue to next GS
            }
            else {                                          // if no longer valid, break the link before making a new one
                // destroyLink(gs, 1);                      // netDeviceIndex is always 1 for GS's
                NS_LOG_INFO("[+] Link destroyed between GS " << gsIndex << " and satellite index " << Names::FindName(connectedSat));
            }
        }
        for (uint32_t satIndex = 0; satIndex < this->satelliteNodes.GetN(); satIndex++) {
            Ptr<Node> newSat = this->satelliteNodes.Get(satIndex);    // get the mobility model of the satellite, same procedure as above
            uint32_t newSatMobModelIndex = newSat->GetId();
            Ptr<SatSGP4MobilityModel> newSatMobModel = this->satelliteMobilityModels[newSatMobModelIndex];
            if (this->GS_link_valid(gsMobModel, newSatMobModel)) {    // check if a link from GS to sat could work, if yes establish it and move to next GS
                // double distance = gsMobModel->GetDistanceFrom(newSatMobModel);
                // StringValue dataRate = StringValue("20MBps");   // find an actual dataRate later - TODO
                // establishLink(gs, 1, newSat, 5, distance, );    // netdevices are always 1 and 5 from GS to sat
                NS_LOG_INFO("[+] Link established between GS " << gsIndex << " and satellite index " << Names::FindName(newSat));
                linkFound = true;
                break;                                          // valid link found, move on to next GS
            }
        }
        if (linkFound == false) {                               // display that we have a problem if this is true
            NS_LOG_INFO("[+] ERROR: GS " << gsIndex << " DID NOT GET A LINK!");
        }
    }
}


bool Constellation::GS_existing_link(Ptr<Node> GSNode) {   
    if (GSNode->GetDevice(1)->GetChannel()->GetNDevices() == 2) {
        return true;
    }
    return false;
}


Ptr<Node> Constellation::get_conn_sat(Ptr<Node> GSNode) {

    // hold GS netdevice for reference
    Ptr<NetDevice> gsNetDev = GSNode->GetDevice(1);
    for (int n=0; n < 2; n++) {
        Ptr<NetDevice> netdevice = GSNode->GetDevice(1)->GetChannel()->GetDevice(n);
        if (gsNetDev != netdevice) {
            return netdevice->GetNode();
        }
    }
    return GSNode;  // never gets here, but this line makes the compiler happy
}


bool Constellation::GS_link_valid(Ptr<SatConstantPositionMobilityModel> GSMobModel, Ptr<SatSGP4MobilityModel>satMobModel) {
    double distance = GSMobModel->GetDistanceFrom(satMobModel);     // in meters
    // calculate the angle between the GS and sat by extruding a triangle with the earths core in ECEF.
    Vector satPos = satMobModel->GetPosition();             // get ECEF pos for sat
    Vector gsPos = GSMobModel->GetPosition();               // also in ECEF
    double satPosMag = satPos.GetLength();                  // in meters
    double gsPosMag = gsPos.GetLength();                    // in meters

    // NS_LOG_DEBUG("distance: " << distance);
    // NS_LOG_DEBUG("satPosMag: " << satPosMag);
    // NS_LOG_DEBUG("gsPosMag: " << gsPosMag);

    /*
    now we have 3 distances in a triangle. We can now calculate the angle between gs and sat relative to the horizon from
    the gs (also known as the elevation). This can be done using the law of cosines:
    cos(A) = (b²+c²-a²) / (2*b*c), where A is the gsPos and a is the sidelength opposite to A 
    */

    double cosTheta = (pow(gsPosMag, 2) + pow(distance, 2) - pow(satPosMag, 2)) / (2 * gsPosMag * distance);
    // NS_LOG_DEBUG("cosTheta: " << cosTheta);

    double Theta = acos(cosTheta);
    // NS_LOG_DEBUG("Theta: " << Theta);
    
    double elevation = (Theta * 180 / pi ) - 90; // convert to degrees and subtract by 90 in order to get the elevation of the GS antenna
    NS_LOG_DEBUG("Elevation: " << elevation << ", distance: " << distance / 1000 << " km");

    if (elevation > this->minGSElevation && distance < (this->maxGStoSatDistance * 1000)) { // compare in meters
        NS_LOG_DEBUG("link valid: true");
        return true;
    }
    else {
        NS_LOG_DEBUG("link valid: false");
        return false;
    }
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

// =========== Outdated function - remove later ==========
// bool checkGSLink(int gsIndex, std::vector<Ptr<SatConstantPositionMobilityModel>> &groundStationsMobilityModels, NodeContainer &groundStations, std::vector<Ptr<SatSGP4MobilityModel>> &satelliteMobilityModels, NodeContainer &satellites, double maxDistanceM = 3000) {
    
//     Ptr<NetDevice> gsNetDevice = groundStations.Get(gsIndex)->GetDevice(0);
//     int attachedChannelNetDevices = groundStations.Get(gsIndex)->GetDevice(0)->GetChannel()->GetNDevices();

//     if (attachedChannelNetDevices != 2) {
//         return false;
//     }

//     for (int devices = 0; devices < attachedChannelNetDevices; ++devices) {

//         Ptr<NetDevice> attachedChannelDevice = groundStations.Get(gsIndex)->GetDevice(0)->GetChannel()->GetDevice(devices);

//         // Check if the first netdevice of the channel is the satellite one.
//         if (gsNetDevice != attachedChannelDevice) { // If we enter we know that the 'attachedChannelDevice' is a satellite device.
            
//             // Get the satellite index id, as it is created before the ground stations, we can use its id.
//             int satId = attachedChannelDevice->GetNode()->GetId();
            
//             // Check distance between the satellite and the ground station
//             double gsSatDist = groundStationsMobilityModels[gsIndex]->GetDistanceFrom(satelliteMobilityModels[satId]);
//             if ( (gsSatDist/1000) < maxDistanceM) {
//                 // Distance to satellite is in reach to keep the link.
//                 return true;
//             } else {
//                 // Distance to satellite is too great to keep the connection.
//                 return false;
//             }
//         }
//     }

//     return false;
// }
// =========== Outdated function - remove later ==========


int Constellation::establishLink(Ptr<Node> node1, int node1NetDeviceIndex, Ptr<Node> node2, int node2NetDeviceIndex, double distanceM, StringValue channelDataRate, Ipv4Address networkAddress, LinkType linkType) {
    // check if any indexes are out of bounds. GS's must be handled seperately
    if (node1NetDeviceIndex > 5 || node2NetDeviceIndex > 5) {
        return -1;
    }

    Ptr<CsmaChannel> channel = CreateObject<CsmaChannel>();
    double channelDelay = distanceM / c;  // seconds 
    channel->SetAttribute("Delay", TimeValue( Seconds(channelDelay) ));
    channel->SetAttribute("DataRate", channelDataRate);

    // Attach nodes to the same csma channel.
    DynamicCast<CsmaNetDevice>(node1->GetDevice(node1NetDeviceIndex))->Attach(channel);
    DynamicCast<CsmaNetDevice>(node2->GetDevice(node2NetDeviceIndex))->Attach(channel);

    
    Ptr<Ipv4> ipv4_0 = node1->GetObject<Ipv4>();
    Ptr<Ipv4> ipv4_1 = node2->GetObject<Ipv4>();
    

    Ipv4InterfaceAddress satNewAddr = Ipv4InterfaceAddress(Ipv4Address("10.0.0.2"), Ipv4Mask("255.255.255.0"));
    ipv4_0->AddAddress(5, satNewAddr);
    ipv4_0->SetUp(5);
    ipv4_1->SetUp(1);

    Ipv4GlobalRoutingHelper::RecomputeRoutingTables();

    
    // Ipv4AddressHelper addrHelper;
    // addrHelper.SetBase(networkAddress, "255.255.255.0");
    // //addrHelper.NewNetwork();

    // Ptr<CsmaChannel> linkChannel = CreateObject<CsmaChannel>();             // Create a new channel
    // linkChannel->SetAttribute("DataRate", dataRate);                        // set datarate and channel delay (based on speed of light)
    // double c = 299792.4580;             // speed of light in km/s      
    // double delayVal = distanceM / c;   // seconds
    // linkChannel->SetAttribute("Delay", TimeValue(Seconds(delayVal)));

    // DynamicCast<CsmaNetDevice>(node1->GetDevice(node1NetDeviceIndex))->Attach(linkChannel); // attach node's x'th NetDevice to the new channel
    // DynamicCast<CsmaNetDevice>(node2->GetDevice(node2NetDeviceIndex))->Attach(linkChannel);

    // Ipv4InterfaceContainer interface1 = addrHelper.Assign(node1->GetDevice(node1NetDeviceIndex));
    // Ipv4InterfaceContainer interface2 = addrHelper.Assign(node2->GetDevice(node2NetDeviceIndex));

    // NS_LOG_DEBUG("IP1: " << interface1.Get(0).first->GetAddress(1, 0).GetAddress());
    // NS_LOG_DEBUG("IP2: " << interface2.Get(0).first->GetAddress(1, 0).GetAddress());
    
    // node1->GetObject<Ipv4>()->SetUp(node1NetDeviceIndex);   // Enable its IPv4 interface again. Using same index variable indicates  
    // node2->GetObject<Ipv4>()->SetUp(node2NetDeviceIndex);   // a correlation between NetDevice and IPv4 interface. If this is false, fuck...
    // node1->GetDevice(1)->Dispose();
    return 0;       // indicating no error
}


int Constellation::destroyLink(Ptr<Node> node1, int node1NetDeviceIndex, LinkType linkType) {
    if (node1NetDeviceIndex > 5) {  // check if any indexes are out of bounds. GS's netDeviceIndex is always 1.
        return -1;
    }

    if (linkType == SAT_SAT){}


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








/*destroyLink
============================================================== Pseudocode: ==============================================================


----------------------------------------------------------- GS specific -----------------------------------------------------------

For each GS: {              // first for GS's, iterate over each satellite for each GS
    
    If (GS_existing_link()) {                               // GS_existing_link() algorithm described below
        connected_sat = get_conn_sat()                      // get_conn_sat() algorithm described below
        if (GS_link_valid(GS_node, connected_sat)) {        // GS_link_valid() algorithm described below
            Skip this GS - link is still valid (continue keyword?)
        }
        else {
            break_link()
        }
    }
    For each satellite:     // if we get here, the GS needs a new link
        if (GS_link_valid(GS_node, new_sat)) {              // if within range and LOS is fine, make link
            establish_link(GS_node, new_sat)                // establish_link() algorithm described below
            break-inner-loop                                // valid link found, stop searching
        }
}                                                           // when outer loop is done, both GS's have been allowed to get a link. If no link was findable, we panic




GS_existing_link(GS_node) {
    Check if there is a channel connected to the GS's netdevice1, which has 2 connected NetDevices. this means a link exists
    return true/false based on if the link exists
}

get_conn_sat(GS_node) {  // this function is ONLY called if a link between a GS and Sat exists. Therefore safety is ensured

    Get GS_netdevice -> get channel -> get Netdevices connected to channel -> find satellites connected netdevice.
    return ns3::Ptr(satNode);       // remember that we know that the satNetDev has index 5, so no need to worry about that
}

GS_link_valid(GS_node, connected_sat) {     // return a bool telling us whether the link is allowed to exist
    dist = get distance between GS and Sat
    azimuth = get azimuth from GS to Sat

    if (dist < 5000 && azimuth > 5) {
        return true
    }
    else {
        return false
    }

}

----------------------------------------------------------- GS specific -----------------------------------------------------------



----------------------------------------------------------- Both for sats and GS's -----------------------------------------------------------

establish_link() {
    Network guys have this fuction on lock. Make sure to only give it nescessary inputs - no need for entire node_containers or anything like that.
    We know the NetDevIndexes to be 1 for a GS and 5 for a sat.
    Distance can be calculated outside of function if nescessary
    DataRate can be based on distance also? Maybe just hardcoded value or from main somewhere
}

break_link() {
    Same story as above
}

----------------------------------------------------------- Both for sats and GS's -----------------------------------------------------------





----------------------------------------------------------- Sat specific -----------------------------------------------------------

For each satellite:








----------------------------------------------------------- Sat specific -----------------------------------------------------------

*/
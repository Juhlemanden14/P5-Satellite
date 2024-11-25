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

    // Set satellite address helper, which is used to assign them Ipv4Addresses
    this->satAddressHelper.SetBase(Ipv4Address("2.0.0.0"), Ipv4Mask("255.255.255.0"));
}

NodeContainer Constellation::createSatellitesFromTLEAndOrbits(std::string tleDataPath, std::string orbitsDataPath) {
    // Read orbit data
    this->OrbitVector = ReadOrbitFile(orbitsDataPath);
    NS_LOG_INFO("[+] Imported orbit data for " << this->OrbitVector.size() << " orbits");

    // Read TLE data
    std::string TLEAge;
    this->TLEVector = ReadTLEFile(tleDataPath, TLEAge);

    // For each satellite in the orbits, only grab those from the TLE data (filtering out the others)
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

    // Create satellite nodes
    NodeContainer satellites(this->satelliteCount);
    NS_LOG_INFO("[+] " << this->satelliteCount << " satellite nodes have been created");

    // Install the internet stack on the satellites
    InternetStackHelper stackHelper;
    stackHelper.Install(satellites);
    NS_LOG_INFO("[+] Internet stack installed on satellites");

    CsmaHelper csmaHelper;
    std::string formatted_TLE;

    // Loop through each satellite and set them up
    for (uint32_t n = 0; n < this->satelliteCount; ++n) {
        Ipv4AddressHelper tmpAddrHelper;
        tmpAddrHelper.SetBase("2.0.0.0", "255.255.255.0");
        // Create 5 NetDevices for each node, assign them, remove their address and set them down!
        // Ignore device with index 0 (loopback interface)
        for (int i = 1; i <= 5; ++i) {
            Ptr<Node> currentSat = satellites.Get(n);
            Ptr<Ipv4> satIpv4 = currentSat->GetObject<Ipv4>();
            // Use .Install() to get both a CsmaNetDevice and a Channel on a new NetDevice
            Ptr<NetDevice> device = csmaHelper.Install(currentSat).Get(0);
            // This is to utilize the importance of the assign() function, while ensuring that they do not have an IP address afterwards
            // So we just assign an arbitrary address to each satellite, and then remove that address right after
            tmpAddrHelper.Assign(device);
            satIpv4->RemoveAddress(i, 0);
            satIpv4->SetDown(i);
            
            // Delete the channel (free memory)
            device->GetChannel()->Dispose();
            // Make sure that channel pointer is pointing to the null channel, since channel is now destroyed
            Ptr<CsmaNetDevice> currCsmaNetDevice = DynamicCast<CsmaNetDevice>(device);
            Ptr<CsmaChannel> nullChannel = CreateObject<CsmaChannel>();
            currCsmaNetDevice->Attach(nullChannel);
        }

        // Create and aggregate the Satellite SGP4 mobility model to each satellite
        Ptr<SatSGP4MobilityModel> satMobility = CreateObject<SatSGP4MobilityModel>();
        // Format the two lines into a single string for NS-3 compatibility - IT MUST BE line1\nline2 WITH NO SPACES!!!
        formatted_TLE = this->TLEVector[n].line1 + "\n" + this->TLEVector[n].line2; 
        satMobility->SetTleInfo(formatted_TLE);
        // Set the simulation absolute start time in string format.
        satMobility->SetStartDate(TLEAge);
        // Keeping nodes and SatSGP4Mobility models seperated - as NetAnim only works when a nodes aggregated mobility model is a ConstantPositionMobilityModel!
        this->satelliteMobilityModels.emplace_back(satMobility);

        // Give each satellite a name equal to the one specified in the TLE
        Names::Add(this->TLEVector[n].name, satellites.Get(n));

        // IMPORTANT: When enabling this pcap trace, it will create 5 pcap files FOR EACH satellite. If running with many satellites, this might slow down your run substantiually
        // csmaHelper.EnablePcap("scratch/P5-Satellite/out/satellite", satellites, true);
    }

    return satellites;
}


NodeContainer Constellation::createGroundStations(std::vector<GeoCoordinate> groundStationsCoordinates) {
    
    NodeContainer groundStations(this->groundStationCount);

    InternetStackHelper stackHelper;
    stackHelper.Install(groundStations);
    NS_LOG_INFO("[+] Internet stack installed on groundstations");
    
    CsmaHelper csmaHelper;
    // Groundstations have static IP addresses, therefore we simply assign them here and never remove the address from their interface
    Ipv4AddressHelper gsAddressHelper;
    gsAddressHelper.SetBase("1.0.0.0", "255.255.255.0");

    // For each ground station, set up its mobi
    for (size_t n = 0; n < this->groundStationCount; ++n) {
        // Create the single netdevice on each ground station
        NetDeviceContainer gsNetDevice = csmaHelper.Install(groundStations.Get(n));
        // Assign an ip to the ground station but turn the interface down!
        gsAddressHelper.Assign(gsNetDevice);
        groundStations.Get(n)->GetObject<Ipv4>()->SetDown(1);
        // Migrate to a new subnet for future ground stations
        gsAddressHelper.NewNetwork();

        // Finally, attach to nullchannel as its not connected to anything
        Ptr<CsmaNetDevice> currCsmaNetDevice = DynamicCast<CsmaNetDevice>(gsNetDevice.Get(0));
        Ptr<CsmaChannel> nullChannel = CreateObject<CsmaChannel>();
        currCsmaNetDevice->Attach(nullChannel);
        gsNetDevice.Get(0)->GetChannel()->Dispose();

        // GroundStation mobility even though they dont move. The mobility models allows use of methods like .GetDistanceFrom(GS) etc.
        Ptr<SatConstantPositionMobilityModel> GSMobility = CreateObject<SatConstantPositionMobilityModel>();
        GSMobility->SetGeoPosition(groundStationsCoordinates[n]);
        this->groundStationsMobilityModels.emplace_back(GSMobility);
    }
    NS_LOG_DEBUG("[+] SatConstantPositionMobilityModel installed on " << groundStations.GetN() << " ground stations");
    csmaHelper.EnablePcap("scratch/P5-Satellite/out/ground-station", groundStations, true);

    return groundStations;
}
// --------------------------------------------------------



void Constellation::simulationLoop(int totalMinutes, int updateIntervalSeconds) {
    // Run simulation phase at i intervals
    int loops = int(60*totalMinutes / updateIntervalSeconds);
    NS_LOG_DEBUG("[+] Simulation scheduled to loop " << loops << " times");

    // Update constellation for time 0 (before the Simulation starts)
    this->updateConstellation();

    // Update constellation for each interval during the Simulation
    for (int i = 1; i < loops; ++i) {
        Time t = Seconds(i * updateIntervalSeconds);
        Simulator::Schedule(t, [this]() {
            this->updateConstellation();
        });
    }

    // TESTING establishing a link between 2 satellites!
    Ptr<Node> sat0 = Names::Find<Node>("STARLINK-30159");
    Ptr<Node> sat1 = Names::Find<Node>("STARLINK-5748");
    this->establishLink(sat0, 1, sat1, 1, 3000000, StringValue("20MBps"), SAT_SAT);
}

// Function to be scheduled periodically in the ns3 simulator.
void Constellation::updateConstellation() {
    // Set the new positions of the satellites and update their position in NetAnimator.
    for (uint32_t n = 0; n < this->satelliteNodes.GetN(); ++n) {
        GeoCoordinate satPos = this->satelliteMobilityModels[n]->GetGeoPosition();
        // latitude is inverted due to NetAnim growing the y-axis downward
        AnimationInterface::SetConstantPosition(this->satelliteNodes.Get(n), satPos.GetLongitude(), -satPos.GetLatitude());
    }

    this->updateGroundStationLinks();
    // this->updateInterSatelliteLinks();

    // NS_LOG_DEBUG("[->] Simulation at second " << Simulator::Now().GetSeconds());    // Gets the elapsed seconds in the simulation
}



void Constellation::updateGroundStationLinks() { // QUESTION: Technically, we want to break ALL invalid links before we start finding new links, right?!

    for (uint32_t gsIndex = 0; gsIndex < this->groundStationNodes.GetN(); gsIndex++) {

        Ptr<Node> gs = this->groundStationNodes.Get(gsIndex);
        Ptr<SatConstantPositionMobilityModel> gsMobModel = this->groundStationsMobilityModels[gsIndex];
        bool linkFound = false;                 // keep track of if any GS which doesnt get a link. That is bad (but okay)

        if (this->GS_existing_link(gs)) {                         // if GS has an existing link, get the connected satellite
            Ptr<Node> connectedSat = this->GS_get_connected_sat(gs);      // get connected satellites mobility model
            uint32_t satMobModelIndex = connectedSat->GetId();      // get the index of the satellite for use in the mobility model vector
            Ptr<SatSGP4MobilityModel> satMobModel = this->satelliteMobilityModels[satMobModelIndex];

            if (this->GS_is_link_valid(gsMobModel, satMobModel)) {   // check if the link is still valid
                NS_LOG_INFO("[+] Link maintained between GS " << gsIndex << " and satellite index " << Names::FindName(connectedSat));
                linkFound = true;
                continue;                                   // if still valid, continue to next GS
            }
            else {                                          // if no longer valid, break the link before making a new one
                // destroyLink(gs, 1);                      // netDeviceIndex is always 1 for GS's
                NS_LOG_INFO("[+] Link destroyed between GS " << gsIndex << " and satellite index " << Names::FindName(connectedSat));
            }
        }

        // Now loop through each satellite and try to establish a new link
        for (uint32_t satIndex = 0; satIndex < this->satelliteNodes.GetN(); satIndex++) {
            Ptr<Node> newSat = this->satelliteNodes.Get(satIndex);    // get the mobility model of the satellite, same procedure as above
            uint32_t newSatMobModelIndex = newSat->GetId();
            Ptr<SatSGP4MobilityModel> newSatMobModel = this->satelliteMobilityModels[newSatMobModelIndex];

            if (this->GS_is_link_valid(gsMobModel, newSatMobModel)) {    // check if a link from GS to sat could work, if yes establish it and move to next GS
                double distance = gsMobModel->GetDistanceFrom(newSatMobModel);
                StringValue dataRate = StringValue("20MBps");   // TODO: find an appropriate dataRate
                // Establish a GS_SAT link. GS NetDevice is always 1, while SAT NetDevice is always 5 
                establishLink(gs, 1, newSat, 5, distance, dataRate, GS_SAT);
                NS_LOG_INFO("[+] Link established between GS " << gsIndex << " and satellite index " << Names::FindName(newSat));
                linkFound = true;
                // Since valid link is established, move on to next GS
                break;
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

Ptr<Node> Constellation::GS_get_connected_sat(Ptr<Node> GSNode) {
    // hold GS netdevice for reference
    Ptr<NetDevice> gsNetDev = GSNode->GetDevice(1);
    for (int n=0; n < 2; n++) {
        Ptr<NetDevice> netdevice = GSNode->GetDevice(1)->GetChannel()->GetDevice(n);
        if (gsNetDev != netdevice)
            return netdevice->GetNode();
    }
    return GSNode;  // never gets here, but this line makes the compiler happy
}


bool Constellation::GS_is_link_valid(Ptr<SatConstantPositionMobilityModel> GSMobModel, Ptr<SatSGP4MobilityModel>satMobModel) {
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

    if (elevation > this->minGSElevation && distance < (this->maxGStoSatDistance * 1000)) { // compare in meters
        NS_LOG_DEBUG("link valid: true - Elevation: " << elevation << ", distance: " << distance / 1000 << " km");
        return true;
    }
    else {
        return false;
    }
}



void Constellation::establishLink(Ptr<Node> node1, int node1NetDeviceIndex, Ptr<Node> node2, int node2NetDeviceIndex, double distanceM, StringValue channelDataRate, LinkType linkType) {
    // check if any indexes are out of bounds. GS's must be handled seperately
    if (node1NetDeviceIndex > 5 || node2NetDeviceIndex > 5) {
        NS_LOG_ERROR("Index out of bounds in establishLink");
        return;
    }

    Ptr<CsmaChannel> channel = CreateObject<CsmaChannel>();
    double channelDelay = distanceM / c;  // seconds 
    channel->SetAttribute("Delay", TimeValue( Seconds(channelDelay) ));
    channel->SetAttribute("DataRate", channelDataRate);

    // Attach nodes to the same csma channel.
    DynamicCast<CsmaNetDevice>(node1->GetDevice(node1NetDeviceIndex))->Attach(channel);
    DynamicCast<CsmaNetDevice>(node2->GetDevice(node2NetDeviceIndex))->Attach(channel);
    
    Ptr<Ipv4> ipv4_1 = node1->GetObject<Ipv4>();
    Ptr<Ipv4> ipv4_2 = node2->GetObject<Ipv4>();
    ipv4_1->SetUp(node1NetDeviceIndex);
    ipv4_2->SetUp(node2NetDeviceIndex);
    

    if (linkType == GS_SAT) {
        // Get IP address of ground station
        Ipv4Address gsIP = ipv4_1->GetAddress(node1NetDeviceIndex, 0).GetAddress(); // will give us either 10.0.0.1 or 10.0.1.1
        
        // Increment ground stations IP with one, which becomes the new satellite address
        uint8_t ipAsInt[4] = {0, 0, 0, 0};
        gsIP.Serialize(ipAsInt);
        ipAsInt[3]++;
        Ipv4Address satNewIP = gsIP.Deserialize(ipAsInt);

        // Set the IP of the satellite
        Ipv4InterfaceAddress satNewAddr = Ipv4InterfaceAddress(satNewIP, Ipv4Mask("255.255.255.0"));
        ipv4_2->AddAddress(node2NetDeviceIndex, satNewAddr);

        NS_LOG_DEBUG("Groundstation current IP: " << gsIP << " -> New Satellite IP: " << satNewIP);
    }
    else if (linkType == SAT_SAT) {      // linkType = SAT_SAT
        Ipv4InterfaceAddress satNewAddr0 = Ipv4InterfaceAddress(Ipv4Address("2.0.0.1"), Ipv4Mask("255.255.255.0"));
        Ipv4InterfaceAddress satNewAddr1 = Ipv4InterfaceAddress(Ipv4Address("2.0.0.2"), Ipv4Mask("255.255.255.0"));
        ipv4_1->AddAddress(node1NetDeviceIndex, satNewAddr0);
        ipv4_2->AddAddress(node2NetDeviceIndex, satNewAddr1);

        // // ALTERNATIVE WAY TO ASSIGN IP ADDRESSES TO SATELLITES
        // NetDeviceContainer tmp = NetDeviceContainer(node1->GetDevice(node1NetDeviceIndex), node2->GetDevice(node2NetDeviceIndex));
        // this->satAddressHelper.Assign(tmp);
        // this->satAddressHelper.NewNetwork();
    }

    Ipv4GlobalRoutingHelper::RecomputeRoutingTables();
}


void Constellation::destroyLink(Ptr<Node> node1, int node1NetDeviceIndex, LinkType linkType) {
    if (node1NetDeviceIndex > 5) {  // check if any indexes are out of bounds. GS's netDeviceIndex is always 1.
        NS_LOG_ERROR("Index out of bounds in destroyLink");
        return;
    }
    NS_LOG_ERROR("destroyLink() method not properly implemented yet!");

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

    return;   // indicating no error
}


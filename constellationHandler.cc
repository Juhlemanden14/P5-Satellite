#include "constellationHandler.h"

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
// #include "ns3/csma-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("P5-Constellation-Handler");

Constellation::Constellation(uint32_t satCount, std::string tleDataPath, std::string orbitsDataPath, uint32_t gsCount, std::vector<GeoCoordinate> groundStationsCoordinates, DataRate gsInputDataRate, DataRate satInputDataRate, double gsSatErrorRate, double satSatErrorRate, TimeValue linkAcquisitionSec) {

    // In the simulation, this Ipv4AddressGenerator keeps track of all allocated IPv4 addresses. If we want to remove an address and later allocate it to another Ipv4Interface, this generates an error! Therefore we enable "TestMode", which means it *does not* check if an new addresses have previously been allocated. Basicallly, enabling TestMode mimics the real world the most, as no one can keep a global record on which IP addresses have been assigned previously in history
    Ipv4AddressGenerator::TestMode();
    
    // TODO: Change this
    // Config::SetDefault("ns3::PointToPointNetDevice::DataRate", DataRateValue(DataRate("100GB/s")));
    // Config::SetDefault("ns3::PointToPointNetDevice::DataRate", DataRateValue(satInputDataRate));
    

    this->satelliteCount = satCount;
    this->groundStationCount = gsCount;

    // Set the eSAT_SAT and GS_SAT DataRates
    this->gsToSatDataRate = gsInputDataRate;
    this->satToSatDataRate = satInputDataRate;
    // BitErrorRates!
    this->gsSatPacketLossRate = gsSatErrorRate;
    this->satSatPacketLossRate = satSatErrorRate;
    // Link acquisition time!
    this->linkAcquisitionTime = linkAcquisitionSec;
    
    // Reserve memory for the vector.
    this->availableSatNetDevices.reserve(this->satelliteCount);
    

    // TESTING PURPOSES 
    // for (uint32_t n = 0; n < this->satelliteCount; ++n) {
    //     for (uint32_t j = 0; j < this->availableSatNetDevices[n].size(); ++j) {
    //         NS_LOG_UNCOND("Val: " << availableSatNetDevices[n][j]);
    //     }
    //     NS_LOG_UNCOND("\n");
    // }

    // Create the satellites in the constellation.
    this->satelliteNodes = this->createSatellitesFromTLEAndOrbits(tleDataPath, orbitsDataPath);

    // Create the ground stations in the constellation.
    this->groundStationNodes = this->createGroundStations(groundStationsCoordinates);
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
    if ( (this->satelliteCount == 0) || (this->satelliteCount > this->TLEVector.size()) ) {
        this->satelliteCount = this->TLEVector.size(); // Change this if you want to include all satellites from TLE data!   
    }

    // populate the available netdevices since they are all available at this moment
    for (uint32_t n = 0; n < this->satelliteCount; ++n) { 
        std::vector<int> availableNetDevs = {1, 2, 3, 4};
        this->availableSatNetDevices.emplace_back(availableNetDevs);
    }



    // Create satellite nodes
    NodeContainer satellites(this->satelliteCount);
    NS_LOG_INFO("[+] " << this->satelliteCount << " satellite nodes have been created");

    // Install the internet stack on the satellites
    InternetStackHelper stackHelper;
    stackHelper.Install(satellites);
    NS_LOG_INFO("[+] Internet stack installed on satellites");

    PointToPointHelper p2pHelper;
    
    // Setup the error model
    Ptr<RateErrorModel> error_model = CreateObject<RateErrorModel>();
    error_model->SetUnit(RateErrorModel::ERROR_UNIT_BIT);
    error_model->SetRate(satSatPacketLossRate);
    p2pHelper.SetDeviceAttribute("ReceiveErrorModel", PointerValue(error_model));
    
    std::string formatted_TLE;

    Ptr<Node> dummyNode = CreateObject<Node>();
    // Give it a constant mobility model to avoid warning in terminal
    AnimationInterface::SetConstantPosition(dummyNode, 180, -90);

    // Loop through each satellite and set them up
    for (uint32_t n = 0; n < this->satelliteCount; ++n) {
        Ipv4AddressHelper tmpAddrHelper;
        tmpAddrHelper.SetBase("2.0.0.0", "255.255.255.0");
        // Create 5 NetDevices for each node, assign them, remove their address and set them down!
        // Ignore device with index 0 (loopback interface)
        for (int i = 1; i <= 5; ++i) {
            Ptr<Node> currentSat = satellites.Get(n);
            Ptr<Ipv4> satIpv4 = currentSat->GetObject<Ipv4>();
            // Use .Install() to get both a PointToPointNetDevice and a Channel on a new NetDevice
            
            Ptr<NetDevice> device = p2pHelper.Install(NodeContainer(currentSat, dummyNode)).Get(0);
            

            // This is to utilize the importance of the assign() function, while ensuring that they do not have an IP address afterwards
            // So we just assign an arbitrary address to each satellite, and then remove that address right after
            tmpAddrHelper.Assign(device);
            satIpv4->RemoveAddress(i, 0);
            satIpv4->SetDown(i);
            
            // Delete the channel (free memory)
            device->GetChannel()->Dispose();
            // Make sure that channel pointer is pointing to the null channel, since channel is now destroyed
            Ptr<PointToPointNetDevice> currP2pNetDevice = DynamicCast<PointToPointNetDevice>(device);
            Ptr<PointToPointChannel> nullChannel = CreateObject<PointToPointChannel>();
            currP2pNetDevice->Attach(nullChannel);
            
            
            // If NetDevice is connected to either GS or SAT, set the appropriate DataRate
            if (i == 5)
                currP2pNetDevice->SetDataRate(this->gsToSatDataRate);
            else
                currP2pNetDevice->SetDataRate(this->satToSatDataRate);
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
        // p2pHelper.EnablePcap("scratch/P5-Satellite/out/satellite", satellites, true);
    }

    return satellites;
}

NodeContainer Constellation::createGroundStations(std::vector<GeoCoordinate> groundStationsCoordinates) {
    
    NodeContainer groundStations(this->groundStationCount);

    InternetStackHelper stackHelper;
    stackHelper.Install(groundStations);
    NS_LOG_INFO("[+] Internet stack installed on groundstations");
    
    PointToPointHelper p2pHelper;

    // Setup the error model
    Ptr<RateErrorModel> error_model = CreateObject<RateErrorModel>();
    error_model->SetUnit(RateErrorModel::ERROR_UNIT_BIT);
    error_model->SetRate(gsSatPacketLossRate);
    p2pHelper.SetDeviceAttribute("ReceiveErrorModel", PointerValue(error_model));

    // Groundstations have static IP addresses, therefore we simply assign them here and never remove the address from their interface
    Ipv4AddressHelper gsAddressHelper;
    gsAddressHelper.SetBase("1.0.0.0", "255.255.255.0");

    Ptr<Node> dummyNode = CreateObject<Node>();
    // Give it a constant mobility model to avoid warning in terminal
    AnimationInterface::SetConstantPosition(dummyNode, -180, 90);

    // For each ground station, set up its mobility
    for (size_t n = 0; n < this->groundStationCount; ++n) {
        // Create the single netdevice on each ground station
        NetDeviceContainer gsNetDevice = p2pHelper.Install(NodeContainer(groundStations.Get(n), dummyNode) ).Get(0);
        

        // Assign an ip to the ground station but turn the interface down!
        gsAddressHelper.Assign(gsNetDevice);
        groundStations.Get(n)->GetObject<Ipv4>()->SetDown(1);
        // Migrate to a new subnet for future ground stations
        gsAddressHelper.NewNetwork();

        // Finally, attach to nullchannel as its not connected to anything
        Ptr<PointToPointNetDevice> currP2PNetDevice = DynamicCast<PointToPointNetDevice>(gsNetDevice.Get(0));
        Ptr<PointToPointChannel> nullChannel = CreateObject<PointToPointChannel>();
        currP2PNetDevice->Attach(nullChannel);
        gsNetDevice.Get(0)->GetChannel()->Dispose();
        // Set the DataRate!
        currP2PNetDevice->SetDataRate(this->gsToSatDataRate);

        // GroundStation mobility even though they dont move. The mobility models allows use of methods like .GetDistanceFrom(GS) etc.
        Ptr<SatConstantPositionMobilityModel> GSMobility = CreateObject<SatConstantPositionMobilityModel>();
        GSMobility->SetGeoPosition(groundStationsCoordinates[n]);
        this->groundStationsMobilityModels.emplace_back(GSMobility);

        Names::Add("Groundstation " + std::to_string(n), groundStations.Get(n));
    }
    NS_LOG_DEBUG("[+] SatConstantPositionMobilityModel installed on " << groundStations.GetN() << " ground stations");
    p2pHelper.EnablePcap("scratch/P5-Satellite/out/ground-station", groundStations, true);

    return groundStations;
}


void Constellation::initializeSatIntraLinks() {
    // A counter used to keep track of the satellites ID
    uint32_t counter = 0;
    
    // For each orbit
    for (size_t i = 0; i < this->OrbitVector.size(); ++i) {
        NS_LOG_DEBUG("ORBIT " << i+1 << "/" << this->OrbitVector.size());
        Orbit orbit = OrbitVector[i];
        
        // For each satellite in each orbit
        for (size_t j = 0; j < orbit.satellites.size(); ++j) {
            NS_LOG_DEBUG("  SAT " << j+1 << "/" << orbit.satellites.size());

            Ptr<Node> satellite = Names::Find<Node>(orbit.satellites[j]);
            Ptr<Node> nextSatellite;

            counter++;

            // If very last satellite in the orbit.
            if (j == orbit.satellites.size() - 1) {
                nextSatellite = Names::Find<Node>(orbit.satellites[0]);
            } else {
                if (counter == this->satelliteCount){
                    return;     // if we get here, return early as all sats have been checked.
                }
                nextSatellite = Names::Find<Node>(orbit.satellites[j+1]);
            }

            // Extract mobility models for the nodes.
            Ptr<SatSGP4MobilityModel> satMob = this->satelliteMobilityModels[satellite->GetId()];
            Ptr<SatSGP4MobilityModel> nextSatMob = this->satelliteMobilityModels[nextSatellite->GetId()];
            
            // For each combination of netdevices, check of link can be established
            for (size_t n1 = 1; n1 <= 4; n1++){         // n1 for netDeviceIndex1
                for (size_t n2 = 1; n2 <= 4; n2++) {     // n2 for netDeviceIndex2
                    // skip impossible situations
                    if (n1 == n2) {
                        continue;
                    }
                    // If either if the NetDevices already have a connection, skip
                    if ( hasExistingLink(satellite, n1) || hasExistingLink(nextSatellite, n2) ) {
                        continue;
                    }
                    // Is the link possible according to angles, establish a connection
                    if (satIsLinkValid(satMob, n1, nextSatMob, n2)) {
                        double distance = satMob->GetDistanceFrom(nextSatMob);
                        this->establishLink(satellite, n1, nextSatellite, n2, distance, SAT_SAT);

                        // remove netDevices from availableNetDevs vector using satIndexes and the netdeviceIndex - 1
                        int satIndex = satellite->GetId();
                        for (uint32_t i = 0; i < this->availableSatNetDevices[satIndex].size(); i++) {   // loop through to find the relevant index to remove at
                            if (this->availableSatNetDevices[satIndex][i] == (int)n1) {
                                this->availableSatNetDevices[satIndex].erase(this->availableSatNetDevices[satIndex].begin() + i);
                            }
                        }
                        // same procedure for both satellites
                        int nextSatIndex = nextSatellite->GetId();
                        for (uint32_t i = 0; i < this->availableSatNetDevices[nextSatIndex].size(); i++) {
                            if (this->availableSatNetDevices[nextSatIndex][i] == (int)n2) {
                                this->availableSatNetDevices[nextSatIndex].erase(this->availableSatNetDevices[nextSatIndex].begin() + i);
                            }
                        }
                    }
                }

            }
        }
    }
    NS_LOG_DEBUG("[!] INIT SAT LINKS DONE");

    // for plane in orbits:         // go through each orbital plane and try to establish valid links
    //     for sat in plane:
    //         get next sat in line. If at last index, get 0'th index as the next, completing the ring-loop
    //         if (satIsLinkValid(currSatMobModel, 1, nextSatMobModel, 3)) {       // pseudocode for method below
    //             connect sat netDev1 with next_sat netDev3 using establishLink()
    //             continue        // go to next sat in the orbit
    //         }
    //         else {
    //             add sat and netDev1 to pairs queue
    //         }
}

// --------------------------------------------------------



void Constellation::scheduleSimulation(int totalMinutes, int updateIntervalSeconds) {
    // Run simulation phase at i intervals
    int loops = int(60*totalMinutes / updateIntervalSeconds);
    NS_LOG_DEBUG("[+] Simulation scheduled to loop " << loops << " times");

    // // TESTING: establishing a link between 2 satellites!
    // Ptr<Node> sat0 = Names::Find<Node>("STARLINK-30159");
    // Ptr<Node> sat1 = Names::Find<Node>("STARLINK-5748");
    // this->establishLink(sat0, 1, sat1, 1, 3000000, SAT_SAT);
    // Ptr<Node> sat6 = Names::Find<Node>("STARLINK-5366");
    // Ptr<Node> sat7 = Names::Find<Node>("STARLINK-30159");
    // this->establishLink(sat6, 2, sat7, 2, 3000000, SAT_SAT);

    NS_LOG_UNCOND("Node 18 -> " << Names::FindName(this->satelliteNodes.Get(18)));


    this->initializeSatIntraLinks();
    NS_LOG_INFO("[+] Initialized intra-plane links!");
    // Update constellation for time 0 (before the Simulation starts)
    this->updateConstellation();

    // Update constellation for each interval during the Simulation
    for (int i = 1; i < loops; ++i) {
        Time t = Seconds(i * updateIntervalSeconds);
        Simulator::Schedule(t, [this]() {

            // TODO: save the current route before breaking any links.
            this->saveCompleteRoute(this->groundStationNodes.Get(0), this->groundStationNodes.Get(1));
            
            this->updateConstellation();

            // TODO: clear current route before next time
            this->currRoute.clear();
        });
    }
}

void Constellation::updateConstellation() {
    NS_LOG_INFO("\n\x1b[32;1m[+]\x1b[37m <" << Simulator::Now().GetSeconds() << "s> UPDATING CONSTELLATION\x1b[0m");

    // Set the new positions of the satellites and update their position in NetAnimator.
    for (uint32_t n = 0; n < this->satelliteNodes.GetN(); ++n) {
        GeoCoordinate satPos = this->satelliteMobilityModels[n]->GetGeoPosition();
        // latitude is inverted due to NetAnim growing the y-axis downward
        AnimationInterface::SetConstantPosition(this->satelliteNodes.Get(n), satPos.GetLongitude(), -satPos.GetLatitude());
    }

    this->updateGroundStationLinks();
    this->updateSatelliteLinks();


    // // TESTING
    // Ptr<Node> testSat = Names::Find<Node>("STARLINK-30146");
    // Ptr<Ipv4> ipv4 = testSat->GetObject<Ipv4>();
    // NS_LOG_DEBUG("STARLINK-30146 DEBUGGING");
    // NS_LOG_DEBUG("Number of interfaces " << ipv4->GetNInterfaces());
    // for (uint32_t i = 0; i < ipv4->GetNInterfaces() - 1; i++) {
    //     for (uint32_t j = 0; j < this->availableSatNetDevices[ testSat->GetId() ].size(); j++) {
    //         if (!(this->availableSatNetDevices[testSat->GetId()][j] == (int)i )) {
    //             NS_LOG_DEBUG("Address " << i << " " << ipv4->GetAddress(i, 0).GetAddress());
    //         }
    //     }
    // }
    // Ptr<Node> otherTestSat = Names::Find<Node>("STARLINK-5714");
    // this->
    //


    // At the end of each round, recompute the routing tables such that new links can be used, and broken ones are forgotten
    // NS-3 specifies that one should call PopulateRoutingTables() as the first thing, and only subsequently call RecomputeRoutingTables()
    // This does not seem to be a problem, so we ONLY use RecomputeRoutingTables without calling PopulateRoutingTables first!
    NS_LOG_DEBUG("Computing tables");
    Ipv4GlobalRoutingHelper::RecomputeRoutingTables();
    NS_LOG_INFO("[+] Routing tables computed");
    // POPULATE All satellites ARP tables :). THIS IS ONLY VALID FOR CSMA LINKS, NOT FOR POINT TO POINT
    // NeighborCacheHelper neighborCacheHelper;
    // neighborCacheHelper.PopulateNeighborCache();
    // NS_LOG_DEBUG("DONE with cache");
}


void Constellation::updateGroundStationLinks() {
    // QUESTION: Technically, we want to break ALL invalid links before we start finding new links, right?!

    for (uint32_t gsIndex = 0; gsIndex < this->groundStationNodes.GetN(); gsIndex++) {

        Ptr<Node> gs = this->groundStationNodes.Get(gsIndex);
        Ptr<SatConstantPositionMobilityModel> gsMobModel = this->groundStationsMobilityModels[gsIndex];
        bool linkFound = false;                 // keep track of if any GS which doesnt get a link. That is bad (but okay)

        if (this->hasExistingLink(gs, 1)) {                         // if GS has an existing link, get the connected satellite
            Ptr<Node> connectedSat = this->getConnectedNetDev(gs, 1)->GetNode();      // get connected satellites mobility model
            uint32_t satMobModelIndex = connectedSat->GetId();      // get the index of the satellite for use in the mobility model vector
            Ptr<SatSGP4MobilityModel> satMobModel = this->satelliteMobilityModels[satMobModelIndex];

            if (this->gsIsLinkValid(gsMobModel, satMobModel)) {   // check if the link is still valid
                NS_LOG_DEBUG("[+] Link maintained between GS " << gsIndex << " and satellite index " << Names::FindName(connectedSat));
                linkFound = true;
                continue;                                   // if still valid, continue to next GS
            }
            else {                                          // if no longer valid, break the link before making a new one
                destroyLink(gs, 1, connectedSat, 5, GS_SAT);    // netDeviceIndex is always 1 for GS's, and 5 for sats
                NS_LOG_DEBUG("[+] Link destroyed between GS " << gsIndex << " and satellite index " << Names::FindName(connectedSat));
            }
        }

        // Now loop through each satellite and try to establish a new link
        for (uint32_t satIndex = 0; satIndex < this->satelliteNodes.GetN(); satIndex++) {
            Ptr<Node> newSat = this->satelliteNodes.Get(satIndex);    // get the mobility model of the satellite, same procedure as above
            uint32_t newSatMobModelIndex = newSat->GetId();
            Ptr<SatSGP4MobilityModel> newSatMobModel = this->satelliteMobilityModels[newSatMobModelIndex];

            if (this->gsIsLinkValid(gsMobModel, newSatMobModel)) {    // check if a link from GS to sat could work, if yes establish it and move to next GS
                double distance = gsMobModel->GetDistanceFrom(newSatMobModel);
                // Establish a GS_SAT link. GS NetDevice is always 1, while SAT NetDevice is always 5
                establishLink(gs, 1, newSat, 5, distance, GS_SAT);
                NS_LOG_DEBUG("Link established between GS " << gsIndex << " and satellite index " << Names::FindName(newSat));
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

bool Constellation::gsIsLinkValid(Ptr<SatConstantPositionMobilityModel> GSMobModel, Ptr<SatSGP4MobilityModel>satMobModel) {
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
        // NS_LOG_DEBUG("link valid: true - Elevation: " << elevation << ", distance: " << distance / 1000 << " km");
        return true;
    }
    else {
        // NS_LOG_DEBUG("link valid: false - Elevation: " << elevation << ", distance: " << distance / 1000 << " km");
        return false;
    }
}





bool Constellation::hasExistingLink(Ptr<Node> node, int netDevIndex) {
    if (node->GetDevice(netDevIndex)->GetChannel()->GetNDevices() == 2) {
        return true;
    }
    return false;
}


Ptr<NetDevice> Constellation::getConnectedNetDev(Ptr<Node> GSNode, int netDevIndex) {
    // hold GS netdevice for reference
    Ptr<NetDevice> localNetDevice = GSNode->GetDevice(netDevIndex);
    for (int n=0; n < 2; n++) {
        // Grab one of the NetDevices on the Channel
        Ptr<NetDevice> foreignNetDevice = localNetDevice->GetChannel()->GetDevice(n);
        // If that NetDevice is NOT your own, it must be the other node
        if (localNetDevice != foreignNetDevice) {
            return foreignNetDevice;
        }
    }
    return localNetDevice;  // never gets here, but this line makes the compiler happy
}


void Constellation::establishLink(Ptr<Node> node1, int node1NetDeviceIndex, Ptr<Node> node2, int node2NetDeviceIndex, double distanceM, LinkType linkType) {
    // check if any indexes are out of bounds. GS's must be handled seperately
    if (node1NetDeviceIndex > 5 || node2NetDeviceIndex > 5) {
        NS_LOG_ERROR("Index out of bounds in establishLink");
        return;
    }

    Ptr<PointToPointChannel> channel = CreateObject<PointToPointChannel>();
    double channelDelay = distanceM / c;  // seconds
    channel->SetAttribute("Delay", TimeValue( Seconds(channelDelay) ));
    
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

        //channel->SetAttribute("DataRate", this->gsToSatDataRate);
    }
    else if (linkType == SAT_SAT) { // Is link is being  established
        std::pair<Ipv4Address, Ipv4Address> addressPair = getLinkAddressPair();

        Ipv4InterfaceAddress satNewAddr0 = Ipv4InterfaceAddress(addressPair.first, Ipv4Mask("255.255.255.0"));
        Ipv4InterfaceAddress satNewAddr1 = Ipv4InterfaceAddress(addressPair.second, Ipv4Mask("255.255.255.0"));
        ipv4_1->AddAddress(node1NetDeviceIndex, satNewAddr0);
        ipv4_2->AddAddress(node2NetDeviceIndex, satNewAddr1);

        NS_LOG_DEBUG("      " << Names::FindName(node1) << " IP: " << satNewAddr0.GetAddress() << " <--> "  << Names::FindName(node2) << " IP: " << satNewAddr1.GetAddress());


        // // ALTERNATIVE WAY TO ASSIGN IP ADDRESSES TO SATELLITES
        // NetDeviceContainer tmp = NetDeviceContainer(node1->GetDevice(node1NetDeviceIndex), node2->GetDevice(node2NetDeviceIndex));
        // this->satAddressHelper.Assign(tmp);
        // this->satAddressHelper.NewNetwork();

        //channel->SetAttribute("DataRate", this->satToSatDataRate);
    }
    // Dispose of the null channels!
    DynamicCast<PointToPointNetDevice>(node1->GetDevice(node1NetDeviceIndex))->GetChannel()->Dispose();
    DynamicCast<PointToPointNetDevice>(node2->GetDevice(node2NetDeviceIndex))->GetChannel()->Dispose();

    // Attach nodes to the same P2P channel.
    DynamicCast<PointToPointNetDevice>(node1->GetDevice(node1NetDeviceIndex))->Attach(channel);
    DynamicCast<PointToPointNetDevice>(node2->GetDevice(node2NetDeviceIndex))->Attach(channel);
}


void Constellation::destroyLink(Ptr<Node> node1, int node1NetDeviceIndex, Ptr<Node> node2, int node2NetDeviceIndex, LinkType linkType) {
    // In this function we assume that there is a link to actually destroy!

    if (node1NetDeviceIndex > 5) {  // check if any indexes are out of bounds. GS's netDeviceIndex is always 1.
        NS_LOG_ERROR("Index out of bounds in destroyLink");
        return;
    }

    // check if the route has been broken
    bool broken = false;
    std::pair<Ptr<Node>, int> node1Pair = {node1, node1NetDeviceIndex};
    std::pair<Ptr<Node>, int> node2Pair = {node2, node2NetDeviceIndex};
    for (const auto& pair : this->currRoute) {
        if ( (pair.first == node1Pair.first && pair.second == node1Pair.second) || (pair.first == node2Pair.first && pair.second == node2Pair.second) ) {
            broken = true;
            NS_LOG_INFO("ROUTE BROKEN BETWEEN NODES " << Names::FindName(node1) << " - " << Names::FindName(node2) << "");
            break;
        }
    }

    // save break to file
    if (broken) {
        // Get the current simulator time in seconds
        double currentTime = Simulator::Now().GetSeconds();

        // Construct the file name using the satCount
        std::ostringstream fileName;
        fileName << "scratch/P5-Satellite/out/link_break_times_satCount" << this->satelliteCount << ".log";

        // Open the file in append mode
        std::ofstream outFile(fileName.str(), std::ios::app);
        if (outFile.is_open()) {
            // Write the current time to the file
            outFile << currentTime << "," << std::endl;
            outFile.close();
        } else {
            NS_LOG_ERROR("Failed to open file: " << fileName.str());
        }
    }


    // Get neccessary pointers for the local node
    Ptr<NetDevice> netDev_1 = node1->GetDevice(node1NetDeviceIndex);
    Ptr<Ipv4> ipv4_1 = node1->GetObject<Ipv4>();
    // Get necessary pointers for the foreign node 
    Ptr<NetDevice> netDev_2 = node2->GetDevice(node2NetDeviceIndex);
    Ptr<Ipv4> ipv4_2 = node2->GetObject<Ipv4>();

    // TODO, if shit fucks up, check that Ipv4Interfaces and NetDeviceIndex's actually match. Maybe add more code
    ipv4_1->SetDown( node1NetDeviceIndex );
    ipv4_2->SetDown( node2NetDeviceIndex );

    // Get the channel and dispose of it, freeing the memory, allegedly.
    node1->GetDevice(node1NetDeviceIndex)->GetChannel()->Dispose();
    
    // create new local NullChannels for each netDevice
    Ptr<PointToPointChannel> nullChannel1 = CreateObject<PointToPointChannel>();
    Ptr<PointToPointChannel> nullChannel2 = CreateObject<PointToPointChannel>();
    DynamicCast<PointToPointNetDevice>( node1->GetDevice(node1NetDeviceIndex) )->Attach(nullChannel1);
    DynamicCast<PointToPointNetDevice>( node2->GetDevice(node2NetDeviceIndex) )->Attach(nullChannel2);

    // If GS_SAT, only remoove the SAT IP. If SAT_SAT, remove both their 
    if (linkType == GS_SAT) {
        ipv4_2->RemoveAddress(node2NetDeviceIndex, 0);  // TODO: yet again, we assume that netdevice Indexes are same as IPv4 indexes
    } else if (linkType == SAT_SAT) {
        releaseLinkAddressPair(ipv4_1->GetAddress(node1NetDeviceIndex, 0).GetAddress(), ipv4_2->GetAddress(node2NetDeviceIndex, 0).GetAddress());
        ipv4_1->RemoveAddress(node1NetDeviceIndex, 0);  // TODO: yet again, we assume that netdevice Indexes are same as IPv4 indexes
        ipv4_2->RemoveAddress(node2NetDeviceIndex, 0);  // TODO: yet again, we assume that
    }

    return;
}

// Get an address subnet pair for an inter satellite link.
std::pair<Ipv4Address, Ipv4Address> Constellation::getLinkAddressPair() {

    // Link address pair to return.
    std::pair<Ipv4Address, Ipv4Address> linkPair;

    // If this is zero, we generate a new address pair.
    if (this->linkAddressProvider.size() == 0) {

        Ipv4Address address = Ipv4Address("0.0.0.1");
        uint8_t addr[4] = {0, 0, 0, 1};
        address.Serialize(addr);

        // Set the new address, starting at 2.0.0.1
        // Similiar to SetBase(Ipv4Address("2.0.0.0"), Ipv4Mask("255.255.255.0"));
        addr[0] = 2+floor(this->linkSubnetCounter/(256*256));
        addr[1] = floor(this->linkSubnetCounter/256);
        addr[2] = 1+this->linkSubnetCounter;

        // Increment the subnet counter so we do not reuse a subnet.
        // TODO, should this also the the case with addr[3] and addr[4]???
        if (addr[2] == 253) {   // Skip the .254 and .255 subnets as they are broadcast addresses
            this->linkSubnetCounter += 3; 
        } else {
            this->linkSubnetCounter++;
        }
        
        // Convert the address bytes to an Ipv4Address
        linkPair.first = address.Deserialize(addr);
        addr[3]++;
        linkPair.second = address.Deserialize(addr);

        return linkPair;
    }

    // Logical else 

    // Get link pair from the linkAddressProvider, and dequeue that link pair.
    linkPair = linkAddressProvider.front();
    linkAddressProvider.pop();

    return linkPair;
}

void Constellation::releaseLinkAddressPair(Ipv4Address linkAddress_0, Ipv4Address linkAddress_1) {
    // Construct link address pair
    std::pair<Ipv4Address, Ipv4Address> linkAddressPair;
    linkAddressPair.first = linkAddress_0;
    linkAddressPair.second = linkAddress_1;

    // Reclaim the link address.
    this->linkAddressProvider.push(linkAddressPair);
}




bool Constellation::satIsLinkValid(Ptr<SatSGP4MobilityModel> mobilityModel, int netDeviceIndex, Ptr<SatSGP4MobilityModel> connMobilityModel, int connNetDeviceIndex) {
    
    // Check that the satellite is in range.
    double distance = mobilityModel->GetDistanceFrom(connMobilityModel);
    if ( distance > (this->maxSatToSatDistance * 1000) ) {
        return false;       // return early if we get here
    }
    
    // get angle from sat1 to sat2 and the other way around
    std::pair<double, double> angles = getAngleFromSatPair(mobilityModel, connMobilityModel);   // angles.first is sat1 to sat2. Angles.second is the otwer way around
    if (angles.first < -45) {
        angles.first = abs(angles.first);
        angles.first = 360 - angles.first;
    }

    if (angles.second < -45) {
        angles.second = abs(angles.second);
        angles.second = 360 - angles.second;
    }
    // NS_LOG_DEBUG("PAIR: " << angles.first << " | " << angles.second);
    
    // first check for sat1. Check negative cases and stop early if true
    if (!(angles.first >= NetDeviceAngles[netDeviceIndex - 1].minAngle) || !(angles.first < NetDeviceAngles[netDeviceIndex - 1].maxAngle)) {
        return false;        // return early if we get here
    }

    // then check for sat2
    if (!(angles.second >= NetDeviceAngles[connNetDeviceIndex - 1].minAngle) || !(angles.second < NetDeviceAngles[connNetDeviceIndex - 1].maxAngle)) {
        return false;        // return early if we get here
    }

    // if both checks passed, the link is valid
    return true;
}



void Constellation::updateSatelliteLinks() {

    // keep track of amount of broken sat-sat links, maintained sat-sat links and established sat-sat links in each simulation round
    int linksBroken = 0;
    int linksMaintained = 0;
    int linksEstablished = 0;

    NS_LOG_DEBUG("[+] updateSatelliteLinks() -----------");
    // Iterate over each satellite
    for (uint32_t i = 0; i < this->satelliteCount; i++) {

        // Get the node and sat mobility model
        Ptr<Node> satNode = this->satelliteNodes.Get(i);
        Ptr<SatSGP4MobilityModel> satMobModel = this->satelliteMobilityModels[i];
        
        // Iterate over each satellites NetDevices' connection. Maintain it, or BREAK it if needed
        for (int netDevIndex = 1; netDevIndex <= 4; netDevIndex++) {

            // Check if there is an already existing link
            if (this->hasExistingLink(satNode, netDevIndex)) {
                Ptr<NetDevice> connNetDev = this->getConnectedNetDev(satNode, netDevIndex);
                Ptr<Node> connSatNode = connNetDev->GetNode();
                Ptr<SatSGP4MobilityModel> connSatMobModel = this->satelliteMobilityModels[connSatNode->GetId()];
                
                // actual warcrime but it works so keep it or live with the consequences of your actions, bling bling brr brr skyat
                uint32_t ipv4InterfaceIndex = connSatNode->GetObject<Ipv4>()->GetInterfaceForDevice(connNetDev);
                uint32_t connNetDevIndex = ipv4InterfaceIndex;      // We know that IPv4 and netDev index are always the same
                
                if (this->satIsLinkValid(satMobModel, netDevIndex, connSatMobModel, connNetDevIndex)) {
                    // NS_LOG_DEBUG("Link maintained satellites <" << Names::FindName(satNode) << "> - <" << Names::FindName(connSatNode) << ">");
                    linksMaintained++;
                    continue;       // move on ot next netdevice for this satellite
                }
                else {
                    NS_LOG_DEBUG("Link BROKEN between satellites <" << Names::FindName(satNode) << "> - <" << Names::FindName(connSatNode) << ">");
                    destroyLink(satNode, netDevIndex, connSatNode, connNetDevIndex, SAT_SAT);
                    NS_LOG_DEBUG("  used netDevs: " << netDevIndex << ", " << connNetDevIndex);
                    NS_LOG_DEBUG("  First satellite " << satNode->GetId() << " second satellite " << connSatNode->GetId());

                    linksBroken++;

                    // remove netDevices from availableNetDevs vector using satIndexes and the netdeviceIndex - 1
                    int satIndex = satNode->GetId();
                    this->availableSatNetDevices[satIndex].emplace_back(netDevIndex);

                    // same procedure for both satellites
                    int connSatIndex = connSatNode->GetId();
                    this->availableSatNetDevices[connSatIndex].emplace_back(connNetDevIndex);


                    // NS_LOG_DEBUG("new available list for sat1: ");
                    // for (uint32_t n = 0; n < this->availableSatNetDevices[satIndex].size(); n++) {
                    //     NS_LOG_DEBUG(n << " = " << this->availableSatNetDevices[satIndex][n]);
                    // }

                    // NS_LOG_DEBUG("new available list for sat2: ");
                    // for (uint32_t n = 0; n < this->availableSatNetDevices[connSatIndex].size(); n++) {
                    //     NS_LOG_DEBUG(n << " = " << this->availableSatNetDevices[connSatIndex][n]);
                    // }

                }

            }
        }
    }
    

    // keep track of which indecies to remove from the sat in the outermost loop. 
    // They canonly be removed AFTER checks for the sats netdevices are all done.
    std::vector<int> netDevIndeciesToRemove = {};
    // link establishment
    for (uint32_t satIndex = 0; satIndex < this->availableSatNetDevices.size(); ++satIndex) {   // lop through sats

        // get node and mobilityModel for this satellite
        Ptr<Node> satNode = this->satelliteNodes.Get(satIndex);
        Ptr<SatSGP4MobilityModel> satMobModel = this->satelliteMobilityModels[satIndex];

        for (uint32_t netDevIndex = 0; netDevIndex < this->availableSatNetDevices[satIndex].size(); ++netDevIndex) {    // loo through sat's netDevices
            
            bool connected = false;     // keep track o if this netdevice gets a connection

            for (uint32_t connSatIndex = 0; connSatIndex < this->availableSatNetDevices.size(); ++connSatIndex) {   // loop through potential sats to connect to
                if (connSatIndex == satIndex) {
                    // Skip so we do not select net devices on the same satellite.
                    continue;
                }

                // get node and mobilityModel for this satellite
                Ptr<Node> connSatNode = this->satelliteNodes.Get(connSatIndex);
                Ptr<SatSGP4MobilityModel> connSatMobModel = this->satelliteMobilityModels[connSatIndex];

                // loop through "potential sat to connect to"'s netdevices
                for (uint32_t connNetDevIndex = 0; connNetDevIndex < this->availableSatNetDevices[connSatIndex].size(); ++connNetDevIndex) {
                    
                    int satFreeNetDev = availableSatNetDevices[satIndex][netDevIndex];
                    int connSatFreeNetDev = availableSatNetDevices[connSatIndex][connNetDevIndex];
                    
                    if (this->satIsLinkValid(satMobModel, satFreeNetDev, connSatMobModel, connSatFreeNetDev)) {
                        
                        // If link is valid, create a link and remove both netDevices from the available vector
                        double distance = satMobModel->GetDistanceFrom(connSatMobModel);
                        // NS_LOG_UNCOND("Coordinates of sat1 with new link: " << satMobModel->GetPosition());
                        NS_LOG_DEBUG("[+] Creating new sat link connection between sat [ " << Names::FindName(satNode) << " ].netDev[ " << satFreeNetDev << " ] and sat [ " << Names::FindName(connSatNode) << " ].netDev[ " << connSatFreeNetDev << " ]   COORDS: " << satMobModel->GetGeoPosition());
                        

                        // Avoid scheduled link acquisition time during first link establishment
                        if (firstTimeLinkEstablishing) {
                            this->establishLink(satNode, satFreeNetDev, connSatNode, connSatFreeNetDev, distance, SAT_SAT);
                        } else {
                            // Establish the new link, but take into account the link acquisition time.
                            // This will schedule the link establish at --> Time.Now() + linkAcquisitionTime
                            Simulator::Schedule(this->linkAcquisitionTime.Get(), [this, satNode, satFreeNetDev, connSatNode, connSatFreeNetDev, distance](){
                                // NS_LOG_DEBUG("[!!!] <" << Simulator::Now().GetSeconds() << "s> Scheduled establish link!");
                                this->establishLink(satNode, satFreeNetDev, connSatNode, connSatFreeNetDev, distance, SAT_SAT);
                            });
                        }

                        connected = true;   // tell the 2 loop that the netdevice now has a connection
                        linksEstablished++;

                        // remove the now taken connNetDevice. We wait with removing the satNode's netDev, so that we dont get indexing errors
                        for (uint32_t i = 0; i < this->availableSatNetDevices[connSatIndex].size(); i++) {   // loop through to find the relevant index to remove at
                            if (this->availableSatNetDevices[connSatIndex][i] == (int)connSatFreeNetDev) {
                                this->availableSatNetDevices[connSatIndex].erase(this->availableSatNetDevices[connSatIndex].begin() + i);
                            }
                        }
                        break;  // after removing, break loop since 2 sats will never be able to connect more netDevices anyway (angles)
                    }
                }

                if (connected) {
                    break;
                }
            }

            if (connected) {
                netDevIndeciesToRemove.emplace_back(netDevIndex);
                continue;       // continue to sat's next netdevice and try to find a link for that one
            }
        }
        // loop through to find the relevant index to remove at
        for (uint32_t i = 0; i < netDevIndeciesToRemove.size(); i++) {
            // -i is added as without it we will remove the wrong indicies as we change the vector's size during the for loop.
            // This problem only arises if a satellite has multiple free netdevices and more than 1 of them gets a link in a single run of this function.
            this->availableSatNetDevices[satIndex].erase(this->availableSatNetDevices[satIndex].begin() + netDevIndeciesToRemove[i] - i);
        }
        netDevIndeciesToRemove.clear();
    }
    // Once we have done it the first time, disable it for the next time!
    firstTimeLinkEstablishing = false;

    NS_LOG_INFO("Maintained " << linksMaintained << " links - Broke " << linksBroken << " links - Established " << linksEstablished << " links");
}





void Constellation::saveCompleteRoute(Ptr<Node> srcNode, Ptr<Node> dstNode){
    NS_LOG_INFO("[!] Route testing (from node " << srcNode->GetId() << " to node " << dstNode->GetId() << ")");

    Ipv4Header header;
    header.SetDestination( dstNode->GetObject<Ipv4>()->GetAddress(1, 0).GetAddress() );
    header.SetSource( srcNode->GetObject<Ipv4>()->GetAddress(1, 0).GetAddress() );
    header.SetProtocol( TcpL4Protocol::PROT_NUMBER );
    Socket::SocketErrno errnoOut = Socket::ERROR_NOTERROR;


    // Start at the source node and get the next hop node based on the routing table
    // The next hop is the gateway, which is found by continously using the IP of the destination node
    // When gateway is found, this becomes the new source node. Repeat until at destination
    Ptr<Node> currentNode = srcNode;
    while (true) {
        Ptr<Ipv4Route> route = currentNode->GetObject<Ipv4>()->GetRoutingProtocol()->RouteOutput(nullptr, header, 0, errnoOut);
        int32_t interfaceOut = currentNode->GetObject<Ipv4>()->GetInterfaceForDevice( route->GetOutputDevice() );

        Ptr<NetDevice> localNetDevice = currentNode->GetDevice(interfaceOut);
        Ptr<NetDevice> foreignNetDevice = getConnectedNetDev(currentNode, interfaceOut);
        // for (int n = 0; n < 2; n++) {
        //     Ptr<NetDevice> tmpDevice = localNetDevice->GetChannel()->GetDevice(n);
        //     if (localNetDevice != tmpDevice)
        //         foreignNetDevice = tmpDevice;
        // }
        Ptr<Node> foreignNode = foreignNetDevice->GetNode();
        // NS_LOG_INFO("Forwarding from " << Names::FindName(currentNode) << "(" << interfaceOut << ") --> " << Names::FindName(foreignNode) << "(" << foreignNode->GetObject<Ipv4>()->GetInterfaceForDevice(foreignNetDevice) << ") at " << route->GetGateway());
        
        // save states here to the vector
        std::pair<Ptr<Node>, int> currPair = {currentNode, interfaceOut};
        std::pair<Ptr<Node>, int> nextPair = {foreignNode, foreignNode->GetObject<Ipv4>()->GetInterfaceForDevice(foreignNetDevice)};
        this->currRoute.emplace_back(currPair);
        this->currRoute.emplace_back(nextPair);

        if (foreignNode == dstNode)
            break;

        currentNode = foreignNode;
    }

    // // print route vector here
    // NS_LOG_INFO("Printing contents of currRoute:");
    // for (const auto& pair : this->currRoute) {
    //     Ptr<Node> node = pair.first;
    //     int netDevice = pair.second;
    //     NS_LOG_INFO("Node name: " << Names::FindName(node) << " - NetDevice: " << netDevice);
    // }


}

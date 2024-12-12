#ifndef CONSTELLATION_HANDLER_H
#define CONSTELLATION_HANDLER_H

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/error-model.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

#include "tleHandler.h"
#include "SRFMath.h"

using namespace ns3;

class Constellation
{
    public:
        NodeContainer satelliteNodes;
        NodeContainer groundStationNodes;

        std::vector<Ptr<SatSGP4MobilityModel>> satelliteMobilityModels;
        std::vector<Ptr<SatConstantPositionMobilityModel>> groundStationsMobilityModels;

        std::vector<TLE> TLEVector;
        std::vector<Orbit> OrbitVector;

        // Constructor declaration
        Constellation(uint32_t satCount,
                      std::string tleDataPath,
                      std::string orbitsDataPath,
                      uint32_t gsCount,
                      std::vector<GeoCoordinate> groundStationsCoordinates,
                      DataRate gsInputDataRate,
                      DataRate satInputDataRate,
                      double gsSatErrorRate,
                      double satSatErrorRate,
                      TimeValue linkAcquisitionSec);


        /**
         * Schedule the simulation to run
         * \param totalMinutes The total amount of minutes the simulation will run
         * \param updateIntervalSeconds How often to update the simulation in seconds
         */
        void scheduleSimulation(int totalMinutes, int updateIntervalSeconds);


    private:
        uint32_t satelliteCount;
        uint32_t groundStationCount;
        
        // Data rates:
        DataRate satToSatDataRate;
        DataRate gsToSatDataRate;

        // Packet loss rates
        double gsSatPacketLossRate = 0.0;
        double satSatPacketLossRate = 0.0;

        // Link acquisition time
        TimeValue linkAcquisitionTime = Seconds(0);

        // Max distance 
        double maxSatToSatDistance = 5000.0;
        double maxGStoSatDistance = 3000.0; // km
        // Idk
        double c = 299792458.0;             // speed of light (m/s)
        double minGSElevation = 5.0;        // in degrees above the horizon


        // =============================================== Route break handling ===============================================
        /**
         * Vector for keeping track of the current route as a vector of [satNode, netDev] pairs. 
         * Check the route before link breaks occur. When breaking links afterwards, check if a the break affects the route.
         * If so, save to a file at which time the break occured. This can then be used for plotting red lines where breaks occur, giving a stronger basis for conclusions
         * based on graphs
         * 
         * This procedure requires the following changes:
         *  - update destroyLink()
         *  - add saveCompleteRoute()
         *  - save link breaks to txt-file for plotting.
         */
        std::vector<std::pair<Ptr<Node>, int>> currRoute;

        /**
         * Save the current route to the 'currRoute' vector
         */
        void saveCompleteRoute(Ptr<Node> srcNode, Ptr<Node> dstNode);


        // ==================== Initial setup ===================
        /**
         * Returns node container with all satellites
         */
        NodeContainer createSatellitesFromTLEAndOrbits(std::string tleDataPath, std::string tleOrbitsPath);

        /**
         * Returns node container with all groundstations
         */
        NodeContainer createGroundStations(std::vector<GeoCoordinate> groundStationsCoordinates);

        /**
         * Initialize all the intra-plane links between the satellites.
         * Should be done in the beginning of the simulation, and only once!
         * Uses the OrbitVector to find intra-planes
         */
        void initializeSatIntraLinks();



        // ==================== General constellation updating ===================
        /**
         * Update the simulation in respect to the simulated time
         */
        void updateConstellation();
        
        void updateSatelliteLinks();

        void updateGroundStationLinks();

    
        // ==================== Utility variables ===================
        typedef enum LinkType
        {
            GS_SAT = 0,
            SAT_SAT
        } LinkType;

        typedef struct AngleRange
        {
            double minAngle;
            double maxAngle;
        } AngleRange;

        const AngleRange NetDeviceAngles[4] = {
            { -45.0,  45.0 }, // NetDev1 forward
            {  45.0, 135.0 }, // NetDev2 left
            { 135.0, 225.0 }, // NetDev3 back
            { 225.0, 315.0 }  // NetDev4 right
        };

        // The available net devices of the satellites.
        std::vector<std::vector<int>> availableSatNetDevices;

        // Queue for providing ipv4 addresses for inter satellite links.
        std::queue<std::pair<Ipv4Address, Ipv4Address>> linkAddressProvider;
        int linkSubnetCounter = 0;

        bool firstTimeLinkEstablishing = true;



        // ==================== Link establishing and destroying ===================

        // Creates a channel between 2 nodes' specified netDevices with delay calculated based on
        // distance and the datarate set from 'dataRate'.
        // If establishing a GS-sat link, GSnode should be node1 and SATnode should be node 2 in the parameters
        // If link type is gs-sat, satellite is assigned an IP on the same subnet as the GS's already existing IP address.
        // If link type is sat-sat, they are both assigned an available IP address (linkAddressProvider)
        // Sets up NetDevices and assigns IP addresses
        void establishLink(Ptr<Node> node1,
                          int node1NetDeviceIndex,
                          Ptr<Node> node2,
                          int node2NetDeviceIndex,
                          double distanceM,
                          LinkType linkType);

        // Destroy the link between node1 and node2's netdevices (specified by index).
        // Makes sure to make all connected netdevices point to the NullChannel, avoiding dangling pointers
        // If destroying a GS-SAT link, GSnode should be node1 and SATnode should be node2 in the parameters
        // Sets down NetDevices and removes IP addresses
        void destroyLink(Ptr<Node> node1,
                        int node1NetDeviceIndex,
                        Ptr<Node> node2,
                        int node2NetDeviceIndex,
                        LinkType linkType);




        // ==================== Utility NetDevice functions ===================
        /**
         * Get the connected satellites node.
         */
        Ptr<NetDevice> getConnectedNetDev(Ptr<Node> GSNode, int netDevIndex);

        /**
         * Check if there is a channel connected to the nodes netdevice with index netDevIndex, which has 2
         * connected netdevices. This means a link exists. return true/false based on if the link exists
         */
        bool hasExistingLink(Ptr<Node> GSNode, int netDevIndex);


        
        // ==================== Link pairs address handling =======================
        /**
         * Get an address subnet pair for an inter satellite link.
         */
        std::pair<Ipv4Address, Ipv4Address> getLinkAddressPair();

        // Method for reclaiming a previously used address.
        void releaseLinkAddressPair(Ipv4Address linkAddress_0, Ipv4Address linkAddress_1);



        // ==================== Link Validators =======================
        /**
         * Returns a bool telling us whether the link is allowed to exist or not.
         * This is based on GS elevation angle and distance between sat and GS
         */
        bool gsIsLinkValid(Ptr<SatConstantPositionMobilityModel> GSMobModel, Ptr<SatSGP4MobilityModel>satMobModel);

        bool satIsLinkValid(Ptr<SatSGP4MobilityModel> mobilityModel,
                            int netDeviceIndex,
                            Ptr<SatSGP4MobilityModel> connMobilityModel,
                            int connNetDeviceIndex);

};

#endif

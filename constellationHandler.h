#ifndef CONSTELLATION_HANDLER_H
#define CONSTELLATION_HANDLER_H

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"

#include "tleHandler.h"

using namespace ns3;


class Constellation {
    public:
        // Attributes
        uint32_t satelliteCount;
        NodeContainer satelliteNodes;

        std::vector<Ptr<SatSGP4MobilityModel>> satelliteMobilityModels;
        std::vector<TLE> TLEVector;
        std::vector<Orbit> OrbitVector;
        

        uint32_t groundStationCount;
        NodeContainer groundStationNodes;
        
        std::vector<Ptr<SatConstantPositionMobilityModel>> groundStationsMobilityModels;

        double maxGStoSatDistance = 5000.0; // km
        double minGSElevation = 5.0;        // in degrees above the horizon
        double c = 299792458.0;             // speed of light (m/s)

        Ipv4AddressHelper satAddressHelper;

        // Constructor declaration
        Constellation(uint32_t satCount, std::string tleDataPath, std::string orbitsDataPath, uint32_t gsCount, std::vector<GeoCoordinate> groundStationsCoordinates);

        // Returns node container with all satellites, passes satelliteMobiliyModels.
        NodeContainer createSatellitesFromTLEAndOrbits(std::string tleDataPath, std::string tleOrbitsPath);

        // Returns node container with all groundstations, passes groundStationsMobilityModels by reference.
        NodeContainer createGroundStations(std::vector<GeoCoordinate> groundStationsCoordinates);

        void simulationLoop(int totalMinutes, int updateIntervalSeconds);

        void updateConstellation();

    private:
        typedef enum LinkType
        {
            GS_SAT = 0,
            SAT_SAT
        } LinkType;

        // Creates a channel between 2 nodes' specified netDevices with calculated delay based on distance and the datarate set to 'dataRate'.
        // If used on a GS-sat link, GSnode should always be before satNode in parameters
        // If link type is gs-sat, satellite is assigned an IP on the same subnet as the GS's already existing IP address
        // If link type is sat-sat, they are both assigned an arbitrary IP address
        void establishLink(Ptr<Node> node1, int node1NetDeviceIndex, Ptr<Node> node2, int node2NetDeviceIndex, double distanceM, StringValue dataRate, LinkType linkType);

        // Destroy the link pointed to by node1's netdevice (specified by index).
        // Makes sure to make all connected netdevices point to the NullChannel, avoiding dangling pointers
        void destroyLink(Ptr<Node> node1, int node1NetDeviceIndex, LinkType linkType);

        // ==================== Satellite specific functions ===================
        void updateInterSatelliteLinks();
        void checkImpossibleSatLinks();

        // ==================== Ground station speecific =======================
        void updateGroundStationLinks();

        bool checkGSLink(int gsIndex, double maxDistanceKM);

        /**
        Check if there is a channel connected to the GS's netdevice1, which has 2 connected NetDevices. this means a link exists.
        return true/false based on if the link exists
        */
        bool GS_existing_link(Ptr<Node> GSNode);

        /**
        Get the connected satellites node.
        */
        Ptr<Node> get_conn_sat(Ptr<Node> GSNode);

        /**
        return a bool telling us whether the link is allowed to exist or not. This is based on GS elevation angle and distance between sat and GS
        */
        bool GS_link_valid(Ptr<SatConstantPositionMobilityModel> GSMobModel, Ptr<SatSGP4MobilityModel>satMobModel);

};


#endif

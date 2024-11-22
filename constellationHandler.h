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

// Returns node container with all satellites, passes satelliteMobiliyModels and TLEDataVector by reference.
NodeContainer createSatellitesFromTLEAndOrbits(uint32_t satelliteCount, std::vector<Ptr<SatSGP4MobilityModel>> &satelliteMobilityModels, std::string tleDataPath, std::string tleOrbitsPath, std::vector<TLE> &TLEVector, std::vector<Orbit> &OrbitVector);

// Returns node container with all groundstations, passes groundStationsMobilityModels by reference.
NodeContainer createGroundStations(int groundStationCount, std::vector<Ptr<SatConstantPositionMobilityModel>> &groundStationsMobilityModels, std::vector<GeoCoordinate> groundStationsCoordinates, Ptr<CsmaChannel> &nullChannel);

// Creates a channel between 2 nodes' specified netDevices with calculated delay based on distance and the datarate set to 'dataRate'.
// Returns -1 if an error occurs. Returns 0 if not
int establishLink(Ptr<Node> node1, int node1NetDeviceIndex, Ptr<Node> node2, int node2NetDeviceIndex, double distanceKM, StringValue dataRate, Ipv4Address networkAddress);

// Destroy the link pointed to by node1's netdevice (specified by index).
// Makes sure to make all connected netdevices point to the NullChannel, avoiding dangling pointers
// Returns -1 if an error occurs. Returns 0 if not
int destroyLink(Ptr<Node> node1, int node1NetDeviceIndex);

// ==================== Satellite specific functions ===================
void updateInterSatelliteLinks();
void checkImpossibleSatLinks();

// ==================== Ground station speecific =======================
void updateGroundStationLinks(std::vector<Ptr<SatConstantPositionMobilityModel>> &groundStationsMobilityModels, 
                              NodeContainer &groundStations, 
                              std::vector<Ptr<SatSGP4MobilityModel>> &satelliteMobilityModels, 
                              NodeContainer &satellites);

bool checkGSLink(int gsIndex, 
                 std::vector<Ptr<SatConstantPositionMobilityModel>> &groundStationsMobilityModels, 
                 NodeContainer &groundStations, 
                 std::vector<Ptr<SatSGP4MobilityModel>> &satelliteMobilityModels, 
                 NodeContainer &satellites, 
                 double maxDistanceKM);

#endif
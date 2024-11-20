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

// Creates a channel with calculated delay based on distance and the datarate set to 'dataRate'
void establishLink(Ptr<Node> node1, Ptr<Node> node2, double distanceKM, StringValue dataRate);

void destroyLink(Ptr<Node> node1, Ptr<Node> node2, Ptr<CsmaChannel> nullChannel);

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
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
NodeContainer createSatellitesFromTLE(int satelliteCount, std::vector<Ptr<SatSGP4MobilityModel>> &satelliteMobilityModels, std::string tleDataPath, std::vector<TLE> &TLEDataVector);

// Returns node container with all groundstations, passes groundStationsMobilityModels by reference.
NodeContainer createGroundStations(int groundStationCount, std::vector<Ptr<SatConstantPositionMobilityModel>> &groundStationsMobilityModels, std::vector<GeoCoordinate> groundStationsCoordinates);

#endif
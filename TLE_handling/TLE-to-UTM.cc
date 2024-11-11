#include "ns3/satellite-module.h"

#include "ns3/core-module.h"
#include "ns3/simulator.h"
#include "ns3/applications-module.h"

using namespace ns3;

// // TODO! Haven't tested if it can read the TLE data properly
// std::vector<TLE> ReadTLEFile(const std::string &filename) {
//     std::ifstream file(filename);
//     std::vector<TLE> tleData;
//     std::string line;
    
//     while (std::getline(file, line)) {
//         TLE tle;
//         tle.name = line;
//         std::getline(file, tle.line1);
//         std::getline(file, tle.line2);
//         tleData.push_back(tle);
//     }
//     return tleData;
// }


void PrintSatellitePosition(Ptr<SatSGP4MobilityModel> satMobility, int interval, int steps) {
    for (int i = 0; i < steps; ++i) {
        // Move the simulator time forward
        Time t = Seconds(i * interval);
        Simulator::Schedule(t, [satMobility, t](){
            // Retrieve the satellite's position at this time
            Vector pos = satMobility->GetPosition();
            GeoCoordinate GeoCoords = GeoCoordinate(pos);   // convert from cartesian to geodesic coordinates
            std::cout << "Time: " << t.GetSeconds() / 60 << " min, Position: ("
                      << GeoCoords.GetLongitude() << ", " << GeoCoords.GetLatitude() << ", "
                      << GeoCoords.GetAltitude() << ")" << std::endl;
        });
    }
}


int main(int argc, char *argv[]) {
    // Setup NS-3 environment
    CommandLine cmd;
    cmd.Parse(argc, argv);

    // Load TLE data for the satellite "STARLINK-1008"
    std::string tleData = "1 44714U 19074B   24315.96966847  .00031810  00000+0  21428-2 0  9992\n2 44714  53.0541 198.1236 0001765 110.2339 249.8839 15.06434075276012";

    // Create an instance of the SatSGP4MobilityModel
    Ptr<SatSGP4MobilityModel> satMobility = CreateObject<SatSGP4MobilityModel>();
    satMobility->SetTleInfo(tleData);

    satMobility->SetStartDate("2024-11-11 10:20:00");   // TLE data fetched at 10:10:45 11-11-2024, make sure it is not too old

    // Set up a loop to check the position every 10 minutes, 10 times
    PrintSatellitePosition(satMobility, 600, 10);

    // Run the simulation
    std::cout << " ===== Start of simulation ===== " << std::endl;
    std::cout << " Simulating position of STARLINK-1008 " << std::endl;
    Simulator::Run();
    Simulator::Destroy();
    std::cout << "===== End of simulation =====" << std::endl;

    return 0;

    
}



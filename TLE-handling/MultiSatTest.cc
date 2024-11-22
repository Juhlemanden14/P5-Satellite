// NS-3 and SNS-3 includes
#include "ns3/satellite-module.h"
#include "ns3/core-module.h"
#include "ns3/simulator.h"
#include "ns3/applications-module.h"

using namespace ns3;

struct TLE {
    std::string name;
    std::string line1;
    std::string line2;
};


// Function to trim trailing spaces, carriage returns, and newline characters
void TrimTrailingSpaces(std::string &str) {
    str.erase(str.find_last_not_of(" \r\n") + 1);
    // std::cout << "Trimmed to: <" << str << ">" << std::endl;
}

// Function to read TLE file and format data into compatible string format
std::vector<TLE> ReadTLEFile(const std::string &filename) {
    std::ifstream file(filename);
    std::vector<TLE> tleData;
    std::string line;
    
    while (std::getline(file, line)) {  // loop over all lines
        TLE tleEntry;
        tleEntry.name = line;         // First line is the satellite name, collect 3 lines at a time in the 'if'
        if (std::getline(file, tleEntry.line1) && std::getline(file, tleEntry.line2)) {
            // Trim extra whitespace and newlines
            TrimTrailingSpaces(tleEntry.name);
            TrimTrailingSpaces(tleEntry.line1);
            TrimTrailingSpaces(tleEntry.line2);

            tleData.push_back(tleEntry);    // push to the back of the vecter containing the TLE entries from the file
        }
    }
    return tleData;
}

// Function to schedule a position update. If verbose is set to true, it prints the new position
void SchedulePositionUpdate(Ptr<SatSGP4MobilityModel> satMobility, int interval, int steps, bool verbose) {
    for (int i = 0; i < steps; ++i) {
        Time t = Seconds(i * interval);
        Simulator::Schedule(t, [satMobility, t, verbose](){
            // Retrieve the satellite's position at this time
            GeoCoordinate pos = satMobility->GetGeoPosition();
            if (verbose) {
                std::cout << "Time: " << t.GetSeconds() << " sec, Position: ("
                          << pos.GetLongitude() << ", " << pos.GetLatitude() << ", "
                          << pos.GetAltitude() << ")" << std::endl;
            }
        });
    }
}

int main(int argc, char *argv[]) {
    // Setup NS-3 environment
    CommandLine cmd;
    cmd.Parse(argc, argv);

    // sim params
    bool verbose = true;
    const int sim_time_s = 60 * 2;
    const int pos_update_interval_s = 15;


    // Load TLE data from a file
    std::string filename = "scratch/P5-Satellite/resources/starlink_11-11-2024_tle_data.txt";
    std::vector<TLE> tleDataVec = ReadTLEFile(filename);    // Read and format the TLE data
    

    // // Display each TLE entry
    // for (const auto &tle : tleDataVec) {
    //     std::cout << "TLE DATA: " << tle.name << "\n" << tle.line1 << "\n" << tle.line2 << "\n" << std::endl;
    // }


    // initialization of satellite mobility model. Use its position for link determination
    Ptr<SatSGP4MobilityModel> satMobility = CreateObject<SatSGP4MobilityModel>();
    TLE tleData = tleDataVec[0];
    std::cout << "TLE DATA: " << tleData.name << "\n" << tleData.line1 << "\n" << tleData.line2 << "\n" << std::endl;
    // Format the two lines into a single string for NS-3 compatibility - IT MUST BE line1\nline2 WITH NO SPACES!!!
    std::string formatted_TLE = tleData.line1 + "\n" + tleData.line2;
    satMobility->SetTleInfo(formatted_TLE);
    satMobility->SetStartDate("2024-11-11 10:20:00");   // Ensure TLE data is recent

    // Set up a loop to check the position 'pos_update_interval_s'
    SchedulePositionUpdate(satMobility, pos_update_interval_s, sim_time_s/pos_update_interval_s, verbose);



    /*
    scheduling another sat
    */
    Ptr<SatSGP4MobilityModel> satMobility2 = CreateObject<SatSGP4MobilityModel>();
    TLE tleData2 = tleDataVec[1];
    std::cout << "TLE DATA: " << tleData2.name << "\n" << tleData2.line1 << "\n" << tleData2.line2 << "\n" << std::endl;
    // Format the two lines into a single string for NS-3 compatibility - IT MUST BE line1\nline2 WITH NO SPACES!!!
    std::string formatted_TLE2 = tleData2.line1 + "\n" + tleData2.line2;
    satMobility2->SetTleInfo(formatted_TLE2);
    satMobility2->SetStartDate("2024-11-11 10:20:00");   // Ensure TLE data is recent

    // Set up a loop to check the position
    SchedulePositionUpdate(satMobility2, pos_update_interval_s, sim_time_s/pos_update_interval_s, verbose);



    for (int i = 0; i < (sim_time_s/pos_update_interval_s); i++) {
        // check distance between sats after 120 seconds have passed.
        // calculates the cartesian distance (straight line) and NOT the haversine distance (distance on sphere)
        Time t_check = Seconds(i * pos_update_interval_s);
        Simulator::Schedule(t_check, [satMobility, satMobility2, t_check, verbose](){
                // Retrieve the satellite's position at this time
                double dist = satMobility->GetDistanceFrom(satMobility2);
                if (verbose) {
                    std::cout << "time: " << t_check.GetSeconds() << " sec, Dist from 0 to 1: "
                            << dist/1000 << "km. "<< std::endl;
                }

                ns3::Vector v = satMobility->GetVelocity();
                if (verbose) {
                    std::cout << "time: " << t_check.GetSeconds() << " sec, Velocity: (x = "
                            << v.x << ", y = " << v.y << ", z = " << v.z << std::endl;
                }

            });
    };



    // Run the simulation
    std::cout << "===== Start of simulation ===== " << std::endl;
    std::cout << "Simulating position of " << tleData.name << std::endl;
    Simulator::Run();
    Simulator::Destroy();
    std::cout << "===== End of simulation =====" << std::endl;

    return 0;

}

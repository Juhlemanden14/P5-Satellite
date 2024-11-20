#include "tleHandler.h"
#include <fstream>
#include <sstream>


// Function to trim trailing spaces, carriage returns, and newline characters
void TrimTrailingSpaces(std::string &str) {
    str.erase(str.find_last_not_of(" \r\n") + 1);
    // std::cout << "Trimmed to: <" << str << ">" << std::endl;
}

// Function to read TLE file and format data into compatible string format
std::vector<TLE> ReadTLEFile(const std::string &filename, std::string &TLEAge) {
    std::ifstream file(filename);
    std::vector<TLE> tleData;
    std::string line;

    // Get TLE data entry
    std::getline(file, TLEAge);
    TrimTrailingSpaces(TLEAge);
    
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

// Function to read orbit file and format data into compatible string format
std::vector<Orbit> ReadOrbitFile(const std::string &filename) {
    std::ifstream file(filename);
    std::vector<Orbit> orbitData;
    std::string line;

    while (std::getline(file, line)) {
        Orbit orbitEntry;
        TrimTrailingSpaces(line);  // Trim whitespace from the orbit name
        orbitEntry.name = line;

        if (std::getline(file, line)) {
            TrimTrailingSpaces(line);  // Trim whitespace from the satellite names line

            // Split the line by commas to get individual satellite names
            std::stringstream ss(line);
            std::string satelliteName;
            while (std::getline(ss, satelliteName, ',')) {
                TrimTrailingSpaces(satelliteName);  // Trim each satellite name
                orbitEntry.satellites.push_back(satelliteName);  // Add satellite to the vector
            }
        }

        orbitData.push_back(orbitEntry);  // Add orbit entry to the list
    }

    return orbitData;
}

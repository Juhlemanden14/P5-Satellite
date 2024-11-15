#include "tleHandler.h"


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
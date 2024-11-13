#include "ns3/satellite-module.h"

struct TLE {
    std::string name;
    std::string line1;
    std::string line2;
};

void TrimTrailingSpaces(std::string &str);

std::vector<TLE> ReadTLEFile(const std::string &filename, std::string &TLEAge);
#ifndef TLE_HANDLER_H
#define TLE_HANDLER_H

#include "ns3/satellite-module.h"

void TrimTrailingSpaces(std::string &str);

struct TLE {
    std::string name;
    std::string line1;
    std::string line2;
};
std::vector<TLE> ReadTLEFile(const std::string &filename, std::string &TLEAge);


struct Orbit {
    std::string name;
    std::vector<std::string> satellites;
};
std::vector<Orbit> ReadOrbitFile(const std::string &filename);

#endif
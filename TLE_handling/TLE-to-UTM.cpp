#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include "sgp4/libsgp4/SGP4.h"  // Adjust this according to where the SGP4 library header file is located

struct TLE {
    std::string name;
    std::string line1;
    std::string line2;
};

std::vector<TLE> ReadTLEFile(const std::string &filename) {
    std::ifstream file(filename);
    std::vector<TLE> tleData;
    std::string line;
    
    while (std::getline(file, line)) {
        TLE tle;
        tle.name = line;
        std::getline(file, tle.line1);
        std::getline(file, tle.line2);
        tleData.push_back(tle);
    }
    return tleData;
}


int main() {
    // Load TLE data
    libsgp4::Tle tle = libsgp4::Tle("STARLINK-1008"           
    "1 44714U 19074B   24315.96966847  .00031810  00000+0  21428-2 0  9992",
    "2 44714  53.0541 198.1236 0001765 110.2339 249.8839 15.06434075276012");
    libsgp4::SGP4 sgp4(tle);

    std::cout << tle << std::endl;

    for (int i = 0; i < 10; ++i)
    {
        libsgp4::DateTime dt = tle.Epoch().AddMinutes(i * 10);
        /*
         * calculate satellite position
         */
        libsgp4::Eci eci = sgp4.FindPosition(dt);
        /*
         * convert satellite position to geodetic coordinates
         */
        libsgp4::CoordGeodetic geo = eci.ToGeodetic();

        std::cout << dt << " " << " " << geo << std::endl;
    };

    return 0;
}

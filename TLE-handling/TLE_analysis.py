"""
This script is made for analysing the TLE data in order to find similarities that might expose if certain satellites are in the
same orbit.
We will look at the RAAN, Inclination and maybe the Mean Anomaly

This is an example of TLE data:
STARLINK-1008           
1 44714U 19074B   24317.96008386  .00012565  00000+0  86086-3 0  9997
2 44714  53.0525 189.1887 0001532 102.3567 257.7594 15.06403837276145
          |         |                       |
          Inclination (degrees)             |
                    RAAN (degrees)          |
                                            Mean Anomaly (degrees)
"""

import matplotlib.pyplot as plt
import numpy as np



if __name__ == "__main__":

    write_file = False

    # ==================== Fetch, split and plot interesting points ====================
    tleDataPath = "scratch/P5-Satellite/TLE-handling/starlink_13-11-2024_tle_data.txt"

    with open(tleDataPath, 'r') as file:
        content = file.readlines()
        # print(content[0])

    satCount = int((len(content)-1) / 3)

    TLEs = {}
    names = []
    Inclinations = np.zeros(satCount, dtype=np.float64)
    RAAN = np.zeros(satCount, dtype=np.float64)
    MeanAnomaly = np.zeros(satCount, dtype=np.float64)


    for n in range(1, len(content), 3):
        name = content[n]
        line1 = content[n+1]
        line2 = content[n+2]

        TLEs[name] = name + line1 + line2

        names.append(name)

        np_index = int(n/3)
        Inclinations[np_index] = line2.split()[2]
        RAAN[np_index] = line2.split()[3]
        MeanAnomaly[np_index] = line2.split()[6]


    print(Inclinations)
    print(RAAN)
    print(MeanAnomaly)

    plt.scatter(Inclinations, RAAN)
    plt.xlabel("Inclination of orbit (degrees)")
    plt.ylabel("RAAN (degrees)")
    plt.title("Scatterplot of satellites in the starlink constellation\nbased on RAAN and Orbit Inclination ")
    plt.grid()
    plt.show()
    # ==================================================================================


    # =================== Find similar orbits and try to group TLEs based on it ===================
    # search for satellites with similar RAAN and Inclination

    InclinationL = 80
    InclinationH = 98

    RAANL = 75   
    RAANH = 81

    MeanAnomalyL = 0
    MeanAnomalyH = 360

    orbit_names = []


    for n in range(len(Inclinations)):
        if (Inclinations[n] > InclinationL) and (Inclinations[n] < InclinationH) and (RAAN[n] > RAANL) and (RAAN[n] < RAANH) and (MeanAnomaly[n] > MeanAnomalyL) and (MeanAnomaly[n] < MeanAnomalyH):
            orbit_names.append(names[n])

    print(len(orbit_names))


    # for name in orbit_names:
        # print(TLEs[name])

    if write_file:
        new_orbit_file_path = f"scratch/P5-Satellite/TLE-handling/starlink_13-11-2024_INC{round((InclinationH + InclinationL) / 2, 1)}_RAAN{round((RAANH + RAANL) / 2, 2)}.txt"

        with open(new_orbit_file_path, "w") as new_file:
            new_file.write(content[0])
            for name in orbit_names:
                new_file.write(TLEs[name])

    # =============================================================================================

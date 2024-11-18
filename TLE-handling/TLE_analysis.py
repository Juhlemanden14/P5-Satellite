"""
This script is made for analysing the TLE data in order to find similarities that might expose if certain satellites are in the
same orbit.
We will look at the RAAN, Inclination and maybe the Mean Anomaly

This is an example of TLE data:
STARLINK-1008           
1 44714U 19074B   24317.96008386  .00012565  00000+0  86086-3 0  9997
2 44714  53.0525 189.1887 0001532 102.3567 257.7594 15.06403837276145
          |         |                       |
          |         RAAN (degrees)          |
          Inclination (degrees)             |
                                            Mean Anomaly (degrees)
"""

import matplotlib.pyplot as plt
import numpy as np
import re
from skyfield.api import Loader, EarthSatellite, Topos

# Function to categorize satellites based on inclination and output debug information
def extract_satellites_by_inclination_range(tleDataPath, polar_limit=90, rosette_ranges=((52.5, 53.5), (42.5, 43.5)),
                                            polar_count=200, rosette_count=150):
    polar_orbit = []
    rosette_53_orbit = []
    rosette_43_orbit = []
    pattern = re.compile(r'(\d+\.\d{4})')  # Match float numbers in line 2 of TLE data

    with open(tleDataPath, 'r') as file:
        lines = file.readlines()
        for i in range(0, len(lines) - 2, 3):  # Step through TLE data in sets of 3 lines
            name = lines[i].strip()
            line2 = lines[i + 2].strip()
            match = pattern.findall(line2)

            if match:
                try:
                    inclination = float(match[0])  # Extract and convert inclination
                    print(f"Satellite: {name}, Inclination: {inclination}")  # Debug print
                    if inclination > polar_limit:
                        polar_orbit.append((name, inclination))
                    elif rosette_ranges[0][0] <= inclination <= rosette_ranges[0][1]:
                        rosette_53_orbit.append((name, inclination))
                    elif rosette_ranges[1][0] <= inclination <= rosette_ranges[1][1]:
                        rosette_43_orbit.append((name, inclination))
                except ValueError:
                    continue  # Skip entries that fail conversion

    # Limit the results to the requested number of satellites
    polar_orbit = polar_orbit[:polar_count]
    rosette_53_orbit = rosette_53_orbit[:rosette_count]
    rosette_43_orbit = rosette_43_orbit[:rosette_count]

    return polar_orbit, rosette_53_orbit, rosette_43_orbit



if __name__ == "__main__":
    write_file = False

    # ==================== Fetch, split and plot interesting points ====================
    tleDataPath = "scratch/P5-Satellite/TLE-handling/starlink_13-11-2024_tle_data.txt"

    with open(tleDataPath, 'r') as file:
        content = file.readlines()

    # Each satellite has 3 datapoints. And the file has 1 entry line containing the date
    satCount = int((len(content)-1) / 3)

    TLEs = {}
    names = []
    Inclinations = np.zeros(satCount, dtype=np.float64)
    RAAN = np.zeros(satCount, dtype=np.float64)
    MeanAnomaly = np.zeros(satCount, dtype=np.float64)
    # For SkyField
    tle_data = []

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
        
        # For Skyfield
        tle_data.append((name, line1, line2))
    # print(Inclinations)
    # print(RAAN)
    # print(MeanAnomaly)


    fig = plt.figure(figsize=(12, 6))
    ax1 = fig.add_subplot(121, projection='3d')
    ax1.scatter(RAAN, MeanAnomaly, Inclinations)
    ax1.set_xlabel("RAAN (degrees)")
    ax1.set_ylabel("MeanAnomaly (degrees)")
    ax1.set_zlabel("Inclination of orbit (degrees)")
    ax1.set_title("Scatterplot of satellites in the starlink constellation\nbased on RAAN and Orbit Inclination")
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


    Inclinations.sort()
    print(Inclinations)
    print(len(Inclinations), len(RAAN), len(MeanAnomaly))
    mask = (Inclinations >= 80) & (Inclinations <= 98)
    Inclinations = Inclinations[mask]
    RAAN = RAAN[mask]
    MeanAnomaly = MeanAnomaly[mask]
    print(len(Inclinations), len(RAAN), len(MeanAnomaly))

    # for n in range(len(Inclinations)):
    #     if (Inclinations[n] > InclinationL) and (Inclinations[n] < InclinationH) and (RAAN[n] > RAANL) and (RAAN[n] < RAANH) and (MeanAnomaly[n] > MeanAnomalyL) and (MeanAnomaly[n] < MeanAnomalyH):
    #         orbit_names.append(names[n])
    # print(len(orbit_names))

    ax2 = fig.add_subplot(122, projection='3d')
    ax2.scatter(RAAN, MeanAnomaly, Inclinations)
    ax2.set_xlabel("RAAN (degrees)")
    ax2.set_ylabel("MeanAnomaly (degrees)")
    ax2.set_zlabel("Inclination of orbit (degrees)")
    ax2.set_title("Filtered version " + tleDataPath)
    plt.tight_layout()
    plt.show()


    # for name in orbit_names:
        # print(TLEs[name])

    if write_file:
        new_orbit_file_path = f"scratch/P5-Satellite/TLE-handling/starlink_13-11-2024_INC{round((InclinationH + InclinationL) / 2, 1)}_RAAN{round((RAANH + RAANL) / 2, 2)}.txt"

        with open(new_orbit_file_path, "w") as new_file:
            new_file.write(content[0])
            for name in orbit_names:
                new_file.write(TLEs[name])







    # Load the TLE data
    load = Loader('~/skyfield-data')  # Change to a suitable directory
    ts = load.timescale()
    satellites = [EarthSatellite(line1, line2, name, ts) for name, line1, line2 in tle_data]
    # Compute latitude and longitude
    time = ts.now()
    latitudes = []
    longitudes = []

    for satellite in satellites:
        geocentric = satellite.at(time)
        subpoint = geocentric.subpoint()
        latitudes.append(subpoint.latitude.degrees)
        longitudes.append(subpoint.longitude.degrees)

    # Plot the results
    plt.figure(figsize=(10, 5))
    plt.scatter(longitudes, latitudes, c='blue', label='Starlink Satellites')
    plt.title('Starlink Satellites - Latitude and Longitude')
    plt.xlabel('Longitude (degrees)')
    plt.ylabel('Latitude (degrees)')
    plt.grid(True)
    plt.legend()



    # =============================================================================================
    # extract_satellites_by_inclination_range(tleDataPath, polar_limit=90, rosette_ranges=((52.5, 53.5), (42.5, 43.5)), polar_count=200, rosette_count=150)




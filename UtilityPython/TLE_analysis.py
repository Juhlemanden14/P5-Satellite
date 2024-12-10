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
from datetime import datetime

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
    # ==================== Fetch, split and plot interesting points ====================
    # tleDataPath =   "scratch/P5-Satellite/resources/starlink_13-11-2024_tle_data.txt"
    # outputPath =    "scratch/P5-Satellite/resources/starlink_13-11-2024_orbits.txt"
    tleDataPath =   "resources/starlink_13-11-2024_tle_data.txt"
    outputPath =    "resources/starlink_13-11-2024_orbits.txt"
    
    # Whether to make 2D or 3D plots(which include Mean Anomaly)
    dimensions = '2d' # 2d or 3d


    with open(tleDataPath, 'r') as file:
        content = file.readlines()
    # Each satellite has 3 datapoints. And the file has 1 entry line containing the date
    satCount = int((len(content)-1) / 3)

    timeOfTLE = datetime.strptime(content[0].strip(), "%Y-%m-%d %H:%M:%S")
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
        names.append(name.strip())

        np_index = int(n/3)
        Inclinations[np_index] = line2.split()[2]
        RAAN[np_index] = line2.split()[3]
        MeanAnomaly[np_index] = line2.split()[6]
        
        # For Skyfield
        tle_data.append((name, line1, line2))
    print(f"Imported {len(names)} satellites from {tleDataPath}")


    fig = plt.figure(figsize=(22, 14))
    plt.rcParams.update({
        'font.size': 18,          # General font size
        'axes.labelsize': 18,     # Font size for axis labels
        'xtick.labelsize': 18,    # Font size for x-axis tick labels
        'ytick.labelsize': 18,    # Font size for y-axis tick labels
        'axes.titlesize': 20      # Font size for plot titles
    })
    plt.subplots_adjust(wspace=0.3, hspace=0.3)

    if dimensions == '2d':    
        ax1 = fig.add_subplot(221)
        ax1.scatter(RAAN, Inclinations)
        ax1.set_ylabel("Inclination of orbit (degrees)")
    elif dimensions == '3d':
        ax1 = fig.add_subplot(221, projection='3d')
        ax1.scatter(RAAN, MeanAnomaly, Inclinations)
        ax1.set_ylabel("MeanAnomaly (degrees)")
        ax1.set_zlabel("Inclination of orbit (degrees)")

    ax1.set_xlabel("RAAN (degrees)")
    ax1.set_title("All satellites")
    # ==================================================================================


    # =================== Find similar orbits and try to group TLEs based on it ===================
    # search for satellites with similar RAAN and Inclination


    # Polar ================================================================
    print("[+] Polar")
    polarMask = (98 >= Inclinations) & (Inclinations >= 80) & (25 >= RAAN)
    tracker = np.arange(0, satCount)[polarMask]
    polarNames = [names[i] for i in tracker]
    if dimensions == '2d':
        ax2 = fig.add_subplot(222)
        ax2.scatter(RAAN[polarMask], Inclinations[polarMask])
        ax2.set_ylabel("Inclination of orbit (degrees)")
    elif dimensions == '3d':
        ax2 = fig.add_subplot(222, projection='3d')
        ax2.scatter(RAAN[polarMask], MeanAnomaly[polarMask], Inclinations[polarMask])
        ax2.set_ylabel("MeanAnomaly (degrees)")
        ax2.set_zlabel("Inclination of orbit (degrees)")

    ax2.set_ylim(97.65, 97.665)
    ax2.set_xlabel("RAAN (degrees)")
    ax2.set_title("Polar orbits at ~97.65 degrees Inclination")
    print("Polar names", polarNames[:3])


    # Rosette43 ================================================================
    print("[+] Rosette43")
    maskRosette43 = (43.5 >= Inclinations) & (Inclinations >= 42.5)
    maskRosette43_X = []
    maskRosette43_X.append(maskRosette43 & (4.2 >= RAAN))
    maskRosette43_X.append(maskRosette43 & (80 >= RAAN) & (RAAN >= 75))
    maskRosette43_X.append(maskRosette43 & (170 >= RAAN) & (RAAN >= 165))
    maskRosette43_X.append(maskRosette43 & (250 >= RAAN) & (RAAN >= 240))
    maskRosette43_X.append(maskRosette43 & (310 >= RAAN) & (RAAN >= 300))
    tracker = np.arange(0, satCount)[maskRosette43]
    
    if dimensions == '2d':
        ax3 = fig.add_subplot(223)
        ax3.set_ylabel("Inclination of orbit (degrees)")
    elif dimensions == '3d':
        ax3 = fig.add_subplot(223, projection='3d')
        ax3.set_ylabel("MeanAnomaly (degrees)")
        ax3.set_zlabel("Inclination of orbit (degrees)")
    
    ax3.set_ylim(42.985, 43.01)
    ax3.set_xlabel("RAAN (degrees)")
    ax3.set_title("Rosette orbits at 43 Inclination")
        
    namesRosette43_X = []
    for m in maskRosette43_X:
        if dimensions == '2d':
            ax3.scatter(RAAN[m], Inclinations[m])
        elif dimensions == '3d':
            ax3.scatter(RAAN[m], MeanAnomaly[m], Inclinations[m])
        tracker = np.arange(0, satCount)[m]
        namesRosette43_X.append([names[i] for i in tracker])

    print("Satellite count", [len(orbit) for orbit in namesRosette43_X])



    # Rosette53 ================================================================
    print("[+] Rosette53")
    maskRosette53 = (53.5 >= Inclinations) & (Inclinations >= 53.2)   # Only take the Rosette53 orbits which have the lowest inclination
    maskRosette53_X = []
    maskRosette53_X.append(maskRosette53 & (20 >= RAAN) & (RAAN >= 10))
    maskRosette53_X.append(maskRosette53 & (90 >= RAAN) & (RAAN >= 80))
    maskRosette53_X.append(maskRosette53 & (190 >= RAAN) & (RAAN >= 180))
    maskRosette53_X.append(maskRosette53 & (265 >= RAAN) & (RAAN >= 255))
    maskRosette53_X.append(maskRosette53 & (325 >= RAAN) & (RAAN >= 315))

    tracker = np.arange(0, satCount)[maskRosette53]

    if dimensions == '2d':
        ax4 = fig.add_subplot(224)
        ax4.set_ylabel("Inclination of orbit (degrees)")
    elif dimensions == '3d':
        ax4 = fig.add_subplot(224, projection='3d')
        ax4.set_ylabel("MeanAnomaly (degrees)")
        ax4.set_zlabel("Inclination of orbit (degrees)")
    ax4.set_xlabel("RAAN (degrees)")
    ax4.set_title("Rosette orbits at 53 Inclination")

    namesRosette53_X = []
    for m in maskRosette53_X:
        if dimensions == '2d':
            ax4.scatter(RAAN[m], Inclinations[m])
        elif dimensions == '3d':
            ax4.scatter(RAAN[m], MeanAnomaly[m], Inclinations[m])

        tracker = np.arange(0, satCount)[m]
        namesRosette53_X.append([names[i] for i in tracker])
    
    print("Satellite count", [len(orbit) for orbit in namesRosette53_X])


    

    # Sort orbits based on coordinates
    load = Loader('~/skyfield-data')  # Change to a suitable directory
    ts = load.timescale()

    satellites = [EarthSatellite(line1, line2, name, ts) for name, line1, line2 in tle_data]
    
    # Compute latitude and longitude
    time = ts.utc(timeOfTLE.year, timeOfTLE.month, timeOfTLE.day, timeOfTLE.hour, timeOfTLE.minute, timeOfTLE.second)
    print(time, timeOfTLE)
    latitudes = []
    longitudes = []
    polarDic = {}
    rosette43_XDic = [{} for _ in range(len(namesRosette43_X))]
    rosette53_XDic = [{} for _ in range(len(namesRosette53_X))]


    for satellite in satellites:
        geocentric = satellite.at(time)
        subpoint = geocentric.subpoint()
        latitudes.append(subpoint.latitude.degrees)
        longitudes.append(subpoint.longitude.degrees)

        if satellite.name in polarNames:
            polarDic[satellite.name] = subpoint.longitude.degrees
        for i, orbit in enumerate(namesRosette43_X):
            if satellite.name in orbit:
                rosette43_XDic[i][satellite.name] = subpoint.longitude.degrees
        for i, orbit in enumerate(namesRosette53_X):
            if satellite.name in orbit:
                rosette53_XDic[i][satellite.name] = subpoint.longitude.degrees

        # # Extract orbital elements
        # n = satellite.model.no_kozai  # Mean motion (radians per minute)
        # M0 = satellite.model.mo        # Mean anomaly at epoch (radians)
        # epoch = satellite.model.jdsatepoch  # Epoch of TLE (Julian date)
        # # Time since epoch in minutes
        # t_since_epoch = (time.tt - epoch) * 1440.0  # Days to minutes
        # # Calculate current Mean Anomaly
        # M = (M0 + n * t_since_epoch) % (2 * 3.141592653589793)  # Ensure within 0 to 2π
        # mean_anomaly_degrees = M * (180.0 / 3.141592653589793)  # Convert to degrees
        # print(f"{satellite.name} {mean_anomaly_degrees}")

    

    # Sort the orbits based on their appending coordinates (lat/long)!
    polarDic = dict(sorted(polarDic.items(), key=lambda item: item[1]))
    for i, d in enumerate(rosette43_XDic):
        rosette43_XDic[i] = dict(sorted(d.items(), key=lambda item: item[1]))
    for i, d in enumerate(rosette53_XDic):
        rosette53_XDic[i] = dict(sorted(d.items(), key=lambda item: item[1]))


    with open(outputPath, 'w') as file:
        file.write("Polar97\n")
        file.write(",".join(list(polarDic.keys())))
        file.write("\n")

        for i, d in enumerate(rosette43_XDic):
            file.write(f"Rosette43_{i}\n")
            file.write(",".join(list(d.keys())))
            file.write("\n")

        for i, d in enumerate(rosette53_XDic):
            file.write(f"Rosette53_{i}\n")
            file.write(",".join(list(d.keys())))
            file.write("\n")

    print(f"[+] Orbits have been written to {outputPath}")

    # Plot the results
    plt.figure(figsize=(10, 5))
    plt.scatter(longitudes, latitudes, c='blue', label='Starlink Satellites')
    plt.title('Starlink Satellites - Latitude and Longitude')
    plt.xlabel('Longitude (degrees)')
    plt.ylabel('Latitude (degrees)')
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.show()

    # =============================================================================================
    extract_satellites_by_inclination_range(tleDataPath, polar_limit=90, rosette_ranges=((52.5, 53.5), (42.5, 43.5)), polar_count=200, rosette_count=150)




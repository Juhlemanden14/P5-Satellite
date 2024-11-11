"""
This script fetches TLE data from celestrak.org and saves it to a .txt file
"""
import requests
import time


def constellation_tle_url(constelation_name: str) -> str:
    """
    Takes a lowercase name as input for the url
    """
    base_url = "https://celestrak.org/NORAD/elements/"
    tle_url = base_url + f"gp.php?GROUP={constelation_name}&FORMAT=tle"
    print(f"This is your constellation URL: {tle_url}")
    return tle_url

def fetch_TLEs(constellation_name):
    """
    Fetches the TLE data from celestrak using the given url
    """

    url = constellation_tle_url(constellation_name)
    response = requests.get(url)
    response.raise_for_status()
    
    # get the date
    ts = time.localtime(time.time())
    print(ts.tm_mday, ts.tm_mon, ts.tm_year)

    # Save the TLE data to a .txt file named after the constellation and the date
    file_path = "scratch/P5-Satellite/TLE_handling/"
    file_name = f"{constellation_name}_{ts.tm_mday}-{ts.tm_mon}-{ts.tm_year}_tle_data.txt"
    with open(file_path + file_name, 'w') as file:
        file.write(response.text)

    # Save the exact point in time of when the data was fetched. For reproducibility and documentation
    file_name = f"{constellation_name}_{ts.tm_mday}-{ts.tm_mon}-{ts.tm_year}_tle_age.txt"
    with open(file_path + file_name, 'w') as file:
        file.write(f"{ts.tm_hour}:{ts.tm_min}:{ts.tm_sec} {ts.tm_mday}-{ts.tm_mon}-{ts.tm_year}")



if __name__ == "__main__":
    constellation_name = 'starlink'

    fetch_TLEs(constellation_name)

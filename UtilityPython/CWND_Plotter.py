import os
import matplotlib.pyplot as plt

# Define the directory containing the .txt files
directory = 'scratch/P5-Satellite/out'

# save link break timestamps
link_break_ts = []

# filepath for link breaks
for filename in os.listdir(directory):
    if filename.startswith("link_break_times_satCount"):
        link_breaks_fp = os.path.join(directory, filename)

        with open(link_breaks_fp, 'r') as linkbreaks_file:
            lines = linkbreaks_file.readlines()
            for line in lines:
                link_break_ts.append(float(line.strip(",\n")))


# Iterate over each file in the directory
for filename in os.listdir(directory):
    if filename.startswith("CongestionWindow"):
        filepath = os.path.join(directory, filename)
        
        # Read the data from the file
        with open(filepath, 'r') as file:
            lines = file.readlines()
        
        # Extract the data
        time = []
        congestion_window = []
        for line in lines[1:]:  # Skip the header line
            t, cwnd = map(float, line.strip().split(','))
            time.append(t)
            congestion_window.append(cwnd)
        
        # Plotting the data
        plt.plot(time, congestion_window, marker='o')

        # plot link breaks
        for ts in link_break_ts:
            plt.axvline(x=ts, color='r', label=f"linkbreak ({ts} s)")

        # Adding title and labels with increased font size
        plt.title(f'Congestion Window Over Time ({filename})', fontsize=36)
        plt.xlabel('Time (s)', fontsize=30)
        plt.ylabel('Congestion Window', fontsize=30)

        # Adjusting axis numbers font size
        plt.tick_params(axis='both', which='major', labelsize=26)


        # Display the plot
        plt.grid(True)
        plt.show()
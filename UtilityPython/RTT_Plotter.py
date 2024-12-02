import os
import matplotlib.pyplot as plt

# Define the directory containing the .txt files
directory = 'scratch/P5-Satellite/out'

# Iterate over each file in the directory
for filename in os.listdir(directory):
    if filename.startswith("RTT"):
        filepath = os.path.join(directory, filename)
        
        # Read the data from the file
        with open(filepath, 'r') as file:
            lines = file.readlines()
        
        # Extract the data
        timestamp = []
        rtt_list = []
        
        for line in lines[1:]:  # Skip the header line
            t, rtt = map(float, line.strip().split(','))
            timestamp.append(t)
            # Convert ns to ms
            rtt_list.append(rtt/1_000_000)
        
        # Plotting the data
        plt.plot(timestamp, rtt_list, marker='o')

        # Adding title and labels
        plt.title(f'RTT ({filename})')
        plt.xlabel('Time (s)')
        plt.ylabel('Round Trip Time (ms)')

        # Display the plot
        plt.grid(True)
        plt.show()

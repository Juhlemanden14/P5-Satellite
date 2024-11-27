import os
import matplotlib.pyplot as plt

# Define the directory containing the .txt files
directory = 'scratch/P5-Satellite/out'

# Iterate over each file in the directory
for filename in os.listdir(directory):
    if filename.endswith('.txt'):
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

        # Adding title and labels
        plt.title(f'Congestion Window Over Time ({filename})')
        plt.xlabel('Time (s)')
        plt.ylabel('Congestion Window')

        # Display the plot
        plt.grid(True)
        plt.show()

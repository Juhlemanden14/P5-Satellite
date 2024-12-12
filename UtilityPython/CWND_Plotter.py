import os
import matplotlib.pyplot as plt
import numpy as np

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

        name_parts: list = filename.split("_")
        # print(f"name_parts: {name_parts}")
        name_suffix: str = name_parts[-2].lower() + "_" +name_parts[-1].lower() # assemble common suffix for finding RTT file. It is lowercase for some reason...
        # print(f"name_suffix: {name_suffix}")

        for other_file in os.listdir(directory):
            if other_file.startswith("RTT") and other_file.endswith(name_suffix):
                rtt_filepath = os.path.join(directory, other_file)

        with open(rtt_filepath, 'r') as rtt_file:
            rtt_lines = rtt_file.readlines()
        
        # Read the data from the file
        with open(filepath, 'r') as file:
            lines = file.readlines()
        

        # Extract the data
        rtt_ts_list = []       # bandwidth delay product = RTT(s) x Datarate(bps)
        time = []
        BDP_list = []
        congestion_window = []
        datarate = 100 * (10**6) # bps (equal to 100 Mbps)

        for rtt_line in rtt_lines[1:]:
            rtt_ts, rtt = map(float, rtt_line.strip().split(','))

            # calculate BDP
            rtt_s = rtt / (10**9)   # go from ns to seconds
            BDP = (datarate * rtt_s) / 8  # in Bytes

            BDP_list.append(BDP)
            rtt_ts_list.append(rtt_ts)

        for line in lines[1:]:  # Skip the header line
            t, cwnd = map(float, line.strip().split(','))
            time.append(t)
            congestion_window.append(cwnd)

        
        # Plotting the data
        plt.plot(rtt_ts_list, BDP_list, color='g', label="Bandwidth-Delay Product (BDP) [MB/s]")
        plt.plot(time, congestion_window, marker='o', label="Congestion window [MB]")

        # scale y-axis
        if len(BDP_list) > 0:
            max_BDP = max(BDP_list)
            num_steps = 10       # steps on the y-axis
            steps = np.arange(0, max_BDP * 1.1, max_BDP/num_steps, dtype='i')
            plt.ylim(0, max_BDP * 1.1)
            plt.yticks(steps, [f"{round(step / 1e6, 2)} MB" for step in steps])


        # plot link breaks
        for ts in link_break_ts:
            plt.axvline(x=ts, color='r')

        # Adding title and labels with increased font size
        plt.title(f'Congestion Window Over Time ({filename})', fontsize=36)
        plt.xlabel('Time (s)', fontsize=30)
        plt.ylabel('Congestion Window (MB)', fontsize=30)

        # Adjusting axis numbers font size
        plt.tick_params(axis='both', which='major', labelsize=26)


        # Display the plot
        plt.grid(True)
        # plt.legend()
        plt.show()

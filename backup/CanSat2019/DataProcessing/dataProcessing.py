import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

labels = ["ID", "RSSI", "Time", "Temp1", "Temp2", "Pressure", "Lng", "Lat", "Speed", "Alt", "Sattelites", "SatValid", "AltValid", "LocValid", "PmValid", "PM 1.0", "PM 2.5", "PM 10.0", "n"]

data = pd.read_csv("DATALOG.txt", sep=";", names = labels)
data = data.drop("n", axis=1)
#data = pd.DataFrame(data.sum(), columns = ["ID", "RSSI", "Time", "Temp1", "Temp2", "Pressure", "Lng", "Lat", "Speed", "Lat", "Sattelites", "SatValid", "AltValid", "LocValid", "PmValid", "PM 1.0", "PM 2.5", "PM 10.0"])

print(data.head())

for i in labels:
    for x in range(2):
        plt.plot(i, labels[x])
        plt.xlabel(i)
        plt.ylabel(labels[x])
        plt.savefig("plots/"+i+"_"+labels[x]+"png")

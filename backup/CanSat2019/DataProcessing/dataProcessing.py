import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

labels = ["ID", "RSSI", "Time", "Temp1", "Temp2", "Pressure", "Lng", "Lat", "Speed", "Alt", "Sattelites", "SatValid", "AltValid", "LocValid", "PmValid", "PM 1.0", "PM 2.5", "PM 10.0", "n"]

data = pd.read_csv("DATALOG.txt", sep=";", names = labels)
data = data.drop("n", axis=1)
data = data.dropna(how = "any")
#data = pd.DataFrame(data.sum(), columns = ["ID", "RSSI", "Time", "Temp1", "Temp2", "Pressure", "Lng", "Lat", "Speed", "Lat", "Sattelites", "SatValid", "AltValid", "LocValid", "PmValid", "PM 1.0", "PM 2.5", "PM 10.0"])

print(data.head())

for i in labels:
    for x in range(2):
        v = 0
        if x==0 :
            v = "ID"
            c = 0
        else:
            v = "Time in Seconds"
            c = 2
        plt.plot( data[labels[c]], data[i])
        if i == "Temp1" :
            plt.ylabel("Temp1 in Celsius")
        if i == "Temp2" :
            plt.ylabel("Temp2 in Celsius")
        if i == "Pressure" :
            plt.ylabel("Pressure in hPa")
        if i == "Lng" :
            plt.ylabel("Lng in degrees")
        if i == "Lat" :
            plt.ylabel("Lat in Degrees")
        if i == "Speed" :
            plt.ylabel("Speed in km/h")
        if i == "Alt" :
            plt.ylabel("Alt in masl")
        if i == "Sattelites" :
            plt.ylabel("Count of sattelites")
        if i == "SatValid" :
            plt.ylabel("If sattelites are valid")
        if i == "AltValid" :
            plt.ylabel("If altitude is valid")
        if i == "LocValid" :
            plt.ylabel("If location is valid")
        if i == "PmValid" :
            plt.ylabel("If pm is valid")
        if i == "PM 1.0" :
            plt.ylabel("Count of partickles below 1 picometers per m^3")
        if i == "PM 2.5" :
            plt.ylabel("Count of partickles below 2.5 picometers per m^3")
        if i == "PM 10.0" :
            plt.ylabel("Count of partickles below 10 picometers per m^3")
        
        plt.xlabel(v)
        plt.savefig("plots/"+i+"_"+labels[c]+".png")
        plt.clf()

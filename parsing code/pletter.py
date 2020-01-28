from matplotlib import pyplot as plt
datapints = [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18]
pm10 = [8,10,10,11,11,10,10,11,11,10,10,10,10,12,569,1133,1148,1161,1721]
pm25 = [12,13,13,15,14,14,14,14,14,14,14,14,14,20,577,1229,1362,1537,2680]
pm100 = [13,14,13,15,14,14,14,15,15,16,16,16,16,22,579,1350,1597,2008,2680]
plt.plot(pm10,datapints)
plt.plot(pm25,datapints)
plt.plot(pm100, datapints)
plt.legend(["PM 1", "PM 2.5", "PM 10"])

plt.savefig("smTest")

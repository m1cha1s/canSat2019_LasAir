import pandas as pd
from matplotlib import pyplot as plt

data = pd.read_csv("DATALOG.txt", sep="; ")
print(data)
data.to_excel("dout.xlsx")

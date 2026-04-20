#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Mon May  9 22:32:46 2022

@author: gavin
"""
import pandas as pd
import numpy as np
import csv
import matplotlib.pyplot as plt
import warnings
warnings.filterwarnings("ignore")



data_path = '/Users/gavin/Projects/Ventilator Flow Monitor/Data/Short Tube Data 4-25.csv'

N = sum(1 for line in open(data_path))


p0 = np.array([0.0]*N)    # inlet
p1 = np.array([0.0]*N)    # middle
p2 = np.array([0.0]*N)    # outlet
p3 = np.array([0.0]*N)    # atmosphere
millis = np.array([0]*N)
index = np.arange(0,N)
with open(data_path) as file:
    
    i = 0
    
    reader = csv.reader(file)
    
    for row in reader:
        
        millis[i] = row[0]
        p0[i] = row[2]
        p1[i] = row[4]
        p2[i] = row[6]
        p3[i] = row[8]  
        
        i += 1
        
        
        

plt.scatter(index,p0)
plt.title('Inlet versus index')


#pump off section
plt.axvline(x=0, color = 'r')
plt.axvline(x=160, color = 'r')


#15 L/s section
plt.axvline(x=180, color = 'blue')
plt.axvline(x=260, color = 'blue')


#20 L/s section
plt.axvline(x=330, color = 'green')
plt.axvline(x=420, color = 'green')

#25 L/s section
plt.axvline(x=475, color = 'orange')
plt.axvline(x=575, color = 'orange')


plt.show()


plt.title('Middle versus index')
plt.scatter(index,p1)
plt.show()


plt.title('Outlet versus index')
plt.scatter(index,p2)
plt.show()


plt.title('Atmosphere versus index')
plt.scatter(index,p3)
plt.show()


constriction = .0001    #m^2
dilation = .0005    #m^2

p0_correction = np.mean(p0[0:160])
p0_corrected = np.array([ p0[180:260] - p0_correction, p0[330:420] - p0_correction, p0[475:575] - p0_correction ]) * 100


p1_correction = np.mean(p1[0:160])
p1_corrected = np.array([ p1[180:260] - p1_correction, p1[330:420] - p1_correction, p1[475:575] - p1_correction ]) * 100



p2_correction = np.mean(p2[0:160])
p2_corrected = np.array([ p2[180:260] - p2_correction, p2[330:420] - p2_correction, p2[475:575] - p2_correction ]) * 100


A1 = .0005 #m^-3
A2 = .0001 #m^-3
rho = 1.22 #kg m^-3


## Examine accuracy at .15 L/s

dp1 = p0_corrected[0] - p1_corrected[0]
dp1_avg = np.around(np.mean(p0_corrected[0] - p1_corrected[0]), 2)

dp1_output = p2_corrected[0] - p1_corrected[0]
dp1_output_avg = np.around(np.mean(p2_corrected[0] - p1_corrected[0]), 2)

print("average input - middle at .15 L/s: ", dp1_avg, 'Pa')
print('average output - middle at .15 L/s: ', dp1_output_avg, 'Pa')
print('Expected: 1.3 Pa')
print()
print()

## Examine accuracy at .2 L/s
dp2 = p0_corrected[1] - p1_corrected[1]
dp2_avg = np.around(np.mean(p0_corrected[1] - p1_corrected[1]), 2)

dp2_output = p2_corrected[1] - p1_corrected[1]
dp2_output_avg = np.around(np.mean(p2_corrected[1] - p1_corrected[1]), 2)

print("average input - middle at .2 L/s: ", dp2_avg), 'Pa'
print('average output - middle at .2 L/s: ', dp2_output_avg, 'Pa')
print('Expected: 2.34 Pa')
print()
print()

## Examine Accuracy at .25 L/s
dp3 = p0_corrected[2] - p1_corrected[2]
dp3_avg = np.around(np.mean(p0_corrected[2] - p1_corrected[2]) , 2)

dp3_output = p2_corrected[2] - p1_corrected[2]
dp3_output_avg = np.around(np.mean(p2_corrected[2] - p1_corrected[2]), 2)


print("average input - middle at .25 L/s: ", dp3_avg, 'Pa')
print('average output - middle at .25 L/s: ', dp3_output_avg, 'Pa')
print('Expected: 3.66 Pa')




"""
This file is bernoulli_flowmeter_study_2022_04_16.py

April 16, 2022

@author: g-gollin

Plot data from Bernoulli flowmeter v5.

"""

# read a CSV file wih flowmeter data.

# import libraries. See
# https://matplotlib.org/stable/tutorials/introductory/sample_plots.html

import numpy as np
import matplotlib
import matplotlib.pyplot as plt
import matplotlib.mlab as mlab
import time
# my own histogramming code
import histogramObject as hb
# import a module that can read csv (comma separated variable) files.
import csv
import math
from mpl_toolkits.mplot3d import Axes3D
from random import Random
import os

from pylab import * 


# hbookObject contains tools for creating an hbook-style
# histogram.

# use this way:
#   import histogramObject as hb
#
#   histo1 = hb.histo('a 1-D histogram', 10, 0., 100.)
#   histo2 = hb.histo('a scatterplot', 10, 0., 100., 15, 0., 4.)
#
#   histo1.hsetlabels('my x axis', 'my y axis');
#
#   histo1.hfill(the_xvalue);
#   histo2.hfill(an_xvalue, a_yvalue);
#
#   histo1.hprint();
#   histo2.hprint();
#
# for a logarithmic y scale use hprintlog instead of hprint:
#   histo1.hprintlog();
#   histo2.hprintlog();


############# initialize stuff ###########

# keep track of execution time
start_time = time.time()

print("set current working directory now ")
os.chdir('/Users/g-gollin/SuperWoodchuckie2/_medicine/ventilator_tube_insertion_flowmeter/python_code')
print("Current working directory is ", os.getcwd())

# CSV filename
# filename = "DPS310A.CSV"
# filename = "DPS310D.CSV"
# filename = "DPS310A_old2.CSV"
# filename = "DPS310D_2022_02_18b_.CSV"
# filename = "DPS310D_2022_02_18b.CSV"
# filename = "DPS310D_2022_02_23.CSV"
# filename = "DPS310E.CSV"
filename = "DPS310E_22_04_21_a.CSV"
# get a line count in this file. Found on the web:
# https://stackoverflow.com/questions/845058/how-to-get-line-count-of-a-large-file-cheaply-in-python 

NumberOfCSVLines = sum(1 for line in open(filename))
print("Number of lines in ", filename, " is ", NumberOfCSVLines)

# set up arrays to hold anemometer data.
# format in the file, on one line, is this (as of 4/2022):
# millis value, "T and P:", 
# temperature0, pressure0 (hPa),
# temperature1, pressure1 (hPa), 
# temperature2, pressure2 (hPa), 
# temperature3, pressure3 (hPa), 
# temperature4, pressure4 (hPa), 
# "corrected P0 - P1 in Pa:", corrected pressure0 - pressure1,
# "corrected P2 - P3 in Pa:", corrected pressure2 - pressure3,
# "Current time (UTC):", hh:mm:ss,
# "Date (dd/mm/yyyy):", dd/mm/yyyy,
# "patient ID", ID

millis = np.array([0] * NumberOfCSVLines)

t0 = np.array([0.0] * NumberOfCSVLines)
p0 = np.array([0.0] * NumberOfCSVLines)

t1 = np.array([0.0] * NumberOfCSVLines)
p1 = np.array([0.0] * NumberOfCSVLines)

t2 = np.array([0.0] * NumberOfCSVLines)
p2 = np.array([0.0] * NumberOfCSVLines)

t3 = np.array([0.0] * NumberOfCSVLines)
p3 = np.array([0.0] * NumberOfCSVLines)

t4 = np.array([0.0] * NumberOfCSVLines)
p4 = np.array([0.0] * NumberOfCSVLines)

p0_p1 = np.array([0.0] * NumberOfCSVLines)
p2_p3 = np.array([0.0] * NumberOfCSVLines)

UTC_time = np.array(["00:00:00"] * NumberOfCSVLines)
UTC_date = np.array(["01/01/2022"] * NumberOfCSVLines)

# create a few histograms
histo_p0_p1 = hb.histo('1. p0 - p1, Pascals', 1401, -2., 12.)
histo_p0_p1.hsetlabels('p0 - p1', 'N');

histo_p0_p1a = hb.histo('1a. p0 - p1, Pascals', 401, -2., 2.)
histo_p0_p1a.hsetlabels('p0 - p1', 'N');

histo_p0_p1b = hb.histo('1b. p0 - p1, Pascals', 601, 3.5, 9.5)
histo_p0_p1b.hsetlabels('p0 - p1', 'N');

histo_p2_p3 = hb.histo('2, p2 - p3, Pascals', 1401, -2., 12.)
histo_p2_p3.hsetlabels('p2 - p3', 'N');

histo_p2_p3a = hb.histo('2a. p2 - p3, Pascals', 401, -2., 2.)
histo_p2_p3a.hsetlabels('p2 - p3', 'N');

histo_p2_p3b = hb.histo('2b. p2 - p3, Pascals', 601, 5.5, 11.5)
histo_p2_p3b.hsetlabels('p2 - p3', 'N');

# now read the file.
with open(filename) as csvfile:

    # array index
    index = 0
    
    # umm... something to do with reading the CSV file.
    filereader = csv.reader(csvfile)
    
    for row in filereader:
        
        # load arrays of data for this line...
        millis[index] = row[0]

        t0[index] = row[2]
        p0[index] = row[3]

        t1[index] = row[4]
        p1[index] = row[5]

        t2[index] = row[6]
        p2[index] = row[7]

        t3[index] = row[8]
        p3[index] = row[9]

        t4[index] = row[10]
        p4[index] = row[11]

        p0_p1[index] = row[13]
        p2_p3[index] = row[15]
        
        index = index + 1

p0mean = np.mean(p0)
p1mean = np.mean(p1)
p2mean = np.mean(p2)
p3mean = np.mean(p3)
p4mean = np.mean(p4)
print("p0 mean = ", p0mean)
print("p1 mean = ", p1mean)
print("p2 mean = ", p2mean)
print("p3 mean = ", p3mean)
print("p4 mean = ", p4mean)

for index in range(0, NumberOfCSVLines):
    histo_p0_p1.hfill(p0_p1[index])
    histo_p2_p3.hfill(p2_p3[index])

    if(p0_p1[index] >= -2. and p0_p1[index] <= 2.):
        histo_p0_p1a.hfill(p0_p1[index])
        
    if(p0_p1[index] >= 3.5 and p0_p1[index] <= 9.5):
        histo_p0_p1b.hfill(p0_p1[index])

    if(p2_p3[index] >= -2. and p2_p3[index] <= 2.):
        histo_p2_p3a.hfill(p2_p3[index])
        
    if(p2_p3[index] >= 5.5 and p2_p3[index] <= 11.5):
        histo_p2_p3b.hfill(p2_p3[index])            

# close already-open graphics windows
matplotlib.pyplot.close("all")

# plot some histograms.
histo_p0_p1.hprint()
histo_p0_p1a.hprint()
histo_p0_p1b.hprint()
histo_p2_p3.hprint()
histo_p2_p3a.hprint()
histo_p2_p3b.hprint()

first_bin = 0
last_bin = 4000

# now plot pressure vs time
fig = plt.figure()
# define axis handle
ax = fig.gca()
# set labels and title
ax.set_xlabel("time (seconds)")
ax.set_ylabel("pressure, hPa")
ax.set_title("5. pressure sensor readings vs time (0:black, 1:red)")
ax.plot(millis/1000, p0, '-', c = 'black')
ax.plot(millis/1000, p1, '-', c = 'red')

# now plot pressure differences vs time
last_bin = 2500
fig = plt.figure()
# define axis handle
ax = fig.gca()
# set labels and title
ax.set_xlabel("time (seconds)")
ax.set_ylabel("pressure differences, baseline-subtracted, Pa")
ax.set_title("6. p0-p1 vs time.")
ax.plot(millis[first_bin:last_bin]/1000, p0_p1[first_bin:last_bin], '.', c = 'black', markersize = 1, alpha = 0.4)
ax.set_ylim(-2, 10)

last_bin = 4000

# now plot pressure differences vs time
fig = plt.figure()
# define axis handle
ax = fig.gca()
# set labels and title
ax.set_xlabel("time (seconds)")
ax.set_ylabel("pressure differences, baseline-subtracted, Pa")
ax.set_title("7. p2-p3 vs time")
ax.plot(millis[first_bin:last_bin]/1000, p2_p3[first_bin:last_bin], '.', c = 'black', markersize = 1, alpha = 0.4)
ax.set_ylim(-5, 7)

"""
fig = plt.figure()
# define axis handle
ax = fig.gca()
# set labels and title
ax.set_xlabel("time (seconds)")
ax.set_ylabel("pressure differences, global baseline-subtracted, Pa")
ax.set_title("8. p0-p1 vs time")
ax.plot(millis/1000, shifted_diff_p0_p1, '.', c = 'black', markersize = 1, alpha = 0.2)
"""

# now plot temperature vs time
fig = plt.figure()
# define axis handle
ax = fig.gca()
# set labels and title
ax.set_xlabel("time (seconds)")
ax.set_ylabel("temperature, *C")
ax.set_title("9. Uncompensated temperatures vs time (0:black, 1:red)")
ax.plot(millis/1000, t0, '-', c = 'black')
ax.plot(millis/1000, t1, '-', c = 'red')

# now plot pressure differences vs. temperature
fig = plt.figure()
# define axis handle
ax = fig.gca()
# set labels and title
ax.set_xlabel("temperature 0, *C")
ax.set_ylabel("corrected pressure differences, Pa")
ax.set_title("10. p0-p1 vs temperature 0")
ax.plot(t0, p0_p1, '.', c = 'black', markersize = 1, alpha = 0.4)

# now plot pressure differences vs. pressure
fig = plt.figure()
# define axis handle
ax = fig.gca()
# set labels and title
ax.set_xlabel("pressure 0, *C")
ax.set_ylabel("corrected pressure differences, Pa")
ax.set_title("11. p0-p1 vs pressure 0")
ax.plot(p0, p0_p1, '.', c = 'black', markersize = 1, alpha = 0.4)

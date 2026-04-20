###################################################################

# This file is histogramObject.py. It defines the class 
# "histogram" and contains tools for creating an hbook-style histogram.
# (hbook is a histogramming package developed at CERN.)

# It also houses various "methods" (functions defined in the class) to allow 
# you to do useful things.

# George Gollin, University of Illinois, March 26, 2017
# nad also November 14, 2021

import numpy as np

class histo:

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
    
    # Note that we want to modify the object in place, rather
    # than working with a copy: this is the ol' call-by-value vs.
    # call-by-reference issue.
    
    def __init__(self, title = "histogram title", nxbins = 10, xleft = 0., \
    xright = 100., nybins = 0, ybottom = 0., ytop = 0.):
        
        # import library
        import numpy as np

        # histogram title: assign the class to have the same value as the 
        # argument when instantiating an object of class histo.
        self.htitle = title

        # number of x bins
        self.nx = nxbins

        # number of y bins, if a scatter plot
        self.ny = nybins
        
        # xmin and xmax: lower edge of first bin, upper edge of last bin
        self.xmin = xleft
        self.xmax = xright

        # ymin and ymax: lower edge of first bin, upper edge of last bin if
        # this is a scatter plot
        self.ymin = ybottom
        self.ymax = ytop
        
        # histogram type: 1 for 1-D, 2 for scatter plot. determine this by the
        # number of y bins.
        if self.ny == 0:
            self.htype = 1
        else:
            self.htype = 2
            self.dy = (self.ymax - self.ymin) / self.ny

        # set the x bin width too
        self.dx = (self.xmax - self.xmin) / self.nx

        # x and y axis labels
        self.xlabel_hist = "x axis label"
        self.ylabel_hist = "y axis label"

        # total number of entries in this histogram, excluding
        # over/underflows
        self.ntot = 0

        # total number of entries in this histogram, including
        # over/underflows
        self.ntot_including_overflows = 0

        # make bin population arrays. integrated_binpop is the cumulative value
        # (running integral) for bin contents; I will normalize it so that it 
        # runs from near zero to 1.000.
        if self.htype == 1:
            
            # nx elements
            self.binpop = np.array([0] * self.nx)
            self.integrated_binpop = self.binpop
            
            # left edges of each of the bins. Setting the last argument to 
            # False means we do not include the right edge of the last bin in 
            # our list of values.
            self.bin_left_edge = np.linspace(self.xmin, self.xmax, self.nx, False )
            
        else:

            # ny rows and nx columns
            self.binpop = np.array([np.array([0] * self.nx)] * self.ny)
            self.integrated_binpop = self.binpop
            
            # left edges and bottom edges of each of the bins.
            self.bin_left_edge = np.linspace(self.xmin, self.xmax, self.nx, False )
            self.bin_bottom_edge = np.linspace(self.ymin, self.ymax, self.ny, False )
            
        # stuff relating to the integrated array so we can throw random numbers
        # flat between zero and one, and decide what value of the integrated
        # histogram's x axis corresponds to that random number.
        self.number_of_probability_bins_uniform_flat = 100
        self.probability_bin_width_uniform_flat = \
        1. / self.number_of_probability_bins_uniform_flat
        
        # integrated probability from zero to the right side of each bin.
        # if, for example, we have a 100 bin array, the bin width will
        # be 0.01 and the first bin will correspond to the 1% point in
        # the histogram. The 50th bin will be the histogram median; the
        # 100th bin will be the right side of the last histogram bin
        # holding anything. I am going to assume that distribution of
        # events inside a single histogram bin is smooth: perhaps flat,
        # or else linear.
        self.integrated_probability_to_here_uniform_flat = \
        np.linspace(self.probability_bin_width_uniform_flat, 1.000, \
        self.number_of_probability_bins_uniform_flat)
        
        self.corresponding_integrated_binpop_array_index = self.integrated_binpop

        # mean and RMS width in x and y
        self.xmean = 0.
        self.xrms = 0.
        self.ymean = 0.
        self.yrms = 0.

        # quantities used to calculate mean and RMS width for points inside
        # the plot boundaries
        self.xsum = 0.
        self.xsumsq = 0.
        self.ysum = 0.
        self.ysumsq = 0.
        
        # flag that ios sete when we have already called hintegrate for this 
        # histogram
        self.already_called_hintegrate = False

        # graphics window handle, set by hplot, and zero before then.
        self.handle = 0
   
    ###########################################################################
    # end of class constructor __init__
    ###########################################################################

    # here is the function to fill the histogram: call it every time we have another 
    # piece of data to accumulate in our histogram.

    def hfill(self, xvalue, yvalue = np.nan):
    
        # use this way:
        #   histo1 = hbookObject('a 1-D histogram', 10, 0., 100.);
        #   histo2 = hbookObject('a scatterplot', 10, 0., 100., ...
        #                                             15, 0., 4.);
        #
        #   histo1.hfill(the_xvalue);
        #   histo2.hfill(an_xvalue, a_yvalue);
    
        # increment a counter
        self.ntot_including_overflows = self.ntot_including_overflows + 1
    
        # calculate the x bin. Note that the first bin is bin zero.
        # do this hbook style, with bins referenced by their lower
        # edge. the "floor" function does a truncation
        # converting (a positive real) to an integer, kinda like Fortran
        if xvalue >= self.xmin and xvalue <= self.xmax:
            
            ixbin = np.floor((xvalue - self.xmin) / self.dx)
    
            # protect against landing exactly on the right edge
            if ixbin > self.nx - 1:
                ixbin = self.nx - 1
    
            # also calculate stuff used in eventual mean/RMS
            # determination
            self.xsum = self.xsum + xvalue
            self.xsumsq = self.xsumsq + xvalue * xvalue
    
        else:
            
            ixbin = -1
    
        # now do the y bin.
        if self.htype == 2 and yvalue >= self.ymin and yvalue <= self.ymax \
        and yvalue != np.nan:
            iybin = np.floor((yvalue - self.ymin) / self.dy)
    
            # protect against landing exactly on the right edge
            if iybin > self.ny - 1:
                iybin = self.ny - 1
    
            # also calculate stuff used in eventual mean/RMS
            # determination
            self.ysum = self.ysum + yvalue
            self.ysumsq = self.ysumsq + yvalue * yvalue
    
        else:
            iybin = -1
    
        # now increment the bin population.
        ixbin_WTF = int(ixbin)
        iybin_WTF = int(iybin)
        
        if self.htype == 1 and ixbin_WTF >= 0:
            self.binpop[ixbin_WTF] = self.binpop[ixbin_WTF] + 1
            self.ntot = self.ntot + 1
    
        if self.htype == 2 and ixbin_WTF >= 0 and iybin_WTF >= 0:
            self.binpop[ixbin_WTF, iybin_WTF] = self.binpop[ixbin_WTF, iybin_WTF] + 1
            self.ntot = self.ntot + 1
    
        # keep a running value of means and rms widths, so calculate
        # them now.
    
        if self.ntot > 0:
    
            self.xmean = self.xsum / self.ntot
            mean_square = self.xsumsq / self.ntot
            self.xrms = np.sqrt(np.abs(mean_square - self.xmean * self.xmean))
    
            if self.htype == 2:
    
                self.ymean = self.ysum / self.ntot
                mean_square = self.ysumsq / self.ntot
                self.yrms = np.sqrt(np.abs(mean_square - self.ymean * self.ymean))
           
    ###########################################################################
    # end of class function hfill
    ###########################################################################
    
    # here is a function to calculate a running integral of the bin
    # population in the histogram. This function also sets up arrays
    # which allow me to generate random numbers distributed
    # (approximately) according to this histogram.
    
    # this routine can only be called for a 1-D histogram.
    
    def hintegrate(self):
    
        if self.htype != 1:
            print("do not call hintegrate for a 2-D histogram.")
            return

        # now calculate a running integration of the histogram
        self.already_called_hintegrate = True
        running_sum = 0
    
        # note that the range function makes a semi-closed interval: it will 
        # include 0, but not self.nx. And since arrays are indexed starting
        # at zero, this is just what we want: the indices should go from 0 to
        # array_length - 1.
        for ixbin in range(0, self.nx):
            running_sum = running_sum + self.binpop[ixbin]
            self.integrated_binpop[ixbin] = running_sum
    
        # now normalize the integrated bin contents so we can use this
        # as a probability distribution. with python we can do all the array
        # elements in one shotm, so comment-out the matlab version.
        # for ixbin in range(0, self.nx):
        #     self.integrated_binpop[ixbin] = self.integrated_binpop[ixbin] / running_sum
        self.integrated_binpop = self.integrated_binpop / running_sum
      
        # now set up arrays I can use to generate random numbers
        # thrown according to the shape of this histogram. We'll be
        # working with two different arrays. One is a uniform, flat
        # distribution of probabilities that range from 0 to 1. The
        # other is the integrated probability distribution for the
        # histogram, namely the fraction of the events in the histogram
        # that occur from the left-most edge of the full histogram up
        # to the right edge of each bin.
    
        # in general, these arrays will have different numbers of
        # bins: the uniform/flat array will typically have 100 bins,
        # while the other will have the same number of bins as the
        # histogram under consideration. we want to associate a bin
        # in the uniform/flat array with a point along the histogram's
        # horizontal axis corresponding to that uniform/flat bin's value.
        # what I mean is this: say bin 50 in the uniform/flat array
        # corresponds to an integrated probability of 0.500. In that
        # case, it should point to the spot in the histogram that is
        # the median in the distribution.
    
        # now loop over bins in the uniform/flat array. Here's what
        # we're doing. Imagine that the uniform/flat array is 100
        # elements long. The first array element, which correspoinds to
        # 1% of the histogram population, will hold the value of the
        # histogram's x axis that corresponds to 1% of the integrated
        # histogram population.
    
        # we want to see which integrated probability bin first exceeds
        # the value of integrated_probability_to_here.
    
        previous_histo_bin = 0
    
        for ibin_uniform_flat in range(0, self.number_of_probability_bins_uniform_flat):
    
            probability_right_edge_uniform_flat = \
            self.integrated_probability_to_here_uniform_flat[ibin_uniform_flat]
    
            # look for the first bin in my histogram that has the
            # integral, up to the right edge of that bin, exceeding the
            # value of probability_right_edge_uniform_flat.
    
            for histo_bin in range(previous_histo_bin, self.nx):
    
                probability_right_edge_histo = self.integrated_binpop[histo_bin]
    
                if probability_right_edge_histo >= probability_right_edge_uniform_flat:
    
                    # found it! see how far from the left edge of this
                    # bin we need to go to get the approximate point on
                    # the histogram x axis.
    
                    if histo_bin > 0:
                        probability_left_edge_histo = self.integrated_binpop[histo_bin - 1]
                    else:
                        probability_left_edge_histo = 0
    
                    fraction_of_histo_bin = \
                    (probability_right_edge_uniform_flat - probability_left_edge_histo) / \
                    (probability_right_edge_histo - probability_left_edge_histo)
    
                    # get histogram x axis value now
    
                    histo_x_value = self.xmin + self.dx * (histo_bin - 1 + fraction_of_histo_bin)
    
                    self.corresponding_integrated_binpop_array_index[ibin_uniform_flat] = \
                    histo_x_value

                    # update a few things then jump out of this inner loop.
                    previous_histo_bin = histo_bin
    
                    # now break out of this loop.
                    break
    
    ###########################################################################
    # end of class function hintegrate
    ###########################################################################
    
    # here is a function to return a random number according to a 
    # distribution given approximately by the histogram.
    
    def hrandom(self):
    
        import random as ran

        # this routine can only be called for a 1-D histogram. 
        if self.htype != 1:
            print("do not call hrandom for a 2-D histogram.")
            return np.nan
    
        # check that we've already called hintegrate by checking the 
        # length of one of the arrays it fills. if not yet called, call 
        # it here.
    
        if not self.already_called_hintegrate:
            self.hintegrate()
    
        # get a random number, chosen flat from [0,1), then turn this
        # into a bin.
        flat_random = ran.random() / self.probability_bin_width_uniform_flat
    
        # now see where in my flat randoms array this falls. the matlab
        # floor function truncates to an integer.
    
        flat_random_bin = np.floor(flat_random)
        flat_random_bin_fraction = flat_random - flat_random_bin
        
        # we need to see where inside this particular bin the random number
        # falls so we can extrapolate across the bin.
    
        if flat_random_bin >= self.number_of_probability_bins_uniform_flat:
    
            # here if we've overshot things, going past the right end of the array
            TheRandom = self.xmax
    
        elif flat_random_bin == 0:
    
            # here if we are in the very first bin
            TheRandom = self.xmin + (self.corresponding_integrated_binpop_array_index[0] - self.xmin) * \
            flat_random_bin_fraction
    
        else:
    
            # here to extrapolate where inside this bin to assign the value.
            TheRandom = self.corresponding_integrated_binpop_array_index[flat_random_bin] + \
            (self.corresponding_integrated_binpop_array_index[flat_random_bin + 1] - \
            self.corresponding_integrated_binpop_array_index[flat_random_bin]) * \
            flat_random_bin_fraction
    
        return TheRandom
    
    ###########################################################################
    # end of class function hrandom
    ###########################################################################

    # here is a function to set the histogram's axis labels.
    
    def hsetlabels(self, xstring = "x axis label", ystring = ""):
    
        self.xlabel_hist = xstring
        self.ylabel_hist = ystring
    
    ###########################################################################
    # end of class function hsetlabels
    ###########################################################################
    
    # here is a function to display the histogram to the screen in a
    # new window, and load the "handle" to that window into the object.
    # we will either display the histogram all by itself in the window (when
    # the arguments are 0 or 1 or unspecified) or else as a sublot in a window
    # with the specified number of rowas and columns of independent histograms.
    # histyograms are numbered from left to right, from top row to bottom row.
    
    def hprint(self, rows = 0, columns = 0, which_pane = 0, print_now = True):
        
        # import library
        import matplotlib.pyplot as plt
        
        # tell whether plot goes by itself or not depending on the
        # number of arguments to this function. If solo, or the first
        # plot to be put in this window, open a new graphics window.
    
        if rows == 0 or columns == 0 or which_pane == 0:
            number_args = 1
        else:
            number_args = 5

        if number_args <= 1 or which_pane == 1:
    
            # open a new window and title it. Also get the handle for
            # the newly opened window. 
            self.handle = plt.figure(self.htitle)
    
        if number_args >= 4:
            
            # set the subplot. 321 indicates three rows, two columns of plots
            # with histogram to be placed in the first row, first column
            subplot_argument = 100 * rows + 10 * columns + which_pane
            plt.subplot(subplot_argument)
            
            if(print_now):
                self.handle = plt.figure(self.htitle)
    
        # now draw it.
        self.hdraw(rows, columns, which_pane, self.handle)
        
    ###########################################################################
    # end of class function hprint
    ###########################################################################
    
    # here is a function to display the histogram to the screen in a
    # new window, and load the "handle" to that window into the object.
    # routine works by calling hprint, then changing the y axis to a
    # log scale.
    
    def hprintlog(self,  rows = 0, columns = 0, which_pane = 0):
        
        self.hprint(rows, columns, which_pane)

        # get the axes        
        ax = self.handle.gca()

        # make a logarithmic y scale
        ax.set_yscale("log")

    ###########################################################################
    # end of class function hprintlog
    ###########################################################################
        
    # here is a function to display the histogram to the screen in the currently
    # open plot window. It is called by hprint.
    
    def hdraw(self,  rows, columns, which_pane, figure_handle):
        
        import matplotlib.pyplot as plt
    
        # tell whether plot goes by itself or not depending on the
        # number of arguments to this function. If solo, or the first
        # plot to be put in this window, put the text box with means,
        # etc. in the upper left corner of the plot.
    
        if rows == 0 or columns == 0 or which_pane == 0:
            number_dimensions = 1
        else:
            number_dimensions = 2
    
        # now get graphics pointer/handle information for a textbox that will go into
        # the currently open window. The "annotation" reference opens the (empty)
        # textbox in the current graphics window. The format for the coordinate
        # information is [xleft ybottom xwidth ywidth], referenced to the full
        # width and height of the graphics portion of the window. (Menus are not
        # included in the width/height calculation.)
    
        # 1-D histogram:
    
        if self.htype == 1 or number_dimensions == 1:
    
            # get axis properties
            # fig, ax = plt.subplots(1, 1)
            # ax = figure_handle.gca()
            # print("zoot! ", figure_handle)
            
            # left edges of bins: self.bin_left_edge
            # bin populations: self.binpop
            # we need to monkey around a little to get the last bin to display.
            bin_edges = \
            np.append(self.bin_left_edge, self.bin_left_edge[self.nx - 1] + self.dx)

            bin_contents = np.append(self.binpop, self.binpop[self.nx - 1])
            
            plt.step(bin_edges, bin_contents, where = "post")

            # label stuff
            plt.xlabel(self.xlabel_hist)
            plt.ylabel(self.ylabel_hist)
            plt.title(self.htitle)
            
            # now create a string that includes 5 digits past the decimal point for the
            # mean and 3 past the decimal for the RMS width. Note the exact placement of
            # percent signs, and so forth. 
            statistics_message = \
            "mean: %.5f  \nRMS = %.3f  \nN = %d" % (self.xmean, self.xrms, self.ntot)
            
            # figure out which column and row of subplots we're in:
            this_column = which_pane % max(columns, 1)
            if(this_column == 0):
                this_column = max(columns, 1)
                            
            this_row = max(int((which_pane - 1) / max(columns, 1)) + 1, 1)

            # x_for_text = 0.02 + float((this_column - 1) / max(columns, 1)) 
            # y_for_text = 0.98 - float((this_row - 1) / max(rows, 1)) 
            x_for_text = 0.15 + 0.85 * float((this_column - 1) / max(columns, 1)) 
            y_for_text = 0.8 - 0.85 * float((this_row - 1) / max(rows, 1)) 
        
            # get axis properties
            # fig, ax = plt.subplots(this_column, this_row)
            # ax = figure_handle.gca()
                    
            # add the statistics information string to the upper left corner of the plot.
            # plt.text(x_for_text, y_for_text, statistics_message, \
            # horizontalalignment="left", verticalalignment="top", \
            # transform=ax.transAxes, color="#550000", fontsize=12)  
            
            # add the statistics information string to the upper left corner of the plot.
            # plt.text(x_for_text, y_for_text, statistics_message, \
            # color="#550000", fontsize=10)  
            plt.figtext(x_for_text, y_for_text, statistics_message)
            # plt.text(x_for_text, 400, statistics_message, \
            # color="#550000", fontsize=10)  

        # 2-D histograms
            
        # I haven't implemented this yet.
            
    ###########################################################################
    # end of class function hdraw
    ###########################################################################

#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from petsys import daqd, config
from copy import deepcopy
import argparse
import math
import time
import os.path

parser = argparse.ArgumentParser(description='Acquire data for QDC calibration')
parser.add_argument("--config", type=str, required=True, help="Configuration file")
parser.add_argument("-o", type=str, dest="fileNamePrefix", required=True, help="Data filename (prefix)")
args = parser.parse_args()

systemConfig = config.ConfigFromFile(args.config, loadMask=config.LOAD_BIAS_CALIBRATION | config.LOAD_BIAS_SETTINGS)
daqd = daqd.Connection()
daqd.initializeSystem()
systemConfig.loadToHardware(daqd, bias_enable=config.APPLY_BIAS_OFF, qdc_mode="qdc")
daqd.openRawAcquisition(args.fileNamePrefix, calMode=True)

nPhases = 3
nLengths = 430
lengthStep = 2
if not os.path.exists(args.fileNamePrefix) or not os.path.samefile(args.fileNamePrefix, "/dev/null"):
	f = open(args.fileNamePrefix + '.bins', 'w')
	f.write("%d\t%f\t%f\n" % (nPhases*nLengths/lengthStep, 0, nLengths))
	f.close()

asicsConfig = daqd.getAsicsConfig()
for ac in list(asicsConfig.values()):
	gc = ac.globalConfig
	for cc in ac.channelConfig:
		# Set simplest trigger_mode_2_* setting
		cc.setValue("trigger_mode_2_t", 0b00)
		cc.setValue("trigger_mode_2_e", 0b000)
		cc.setValue("trigger_mode_2_q", 0b00)
		cc.setValue("trigger_mode_2_b", 0b000)
		
		# Allow full range integration time
		cc.setValue("min_intg_time", 0)
		cc.setValue("max_intg_time", 127)
	
		# Disable channel from triggering.
		# Will selectively enable channels below
		cc.setValue("trigger_mode_1", 0b11)

simultaneousChannels = 64

# Clamp down simulatenous channels due to system limitations
# 126/16 -- GbE interface: 126 events/frame with FEB/D, 16 ASICs per FEB/D
# 1024/80 -- ASIC TX: 1024 clock/frame (x1 SDR), 80 bits per event
simultaneousChannels = min([simultaneousChannels, 126/16, 1024/80])
simultaneousChannels = 1

channelStep = int(math.ceil(64.0/simultaneousChannels))

# Take some data while saturating the range
for firstChannel in range(0, channelStep):
	activeChannels = [ channel for channel in range(firstChannel, 64, channelStep) ]
	activeChannels_string = (", ").join([ "%d" % channel for channel in activeChannels ])

	# Enable triggering for active channels
	cfg = deepcopy(asicsConfig)
	for ac in list(cfg.values()):
		ac.globalConfig.setValue("imirror_bias_top", 23)
		for channel in activeChannels:
			ac.channelConfig[channel].setValue("trigger_mode_1", 0b01)
	daqd.setAsicsConfig(cfg)

	for phase in [float(x)/nPhases for x in range(nPhases)]:
		t_start = time.time()
		daqd.set_test_pulse_febds(nLengths, 1024, phase, False)
		daqd.acquire(0.02, -1, phase)
		t_finish = time.time()
		print("Channel(s): %s Phase: %4.3f (saturation) in %3.2f seconds " % (activeChannels_string, phase, t_finish - t_start))

# Main data acquisition
for firstChannel in range(0, channelStep):
	activeChannels = [ channel for channel in range(firstChannel, 64, channelStep) ]
	activeChannels_string = (", ").join([ "%d" % channel for channel in activeChannels ])
        
	# Enable triggering for active channels
	cfg = deepcopy(asicsConfig)
	for ac in list(cfg.values()):
		for channel in activeChannels:
			ac.channelConfig[channel].setValue("trigger_mode_1", 0b01)
	daqd.setAsicsConfig(cfg)

	for phase in [float(x)/nPhases for x in range(nPhases)]:
		for integrationTime in range(0, nLengths, lengthStep):
			t_start = time.time()
			daqd.set_test_pulse_febds(integrationTime, 1024, phase, False)
			daqd.acquire(0.02, integrationTime, phase)
			t_finish = time.time()
			print("Channel(s): %s Phase: %4.3f clk Length %d clk in %3.2f seconds " % (activeChannels_string, phase, integrationTime, t_finish - t_start))

systemConfig.loadToHardware(daqd, bias_enable=config.APPLY_BIAS_OFF)

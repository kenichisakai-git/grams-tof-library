#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from petsys import daqd, config
from copy import deepcopy
import argparse
import math
import time
import os
import os.path
import sys
import traceback

def acquire_qdc_calibration(config_path, file_name_prefix):
    print("acquire_qdc_calibration called with:", config_path, file_name_prefix)

    systemConfig = config.ConfigFromFile(config_path, loadMask=config.LOAD_BIAS_CALIBRATION | config.LOAD_BIAS_SETTINGS)
    daqd_conn = daqd.Connection()
    daqd_conn.initializeSystem()
    systemConfig.loadToHardware(daqd_conn, bias_enable=config.APPLY_BIAS_OFF, qdc_mode="qdc")
    daqd_conn.openRawAcquisition(file_name_prefix, calMode=True)

    nPhases = 3
    nLengths = 430
    lengthStep = 2

    if not os.path.exists(file_name_prefix) or not os.path.samefile(file_name_prefix, "/dev/null"):
        with open(file_name_prefix + '.bins', 'w') as f:
            f.write("%d\t%f\t%f\n" % (nPhases * nLengths / lengthStep, 0, nLengths))

    asicsConfig = daqd_conn.getAsicsConfig()

    for ac in list(asicsConfig.values()):
        for cc in ac.channelConfig:
            cc.setValue("trigger_mode_2_t", 0b00)
            cc.setValue("trigger_mode_2_e", 0b000)
            cc.setValue("trigger_mode_2_q", 0b00)
            cc.setValue("trigger_mode_2_b", 0b000)
            cc.setValue("min_intg_time", 0)
            cc.setValue("max_intg_time", 127)
            cc.setValue("trigger_mode_1", 0b11)

    simultaneousChannels = min([64, 126/16, 1024/80])
    simultaneousChannels = 1
    channelStep = int(math.ceil(64.0 / simultaneousChannels))

    # Take some data while saturating the range
    for firstChannel in range(0, channelStep):
        activeChannels = [channel for channel in range(firstChannel, 64, channelStep)]
        activeChannels_string = ", ".join(str(c) for c in activeChannels)

        cfg = deepcopy(asicsConfig)
        for ac in list(cfg.values()):
            ac.globalConfig.setValue("imirror_bias_top", 23)
            for channel in activeChannels:
                ac.channelConfig[channel].setValue("trigger_mode_1", 0b01)
        daqd_conn.setAsicsConfig(cfg)

        for phase in [float(x) / nPhases for x in range(nPhases)]:
            t_start = time.time()
            daqd_conn.set_test_pulse_febds(nLengths, 1024, phase, False)
            daqd_conn.acquire(0.02, -1, phase)
            t_finish = time.time()
            print(f"Channel(s): {activeChannels_string} Phase: {phase:.3f} (saturation) in {t_finish - t_start:.2f} seconds")

    # Main data acquisition
    for firstChannel in range(0, channelStep):
        activeChannels = [channel for channel in range(firstChannel, 64, channelStep)]
        activeChannels_string = ", ".join(str(c) for c in activeChannels)

        cfg = deepcopy(asicsConfig)
        for ac in list(cfg.values()):
            for channel in activeChannels:
                ac.channelConfig[channel].setValue("trigger_mode_1", 0b01)
        daqd_conn.setAsicsConfig(cfg)

        for phase in [float(x) / nPhases for x in range(nPhases)]:
            for integrationTime in range(0, nLengths, lengthStep):
                t_start = time.time()
                daqd_conn.set_test_pulse_febds(integrationTime, 1024, phase, False)
                daqd_conn.acquire(0.02, integrationTime, phase)
                t_finish = time.time()
                print(f"Channel(s): {activeChannels_string} Phase: {phase:.3f} clk Length {integrationTime} clk in {t_finish - t_start:.2f} seconds")

    try:
        systemConfig.loadToHardware(daqd_conn, bias_enable=config.APPLY_BIAS_OFF)
    finally:
        daqd_conn.closeAcquisition()
    print("[acquire_qdc_calibration] Done.")
    return True

def safe_acquire_qdc_calibration(config_path, file_name_prefix):
    try:
        return acquire_qdc_calibration(config_path, file_name_prefix)
    except Exception as e:
        print("[Python] Caught exception:", e)
        if os.environ.get("DEBUG"):
            traceback.print_exc()
        return False

def main():
    parser = argparse.ArgumentParser(description='Acquire data for QDC calibration')
    parser.add_argument("--config", type=str, required=True, help="Configuration file")
    parser.add_argument("-o", type=str, dest="fileNamePrefix", required=True, help="Data filename (prefix)")
    args = parser.parse_args()

    safe_acquire_qdc_calibration(args.config, args.fileNamePrefix)

if __name__ == '__main__' and not hasattr(sys, '_called_from_c'):
    main()


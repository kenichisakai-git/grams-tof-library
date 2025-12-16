#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from petsys import daqd, config
from copy import deepcopy
import pandas
import os
import argparse
import math
import time
import sys
import traceback

def acquire_sipm_data(config_path, file_name_prefix, acquisition_time, mode, hw_trigger=False, param_table=None):
    print("acquire_sipm_data called with:", config_path, file_name_prefix, acquisition_time, mode, hw_trigger, param_table)

    validParams = ["OV", "vth_t1", "vth_t2", "vth_e", "disc_lsb_t1"]

    if param_table:  # None or "" will evaluate to False
        if not os.path.exists(param_table):
            print("Error: no such file - %s" % param_table)
            return False
    
        table = pandas.read_table(param_table)
        parNames = list(table)
    
        for name in parNames:
            if name not in validParams:
                print("Error: Invalid parameter - %s" % name)
                return False
    
        step1Values = list(table[parNames[0]])
        if len(parNames) == 2:
            step2Values = list(table[parNames[1]])
        elif len(parNames) == 1:
            step2Values = [0]
            parNames.append("none")
        else:
            print("Error: only two parameters can be scanned at the same time")
            return False
    else:
        parNames = []
        step1Values = []
        step2Values = []
        param_table = None 

    mask = config.LOAD_ALL
    if mode != "mixed":
        mask ^= config.LOAD_QDCMODE_MAP
    systemConfig = config.ConfigFromFile(config_path, loadMask=mask)

    daqd_conn = daqd.Connection()
    daqd_conn.initializeSystem()
    systemConfig.loadToHardware(daqd_conn, bias_enable=config.APPLY_BIAS_ON, hw_trigger_enable=hw_trigger, qdc_mode=mode)

    daqd_conn.openRawAcquisition(file_name_prefix)

    activeAsics = daqd_conn.getActiveAsics()
    activeChannels = [(portID, slaveID, chipID, channelID)
                      for channelID in range(64)
                      for portID, slaveID, chipID in activeAsics]

    asicsConfig = daqd_conn.getAsicsConfig()

    if param_table is None:
        daqd_conn.acquire(acquisition_time, 0, 0)
    else:
        for step1 in step1Values:
            step1 = float(step1)
            if math.isnan(step1):
                continue

            if parNames[0] in ['vth_t1', 'vth_t2', 'vth_e']:
                for portID, slaveID, chipID, channelID in activeChannels:
                    cc = asicsConfig[(portID, slaveID, chipID)].channelConfig[channelID]
                    dac_setting = systemConfig.mapAsicChannelThresholdToDAC(
                        (portID, slaveID, chipID, channelID), parNames[0], int(step1))
                    cc.setValue(parNames[0], dac_setting)
            elif parNames[0] == 'disc_lsb_t1':
                for portID, slaveID, chipID in activeAsics:
                    cc = asicsConfig[(portID, slaveID, chipID)].globalConfig
                    cc.setValue(parNames[0], int(step1))
            elif parNames[0] == 'OV':
                biasVoltageConfig = daqd_conn.get_hvdac_config()
                for key in daqd_conn.getActiveBiasChannels():
                    offset, prebd, bd, over__ = systemConfig.getBiasChannelDefaultSettings(key)
                    vset = offset + bd + step1
                    dac_setting = systemConfig.mapBiasChannelVoltageToDAC(key, vset)
                    biasVoltageConfig[key] = dac_setting
                daqd_conn.set_hvdac_config(biasVoltageConfig)

            daqd_conn.setAsicsConfig(asicsConfig)

            for step2 in step2Values:
                step2 = float(step2)
                if math.isnan(step2):
                    continue

                if parNames[1] in ['vth_t1', 'vth_t2', 'vth_e']:
                    for portID, slaveID, chipID, channelID in activeChannels:
                        cc = asicsConfig[(portID, slaveID, chipID)].channelConfig[channelID]
                        dac_setting = systemConfig.mapAsicChannelThresholdToDAC(
                            (portID, slaveID, chipID, channelID), parNames[1], int(step2))
                        cc.setValue(parNames[1], dac_setting)
                elif parNames[1] == 'disc_lsb_t1':
                    for portID, slaveID, chipID in activeAsics:
                        cc = asicsConfig[(portID, slaveID, chipID)].globalConfig
                        cc.setValue(parNames[1], int(step2))
                elif parNames[1] == 'OV':
                    biasVoltageConfig = daqd_conn.get_hvdac_config()
                    for key in daqd_conn.getActiveBiasChannels():
                        offset, prebd, bd, over__ = systemConfig.getBiasChannelDefaultSettings(key)
                        vset = offset + bd + step2
                        dac_setting = systemConfig.mapBiasChannelVoltageToDAC(key, vset)
                        biasVoltageConfig[key] = dac_setting
                    daqd_conn.set_hvdac_config(biasVoltageConfig)

                if parNames[1] == "none":
                    print(f"Python:: Acquiring data for: {parNames[0]} = {step1}")
                else:
                    print(f"Python:: Acquiring data for: {parNames[0]} = {step1} ; {parNames[1]} = {step2}")
                daqd_conn.setAsicsConfig(asicsConfig)
                daqd_conn.acquire(acquisition_time, float(step1), float(step2))

    try:
        systemConfig.loadToHardware(daqd_conn, bias_enable=config.APPLY_BIAS_OFF)
    finally:
        daqd_conn.closeAcquisition()
    print("[acquire_sipm_data] Done.")
    return True

def safe_acquire_sipm_data(config_path, file_name_prefix, acquisition_time, mode, hw_trigger=False, param_table=None):
    try:
        return acquire_sipm_data(config_path, file_name_prefix, acquisition_time, mode, hw_trigger, param_table)
    except Exception as e:
        print("[Python] Caught exception:", e)
        if os.environ.get("DEBUG"):
            traceback.print_exc()
        return False

def main():
    parser = argparse.ArgumentParser(description='Acquire data for TDC calibration')
    parser.add_argument("--config", type=str, required=True, help="Configuration file")
    parser.add_argument("-o", type=str, dest="fileNamePrefix", required=True, help="Data filename (prefix)")
    parser.add_argument("--time", type=float, required=True, help="Acquisition time (in seconds)")
    parser.add_argument("--mode", type=str, required=True, choices=["tot", "qdc", "mixed"], help="Acquisition mode (tot, qdc or mixed)")
    parser.add_argument("--enable-hw-trigger", dest="hwTrigger", action="store_true", help="Enable the hardware coincidence filter")
    parser.add_argument("--param-table", type=str, dest="paramTable", help="Optional scan table with 1 or 2 parameters to vary")
    args = parser.parse_args()

    safe_acquire_sipm_data(
        config_path=args.config,
        file_name_prefix=args.fileNamePrefix,
        acquisition_time=args.time,
        mode=args.mode,
        hw_trigger=args.hwTrigger,
        param_table=args.paramTable
    )

if __name__ == '__main__' and not hasattr(sys, '_called_from_c'):
    main()


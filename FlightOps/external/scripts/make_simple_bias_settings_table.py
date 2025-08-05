#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from petsys import daqd, config
from copy import deepcopy
import argparse
import os
import sys
import traceback

def make_simple_bias_settings_table(config_path, offset, prebd, bd, over, output_path):
    print("make_simple_bias_settings_table called with:", config_path, offset, prebd, bd, over, output_path)

    cfg = config.ConfigFromFile(config_path, loadMask=config.LOAD_BIAS_CALIBRATION)
    outputFile = open(output_path, "w")

    outputFile.write("#portID\tslaveID\tslotID\tchannelID\tOffset\tPre-breakdown\tBreakdown\tOvervoltage\n")
    for (portID, slaveID, slotID, channelID) in cfg.getCalibratedBiasChannels():
        outputFile.write("%d\t%d\t%d\t%d\t%4.2f\t%4.2f\t%4.2f\t%4.2f\n" %
                         (portID, slaveID, slotID, channelID, offset, prebd, bd, over))

    outputFile.close()
    print("[make_simple_bias_settings_table] Done.")
    return True

def safe_make_simple_bias_settings_table(config_path, offset, prebd, bd, over, output_path):
    try:
        return make_simple_bias_settings_table(config_path, offset, prebd, bd, over, output_path)
    except Exception as e:
        print("[Python] Caught exception:", e)
        if os.environ.get("DEBUG"):
            traceback.print_exc()
        return False

def main():
    parser = argparse.ArgumentParser(description='Make a simple SiPM bias voltage table')
    parser.add_argument("--config", type=str, required=True, help="Configuration file")
    parser.add_argument("--offset", type=float, required=True, help="Bias channel offset")
    parser.add_argument("--prebd", type=float, required=True, help="Pre-breakdown voltage")
    parser.add_argument("--bd", type=float, required=True, help="Breakdown voltage")
    parser.add_argument("--over", type=float, required=True, help="Nominal overvoltage")
    parser.add_argument("-o", type=str, required=True, help="Output file")
    args = parser.parse_args()

    safe_make_simple_bias_settings_table(args.config, args.offset, args.prebd, args.bd, args.over, args.o)

if __name__ == '__main__' and not hasattr(sys, '_called_from_c'):
    main()


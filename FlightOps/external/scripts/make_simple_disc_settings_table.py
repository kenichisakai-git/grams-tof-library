#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from petsys import daqd, config
from copy import deepcopy
import argparse
import os
import sys
import traceback

def make_simple_disc_settings_table(config_path, vth_t1, vth_t2, vth_e, output_path):
    print("make_simple_disc_settings_table called with:", config_path, vth_t1, vth_t2, vth_e, output_path)
    
    cfg = config.ConfigFromFile(config_path, loadMask=config.LOAD_DISC_CALIBRATION)
    outputFile = open(output_path, "w")

    outputFile.write("#portID\tslaveID\tchipID\tchannelID\tvth_t1\tvth_t2\tvth_e\n")
    for portID, slaveID, chipID, channelID in cfg.getCalibratedDiscChannels():
        outputFile.write("%d\t%d\t%d\t%d\t%d\t%d\t%d\n" %
                         (portID, slaveID, chipID, channelID, vth_t1, vth_t2, vth_e))

    outputFile.close()
    print("[make_simple_disc_settings_table] Done.")
    return True

def safe_make_simple_disc_settings_table(config_path, vth_t1, vth_t2, vth_e, output_path):
    try:
        return make_simple_disc_settings_table(config_path, vth_t1, vth_t2, vth_e, output_path)
    except Exception as e:
        print("[Python] Caught exception:", e)
        if os.environ.get("DEBUG"):
            traceback.print_exc()
        return False

def main():
    parser = argparse.ArgumentParser(description='Make a simple SiPM discriminator voltage table')
    parser.add_argument("--config", type=str, required=True, help="Configuration file")
    parser.add_argument("--vth_t1", type=int, required=True, help="Discriminator T1 (DAC above zero)")
    parser.add_argument("--vth_t2", type=int, required=True, help="Discriminator T2 (DAC above zero)")
    parser.add_argument("--vth_e", type=int, required=True, help="Discriminator E (DAC above zero)")
    parser.add_argument("-o", type=str, required=True, help="Output file")
    args = parser.parse_args()

    safe_make_simple_disc_settings_table(args.config, args.vth_t1, args.vth_t2, args.vth_e, args.o)

if __name__ == '__main__' and not hasattr(sys, '_called_from_c'):
    main()


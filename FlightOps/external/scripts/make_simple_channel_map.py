#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from petsys import daqd, config, info
from copy import deepcopy
import argparse
import struct
import sys
import os
import traceback

def make_simple_channel_map(output_path):
    print("make_simple_channel_map called with:", output_path)

    channelOutputFile = open(output_path + "_channel.tsv", "w")
    triggerOutputFile = open(output_path + "_trigger.tsv", "w")

    connection = daqd.Connection()
    connection.initializeSystem(skipFEM=True)

    tgr = connection.getTriggerUnit()
    is_ekit = (connection.getActiveFEBDs() == [tgr])
    no_tgr = (tgr is None)

    region_list = []
    for i, (portID, slaveID) in enumerate(connection.getActiveFEBDs()):
        if is_ekit:
            trigger_port = 0
        elif no_tgr:
            trigger_port = i
        else:
            trigger_port = connection.read_config_register(portID, slaveID, 16, 0x0610) ^ 0xFFFF

        d = connection.getUnitInfo(portID, slaveID)
        n_fem = info.fem_per_febd(d)
        asics_per_fem = info.asic_per_module(d)
        n_asics_per_region = n_fem * asics_per_fem // 4

        for quadrant in range(4):
            trigger_region = 4 * trigger_port + quadrant
            region_list.append(trigger_region)
            for asicID in range(n_asics_per_region * quadrant, n_asics_per_region * (quadrant + 1)):
                for channelID in range(64):
                    channelOutputFile.write(
                        f"{portID}\t{slaveID}\t{asicID}\t{channelID}\t{trigger_region}\t0\t0\t0.0\t0.0\t0.0\n"
                    )

            triggerOutputFile.write(f"{trigger_region}\t{trigger_region}\tM\n")

    for i in range(len(region_list)):
        for j in range(i + 1, len(region_list)):
            trigger_region1 = region_list[i]
            trigger_region2 = region_list[j]
            if trigger_region1 == trigger_region2:
                continue
            triggerOutputFile.write(f"{trigger_region1}\t{trigger_region2}\tC\n")

    channelOutputFile.close()
    triggerOutputFile.close()

    print("[make_simple_channel_map] Done.")
    return True

def safe_make_simple_channel_map(output_path):
    try:
        return make_simple_channel_map(output_path)
    except Exception as e:
        print("[Python] Caught exception:", e)
        if os.environ.get("DEBUG"):
            traceback.print_exc()
        return False

def main():
    parser = argparse.ArgumentParser(description='Make a simple channel and trigger map')
    parser.add_argument("-o", type=str, required=True, help="Output file (base path, no extension)")
    args = parser.parse_args()

    safe_make_simple_channel_map(args.o)

if __name__ == '__main__' and not hasattr(sys, '_called_from_c'):
    main()


#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from petsys import daqd, config, fe_temperature, fe_power
from copy import deepcopy
import argparse
from time import sleep, time
from sys import stdout
import traceback
import os
import sys

def acquire_threshold_calibration(config_file, out_file_prefix, noise_reads=4, dark_reads=4, ext_bias=False, mode="all"):
    print("acquire_threshold_calibration:", config_file, out_file_prefix, noise_reads, dark_reads, ext_bias, "mode:", mode)

    if ext_bias:
            systemConfig = config.ConfigFromFile(config_file, loadMask=0x0)
            if mode in ["all", "dark"]:
                input("Set no-dark bias voltage and press ENTER")
    else:
            systemConfig = config.ConfigFromFile(config_file, loadMask=config.LOAD_BIAS_CALIBRATION | config.LOAD_BIAS_SETTINGS)
    
    conn = daqd.Connection()

    # Ensure FEM power is ON
    for portID, slaveID in conn.getActiveFEBDs():
        if not fe_power.get_fem_power_status(conn, portID, slaveID):
            print(f'WARNING: FEM Power for (portID, slaveID) = ({portID}, {slaveID}) is OFF.')
            fe_power.set_fem_power(conn, portID, slaveID, "on")
            time.sleep(0.01)

    # Check sensors
    sensor_list = fe_temperature.get_sensor_list(conn)
    if not sensor_list:
        print("ERROR: No sensors found. Check connections and power.")
        return False

    conn.initializeSystem()
    
    # Set initial bias state based on mode
    if mode == "dark":
        systemConfig.loadToHardware(conn, bias_enable=config.APPLY_BIAS_OFF)
    else:
        systemConfig.loadToHardware(conn, bias_enable=config.APPLY_BIAS_OFF if ext_bias else config.APPLY_BIAS_PREBD)
        
    asicsConfig = conn.getAsicsConfig()
    COUNT_MAX = 1.0 * (2**22)
    T = COUNT_MAX * (1 / conn.getSystemFrequency())
    
    thresholdList = [
            (0,  "vth_t1", "baseline_t" ),
            (1,  "vth_t2", "baseline_t" ),
            (2,  "vth_e", "baseline_e")
    ]
    
    activeAsics = conn.getActiveAsics()
    activeChannels = [ (portID, slaveID, chipID, channelID) for channelID in range(64) for portID, slaveID, chipID in activeAsics ]
    
    # Global ASIC initialization (Required for both modes to enable counters)
    for (portID, slaveID, chipID), ac in list(asicsConfig.items()):
            if not ac: continue
            gc = ac.globalConfig
            COUNTER_SETTING = 0x4 if conn.getAsicSubtype(portID, slaveID, chipID) == "2B" else 0b110
            gc.setValue("counter_en", 0b1)
            gc.setValue("counter_period", COUNTER_SETTING)
            for cc in ac.channelConfig:
                    cc.setValue("trigger_mode_1", 0)
                    cc.setValue("counter_mode", 0xF)
                    cc.setValue("trigger_b_latched", 0)

    counter_sharing = 1
    asicSubTypes = set([ conn.getAsicSubtype(portID, slaveID, chipID) for portID, slaveID, chipID in list(asicsConfig.keys()) ])
    if "2B" in asicSubTypes:
            counter_sharing = 8

    # --- SECTION: BASELINE & NOISE ---
    if mode in ["all", "baseline_noise", "dark"]:
        print("Adjusting baseline")
        for thresholdIndex, thresholdName, baselineName in thresholdList:
            N_ITERATIONS = 0
            while N_ITERATIONS < 20:
                for ac in list(asicsConfig.values()):
                    for cc in ac.channelConfig:
                        cc.setValue("vth_t1", 0); cc.setValue("vth_t2", 0); cc.setValue("vth_e", 0)
                        cc.setValue("trigger_mode_2_b", thresholdIndex)
                        cc.setValue(thresholdName, 61)
                conn.setAsicsConfig(asicsConfig)
                sleep(1 * T); sleep(counter_sharing * T)

                count_high = {}
                for portID, slaveID, chipID in activeAsics:
                    vv = conn.read_mem_ctrl(portID, slaveID, 5, 24, 64*chipID, 64)
                    for channelID, v in enumerate(vv):
                        count_high[(portID, slaveID, chipID, channelID)] = v / COUNT_MAX

                adjustmentMade = False
                for portID, slaveID, chipID, channelID in activeChannels:
                    b = asicsConfig[(portID, slaveID, chipID)].channelConfig[channelID].getValue(baselineName)
                    if count_high[(portID, slaveID, chipID, channelID)] < 0.95:
                        if b > 0:
                            asicsConfig[(portID, slaveID, chipID)].channelConfig[channelID].setValue(baselineName, b - 1)
                            adjustmentMade = True
                N_ITERATIONS += 1
                if not adjustmentMade: break

        # Generate baseline.tsv for all modes
        if mode == "dark":
            baseline_filename = out_file_prefix + "_baseline_tmp.tsv"
            noise_filename = out_file_prefix + "_noise_tmp.tsv"
        else:
            baseline_filename = out_file_prefix + "_baseline.tsv" 
            noise_filename = out_file_prefix + "_noise.tsv"

        outFile = open(baseline_filename, "w")
        for portID, slaveID, chipID, channelID in activeChannels:
            baseline_T = asicsConfig[(portID, slaveID, chipID)].channelConfig[channelID].getValue("baseline_t")
            baseline_E = asicsConfig[(portID, slaveID, chipID)].channelConfig[channelID].getValue("baseline_e")
            outFile.write("%d\t%d\t%d\t%d\t%d\t%d\n" % (portID, slaveID, chipID, channelID, baseline_T, baseline_E))
        outFile.close()

        print("Scanning threshold for noise")
        outFile = open(noise_filename, "w")
        for thresholdIndex, thresholdName, baselineName in thresholdList:
            stdout.write("%6s " % thresholdName); stdout.flush()
            for thresholdValue in range(0,64):
                for ac in list(asicsConfig.values()):
                    for cc in ac.channelConfig:
                        cc.setValue("vth_t1", 0); cc.setValue("vth_t2", 0); cc.setValue("vth_e", 0)
                        cc.setValue("trigger_mode_2_b", thresholdIndex)
                        cc.setValue(thresholdName, thresholdValue)
                conn.setAsicsConfig(asicsConfig)
                sleep(1*T)
                next_read = time() + counter_sharing*T + 1E-3
                for n in range(noise_reads):
                    s = next_read - time()
                    if s > 0: sleep(s)
                    next_read = time() + counter_sharing*T + 1E-3
                    for portID, slaveID, chipID in activeAsics:
                        vv = conn.read_mem_ctrl(portID, slaveID, 5, 24, 64*chipID, 64)
                        for channelID, v in enumerate(vv):
                            outFile.write("%d\t%d\t%d\t%d\t%s\t%d\t%f\n" % (portID, slaveID, chipID, channelID, thresholdName, thresholdValue, v / COUNT_MAX))
                stdout.write("."); stdout.flush()
            stdout.write("\n")
        outFile.close()

    # --- SECTION: DARK COUNTS ---
    if mode in ["all", "dark"]:
        print("Scanning threshold for dark counts")
        for ac in list(asicsConfig.values()):
            if not ac: continue
            for cc in ac.channelConfig:
                cc.setValue("trigger_mode_1", 0)
                cc.setValue("counter_mode", 0xC)
                cc.setValue("trigger_b_latched", 0)
                cc.setValue("dead_time", 20)
    
        if ext_bias:
            input("Set normal operation bias voltage and press ENTER")
        else:
            systemConfig.loadToHardware(conn, bias_enable=config.APPLY_BIAS_ON)
            sleep(2) # Stabilization
    
        outFile = open(out_file_prefix + "_dark.tsv", "w")
        for thresholdIndex, thresholdName, baselineName in thresholdList:
            stdout.write("%6s " % thresholdName); stdout.flush()
            for thresholdValue in range(64):
                for ac in list(asicsConfig.values()):
                    for cc in ac.channelConfig:
                        cc.setValue("vth_t1", 0); cc.setValue("vth_t2", 0); cc.setValue("vth_e", 0)
                        cc.setValue("trigger_mode_2_b", thresholdIndex)
                        cc.setValue(thresholdName, thresholdValue)
                conn.setAsicsConfig(asicsConfig)
                sleep(1*T)
                next_read = time() + counter_sharing*T + 1E-3
                for n in range(dark_reads):
                    s = next_read - time()
                    if s > 0: sleep(s)
                    next_read = time() + counter_sharing*T + 1E-3
                    for portID, slaveID, chipID in activeAsics:
                        vv = conn.read_mem_ctrl(portID, slaveID, 5, 24, 64*chipID, 64)
                        for channelID, v in enumerate(vv):
                            outFile.write("%d\t%d\t%d\t%d\t%s\t%d\t%f\n" % (portID, slaveID, chipID, channelID, thresholdName, thresholdValue, v / T))
                stdout.write("."); stdout.flush()
            stdout.write("\n")
        outFile.close()

    try:
        systemConfig.loadToHardware(conn, bias_enable=config.APPLY_BIAS_OFF)
    except Exception as e:
        print(f"[Python] Warning: Could not turn off bias: {e}")
    finally:
        try:
            if hasattr(conn, 'closeAcquisition'):
                conn.closeAcquisition()
        except AttributeError:
            print("[Python] DAQ worker already closed or failed to start.")
        except Exception as e:
            print(f"[Python] Error during cleanup: {e}")
            
    print("[acquire_threshold_calibration] Done.")
    return True

def safe_acquire_threshold_calibration(config_file, out_file_prefix, noise_reads=4, dark_reads=4, ext_bias=False, mode="all"):
    try:
        return acquire_threshold_calibration(config_file, out_file_prefix, noise_reads, dark_reads, ext_bias, mode)
    except Exception as e:
        print("[Python] Caught exception:", e)
        traceback.print_exc()
        return False

def main(argv):
    parser = argparse.ArgumentParser(description='Acquire data for threshold calibration')
    parser.add_argument("--config", type=str, required=True)
    parser.add_argument("-o", type=str, dest="outFilePrefix", required=True)
    parser.add_argument("--mode", type=str, choices=["all", "baseline_noise", "dark"], default="all")
    parser.add_argument("--nreads-noise", dest="noise_reads", type=int, default=4)
    parser.add_argument("--nreads-dark", dest="dark_reads", type=int, default=4)
    parser.add_argument("--ext-bias", dest="ext_bias", action="store_true", default=False)
    args = parser.parse_args(argv[1:])

    safe_acquire_threshold_calibration(args.config, args.outFilePrefix, args.noise_reads, args.dark_reads, args.ext_bias, args.mode)
    
if __name__ == '__main__' and not hasattr(sys, '_called_from_c'):
    main(sys.argv)

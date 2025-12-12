#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from petsys import daqd, spi, bias # type: ignore 
import argparse
import struct
import sys
import re
import os
import traceback

def add_calibration_from_rom(connection, portID, slaveID, slotID, outputFile):
	chipID = 0x8000 + 0x100 * slotID + 0x0
	offset = 0

	data = spi.m95256_read(connection, portID, slaveID, chipID, 0x0000, 8)
	if data != b"PETSYS  ":
		if [x for x in spi.m95256_read(connection, portID, slaveID, chipID, 0x0000, 16)] in [bias.BIAS_32P_MAGIC, bias.BIAS_32P_AG_MAGIC, bias.BIAS_32P_LTC2439_MAGIC]:
			print(f'INFO: BIAS_32P detected @ (portID, slaveID, slotID) = ({portID},{slaveID},{slotID})')
			offset = 4
			promLayoutVersion = 0x01
		else:
			sys.stderr.write("ERROR: FEBD (portID %d, slaveID %d) slotID %2d PROM does not contain a valid header. \n" % (portID, slaveID, slotID))
			exit(1)
	
	if offset == 0:
		data = spi.m95256_read(connection, portID, slaveID, chipID, 0x0008, 8)
		promLayoutVersion, = struct.unpack("<Q", data)
	
	if promLayoutVersion == 0x01:
		measuredVoltageFullScale = 100.0
		
		data = spi.m95256_read(connection, portID, slaveID, chipID, 0x0010 + offset, 8)
		n_channels, = struct.unpack("<Q", data)

		data = spi.m95256_read(connection, portID, slaveID, chipID, 0x0018 + offset, 8)
		n_x_values, = struct.unpack("<Q", data)
		
		address = 0x0020
		x_values = [ 0 for i in range(n_x_values) ]
		v_meas = {}
		adc_meas = {}
		for i in range(n_x_values):
			data = spi.m95256_read(connection, portID, slaveID, chipID, 0x020 + 2*i + offset, 2)
			address += 2
			v, =  struct.unpack("<H", data)
			x_values[i] = v

		for j in range(n_channels):
			ch_v_meas = [ 0.0 for i in range(n_x_values) ]
			for i in range(n_x_values):
				data = spi.m95256_read(connection, portID, slaveID, chipID, address + offset, 4)
				address += 4
				v, =  struct.unpack("<I", data)
				v = v * measuredVoltageFullScale / (2**32)
				ch_v_meas[i] = v
			v_meas[(portID, slaveID, j)] = ch_v_meas
			
		for j in range(n_channels):
			ch_adc_meas = [ 0 for i in range(n_x_values) ]
			for i in range(n_x_values):
				data = spi.m95256_read(connection, portID, slaveID, chipID, address + offset, 4)
				address += 4
				v, =  struct.unpack("<I", data)
				ch_adc_meas[i] = v
			adc_meas[(portID, slaveID, j)] = ch_adc_meas
			
		
		for j in range(n_channels):
			for i in range(n_x_values):
				outputFile.write("%d\t%d\t%d\t%d\t%d\t%f\t%d\n" % (portID, slaveID, slotID, j, x_values[i], v_meas[(portID, slaveID, j)][i], adc_meas[(portID, slaveID, j)][i]))
		
	else:
		sys.stderr.write("ERROR: FEBD (portID %d, slaveID %d) slotID %2d has a PROM with an unknown layout 0x%016x\n" % (portID, slaveID, slotID, promLayoutVersion))
		exit(1)
		
	return None

def normalizeAndSplit(l):
        l = re.sub(r"\s*#.*", "", l)   # Remove comments
        l = re.sub(r'^\s*', '', l)     # Remove leading white space
        l = re.sub(r'\s*$', '', l)     # Remove trailing whitespace
        l = re.sub(r'\s+', '\t', l)    # Normalize whitespace to tabs
        l = re.sub(r'\r', '', l)       # Remove \r
        l = re.sub(r'\n', '', l)       # Remove \n
        l = l.split('\t')
        return l

def add_calibration_table_from_file(inputFileName, portID, slaveID, slotID, outputFile):
	f = open(inputFileName)
	c = {}
	x = []
	for l in f:
		l = normalizeAndSplit(l)
		if l == ['']: continue

		if x == []:
			x = [ int(v) for v in l]
		else:
			_, _, channelID = [ int(v) for v in l[0:3] ]
			y = [ float(v) for v in l[3:] ]
			assert len(x) == len(y)

			for i in range(len(x)):
				outputFile.write("%d\t%d\t%d\t%d\t%d\t%f\t%d\n" % (portID, slaveID, slotID, channelID, x[i], y[i], 0))
	f.close()
	return c

def make_bias_calibration_table(output_path, port_ids=None, slave_ids=None, slot_ids=None, filenames=None):
    print("make_bias_calibration_table called with:", output_path, port_ids, slave_ids, slot_ids, filenames)
    connection = daqd.Connection()
    outputFile = open(output_path, "w")

    febd_calibration_type = {}
    for portID, slaveID, slotID in connection.getActiveBiasSlots():
        febd_calibration_type[(portID, slaveID, slotID)] = "NONE"
        if not bias.has_prom(connection, portID, slaveID, slotID):
            continue

        print(f"PROM found: ({portID}, {slaveID}, {slotID})")
        add_calibration_from_rom(connection, portID, slaveID, slotID, outputFile)
        febd_calibration_type[(portID, slaveID, slotID)] = "ROM"

    if filenames:
        for i in range(len(filenames)):
            portID = port_ids[i]
            slaveID = slave_ids[i]
            slotID = slot_ids[i]

            if (portID, slaveID, slotID) not in febd_calibration_type:
                print(f"WARNING: No such FEB/D in system ({portID}, {slaveID}, {slotID})")
            elif febd_calibration_type[(portID, slaveID, slotID)] == "ROM":
                print(f"ERROR: Calibration file given but ROM exists for ({portID}, {slaveID}, {slotID})")
                return False

            print(f"Loading file {filenames[i]} for ({portID}, {slaveID}, {slotID})")
            add_calibration_table_from_file(filenames[i], portID, slaveID, slotID, outputFile)
            febd_calibration_type[(portID, slaveID, slotID)] = "FILE"

    for key, val in febd_calibration_type.items():
        if val == "NONE":
            print(f"ERROR: No calibration for FEB/D {key}")
            return False

    outputFile.close()
    print("[run_bias_calibration] Done.")
    return True

def safe_make_bias_calibration_table(output_path, port_ids=None, slave_ids=None, slot_ids=None, filenames=None):
    try:
        make_bias_calibration_table(output_path, port_ids, slave_ids, slot_ids, filenames)
        return True
    except Exception as e:
        print("[Python] Caught:", e)
        if os.environ.get("DEBUG"):
            traceback.print_exc()
        return False

def main():
    parser = argparse.ArgumentParser(description='Make a simple SiPM bias voltage table')
    parser.add_argument("-o", type=str, required=True, help="Output file")
    parser.add_argument("--port", type=int, action="append", help="Port ID")
    parser.add_argument("--slave", type=int, action="append", help="Slave ID")
    parser.add_argument("--slotID", type=int, action="append", help="Slot ID")
    parser.add_argument("--filename", type=str, action="append", default=[], help="File name")
    args = parser.parse_args()

    safe_make_bias_calibration_table(args.o, args.port, args.slave, args.slotID, args.filename)

if __name__ == '__main__' and not hasattr(sys, '_called_from_c'):
    main()


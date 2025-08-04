#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from petsys import daqd, config
from copy import deepcopy
import argparse
import math
import time

def init_system():
    print("init_system called")
    connection = daqd.Connection()
    connection.initializeSystem()
    return True

def main():
    parser = argparse.ArgumentParser(description="Initialize system and print status.")
    args = parser.parse_args()

    init_system()

if __name__ == '__main__' and not hasattr(sys, '_called_from_c'):
    main()

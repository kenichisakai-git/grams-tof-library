#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from petsys import daqd, config
from copy import deepcopy
import argparse
import math
import time
import os
import traceback

def init_system():
    print("init_system called")
    connection = daqd.Connection()
    connection.initializeSystem()
    return True

def safe_init_system():
    try:
        init_system()
        return True
    except Exception as e:
        print("[Python] Caught:", e)
        if os.environ.get("DEBUG"):
            traceback.print_exc()
        return False

def main():
    parser = argparse.ArgumentParser(description="Initialize system and print status.")
    args = parser.parse_args()
    safe_init_system()

if __name__ == '__main__' and not hasattr(sys, '_called_from_c'):
    main()

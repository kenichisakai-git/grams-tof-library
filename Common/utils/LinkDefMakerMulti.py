#!/usr/bin/python3 -Wall
#
# A little script to generate linkdef files for each of the classes that needs a ROOT dictionary.

import sys
import argparse


def main():
    if len(sys.argv) < 2:
        print("Usage: LinkDefMakerMulti.py <linkdef_filename>"
              " --classes='<classname1> <classname2>' --classes_no_io='<classname3>' ... ")
        print("   ex: LinkDefMakerMulti.py  LinkDef.h --classes='<derp1> <derp2>' --classes_no_io='derp2'")
        return -1

    parser = argparse.ArgumentParser()
    parser.add_argument('fname', type=str, help='LinkDef file name')
    parser.add_argument('--classes',        type=str, help='List of normal classes [will also get vector<Class> built]')
    parser.add_argument('--classes_no_vec', type=str, help='List of classes that do NOT need vectors')
    parser.add_argument('--classes_no_io',  type=str, help='List of classes that do NOT need I/O (or vectors)')
    args = parser.parse_args()

    opf_name = args.fname

    # Class lists
    classes = classes_no_io = classes_no_vec = []

    if args.classes:
        classes = args.classes.split()

    if args.classes_no_vec:
        classes_no_vec = args.classes_no_vec.split()

    if args.classes_no_io:
        classes_no_io = args.classes_no_io.split()

    # open file and print away
    opf = open(opf_name, "w")
    opf.write("#ifdef __CLING__\n")
    opf.write("#pragma link off all globals;\n")    # these are evidently ignored in ROOT6, but leaving them in.
    opf.write("#pragma link off all classes;\n")    # these are evidently ignored in ROOT6, but leaving them in.
    opf.write("#pragma link off all functions;\n\n")  # these are evidently ignored in ROOT6, but leaving them in.
    opf.write("#pragma link C++ nestedclasses;\n")
    opf.write("#pragma link C++ nestedtypedefs;\n\n")
    # Note! before adding any STL dictionaries, check in /cern/src/root-xxx/core/clingutils/src for built-ins.
    for cl in classes:
        opf.write("#pragma link C++ class {0}+;\n".format(cl))
        opf.write("#pragma link C++ class std::vector<{0}>+;\n".format(cl))
    for cl in classes_no_vec:
        opf.write("#pragma link C++ class {0}+;\n".format(cl))
    for cl in classes_no_io:
        opf.write("#pragma link C++ class {0}-;\n".format(cl))

    opf.write("#endif\n")
    opf.close()


if __name__ == "__main__":
    main()

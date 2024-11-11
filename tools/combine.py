#!/bin/python3

import sys
import argparse
from typing import NamedTuple

"""
Tool for combining raw binary files with different origins.
"""

class FileAddress(NamedTuple):
    file_name: str
    origin: int

def parse_name_address(value: str) -> FileAddress:
    parts = value.split(':', 1)

    if len(parts) == 1:
        return FileAddress(value, 0)

    try:
        return FileAddress(
            file_name=parts[0],
            origin=int(eval(parts[1])))
    except:
        print(f'Error parsing name and offset from argument {value}')
        exit(1)

def parse_top(value: str) -> int:
    try:
        return int(eval(value))
    except:
        print(f'Error parsing top of memory from argument {value}')

parser = argparse.ArgumentParser(
    description='Tool for handling raw binary files with different origin addresses.',
    epilog='Memory addresses can be given as most Python evaluable expressions.')

parser.add_argument(
    'inputs', metavar='input:origin', nargs='+',
    type=parse_name_address,
    help='An input file and origin as "filename:address"')

parser.add_argument(
    'output', metavar='output:origin',
    type=parse_name_address,
    help='Output file and origin as "filename:address"')

parser.add_argument(
    '-v', '--verbose',
    action='store_true',
    help='Increase verbosity')

parser.add_argument(
    '-t','--top',
    type=parse_top,
    default=-1,
    help='Top of memory address')

args = parser.parse_args()

inputs = args.inputs
output = args.output

combined_data = bytearray()

if args.verbose:
    print(f'Processing {len(inputs)} input file(s):')

for input in inputs:
    with open(input.file_name,'rb') as fidin:
        indata = fidin.read()

        if args.verbose:
            print(f'  {input.file_name} @ {input.origin:04x} - {input.origin + len(indata) - 1:04x}')

        if len(combined_data) < input.origin:
            combined_data += b'\0' * (input.origin - len(combined_data))
        combined_data[input.origin:(input.origin + len(indata))] = indata

if args.top >= 0:
    if len(combined_data) < args.top:
       combined_data += b'\0' * (args.top - len(combined_data) + 1)
    else:
        combined_data = combined_data[:(args.top+1)]

if args.verbose:
    print(f'Output file: {output.file_name} @ {output.origin:04x} - {len(combined_data) - 1:04x}')

with open(output.file_name,'wb') as fidout:
    fidout.write(combined_data[output.origin:])

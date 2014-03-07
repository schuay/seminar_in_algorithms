#!/usr/bin/env python2

import re
import subprocess

from optparse import OptionParser

ALGORITHMS = [ 'heap'
             , 'noble'
             , 'linden'
             ]

NCPUS = [  1,  2,  3
        ,  4,  6,  8, 10
        , 11, 15, 20
        , 21, 25, 30
        , 31, 35, 40
        , 41, 45, 50
        , 51, 55, 60
        , 61, 65, 70
        , 71, 75, 80
        ]

BIN = 'build/src/pqbench'

def bench(algorithm, ncpus):
    output = subprocess.check_output([ BIN
                                     , '-q', algorithm
                                     , '-n', str(ncpus)
                                     ]) # TODO: Size, offset

    print output

if __name__ == '__main__':
    parser = OptionParser()
    parser.add_option("-a", "--algorithms", dest = "algorithms",
            help = "Comma-separated list of ['heap', 'noble', 'linden']")
    parser.add_option("-n", "--ncpus", dest = "ncpus",
            help = "Comma-separated list of cpu counts")
    (options, args) = parser.parse_args()

    if options.algorithms is None:
        algorithms = ALGORITHMS
    else:
        algorithms = options.algorithms.split(',')
        for a in algorithms:
            if a not in ALGORITHMS:
                parser.error('Invalid algorithm')

    if options.ncpus is None:
        ncpus = NCPUS
    else:
        ncpus = list()
        for n in options.ncpus.split(','):
            try:
                ncpus.append(int(n))
            except:
                parser.error('Invalid cpu count')

    for a in algorithms:
        for n in ncpus:
            bench(a, n)

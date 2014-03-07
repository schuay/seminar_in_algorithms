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

REPS = 10

BIN = 'build/src/pqbench'

def bench(algorithm, ncpus, outfile):
    output = subprocess.check_output([ BIN
                                     , '-q', algorithm
                                     , '-n', str(ncpus)
                                     ]) # TODO: Size, offset

    outstr = '%s, %d, %s' % (algorithm, ncpus, output.strip())

    print outstr
    f.write(outstr + '\n')

if __name__ == '__main__':
    parser = OptionParser()
    parser.add_option("-a", "--algorithms", dest = "algorithms", default = ",".join(ALGORITHMS),
            help = "Comma-separated list of ['heap', 'noble', 'linden']")
    parser.add_option("-n", "--ncpus", dest = "ncpus", default = ",".join(map(str, NCPUS)),
            help = "Comma-separated list of cpu counts")
    parser.add_option("-o", "--outfile", dest = "outfile", default = '/dev/null',
            help = "Write results to outfile")
    parser.add_option("-r", "--reps", dest = "reps", type = 'int', default = REPS,
            help = "Repetitions per run")
    (options, args) = parser.parse_args()

    algorithms = options.algorithms.split(',')
    for a in algorithms:
        if a not in ALGORITHMS:
            parser.error('Invalid algorithm')

    ncpus = list()
    for n in options.ncpus.split(','):
        try:
            ncpus.append(int(n))
        except:
            parser.error('Invalid cpu count')

    with open(options.outfile, 'a') as f:
        for a in algorithms:
            for n in ncpus:
                for r in xrange(options.reps):
                    bench(a, n, f)

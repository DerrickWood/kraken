#!/usr/bin/env python3

import sys
import os
import gzip

extension='_genomic.fna.gz'
if len(sys.argv) < 4:
    print("Usage: {} assembly_summary.txt folder seqid2tax.map".format(sys.argv[0]))
    exit(1)

assembly=sys.argv[1]
folder=sys.argv[2]
mapping=sys.argv[3]

with open(assembly,'r') as fin:
    with open(mapping,'w') as out:
        for line in fin:
            if line.startswith('#'):
                continue
            spline=line.split('\t')
            taxid=spline[5]
            ftppath=spline[19]
            filename=os.path.basename(ftppath)
            fasta=os.path.join(folder,filename)+extension

            with gzip.open(fasta,'rt') as fas:
                for line in fas:
                    if line.startswith('>'):    
                        print(line.split()[0][1:],taxid,sep='\t',file=out)

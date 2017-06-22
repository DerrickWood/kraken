#!/usr/bin/env bash

set -e

#TODO: protect agains unassigned variables
EXTENSION="_genomic.fna.gz"

if [ $# -ne 3 ];then
    echo "Usage: make_seqid2taxid_map.sh assembly_summary.txt folder seqid2tax.map"
    exit 1
fi

assembly_summary=$1
folder=$2
seqid2tax=$3
while read line; do
    case "$line" in \#*)
        continue;;
      *)
        ftp_path=`echo -e "$line" | cut -f 20`
        taxid=`echo -e "$line" | cut -f 6`
        sample=`basename $ftp_path`
        fasta=$folder/$sample${EXTENSION%.gz}

        # Read all headers
        grep '^>' < $fasta |
        while read header; do
            header=`echo $header | cut -d ' ' -f 1`
            echo -e "${header:1}\t$taxid" >> $seqid2tax
        done 
    esac
done < $assembly_summary

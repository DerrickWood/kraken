#!/usr/bin/env bash

set -e

#TODO: protect agains unassigned variables
EXTENSION="_genomic.fna.gz"

if [ $# -ne 2 ]; then
    echo "Usage: make_rsync_file.sh assembly_summary.txt rsync_listing.txt"
    exit 1
fi
assembly_summary=$1
rsync_listing=$2

rm -f $rsync_listing

while read line; do
    case "$line" in \#*)
        continue;;
      *)
        ftp_path=`echo -e "$line" | cut -f 20`
        sample=`basename $ftp_path`
        rsync_path=${ftp_path#ftp://ftp.ncbi.nlm.nih.gov/genomes}
        rsync_path=$rsync_path/$sample$EXTENSION
        echo "$rsync_path" >> $rsync_listing
        
    esac
done < $assembly_summary

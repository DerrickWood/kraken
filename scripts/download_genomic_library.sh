#!/bin/bash

# Copyright 2013-2015, Derrick Wood <dwood@cs.jhu.edu>
#
# This file is part of the Kraken taxonomic sequence classification system.
#
# Kraken is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Kraken is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Kraken.  If not, see <http://www.gnu.org/licenses/>.

# Download specific genomic libraries for use with Kraken.
# Supported choices are the folder (NOT LINKS) here:
# ftp://ftp.ncbi.nlm.nih.gov/genomes/refseq/
# and also:
#   human - NCBI RefSeq GRCh38 human reference genome

# Please note: This script does not check if the specified library is valid
# This is left for the kraken-build script

set -u  # Protect against uninitialized vars.
set -e  # Stop on error

LIBRARY_DIR="$KRAKEN_DB_NAME/library"
NCBI_SERVER="ftp.ncbi.nlm.nih.gov"

FTP_SERVER="ftp://$NCBI_SERVER"
RSYNC_SERVER="rsync://$NCBI_SERVER"
SEQ2TAXID="seqid2taxid.map"
THIS_DIR=$PWD

RSYNC="rsync --progress -ai --no-relative --delete --force"

# Just for the pretty printing of rsync progress
function sameline {
    while read line; do
        echo -ne "$line\r"
    done
    echo -e "\r"
}

function download {
  # Parse samples into a file that rsync can use
  echo -n "Preparing download list..."
  make_rsync_file.sh assembly_summary.txt rsync_listing.txt
  echo " complete."

  # Download the files
  echo "Downloading `cat rsync_listing.txt|wc -l` files..."
  $RSYNC --files-from=rsync_listing.txt ftp.ncbi.nlm.nih.gov::genomes . | sameline
  echo "Downloading `cat rsync_listing.txt|wc -l` files... complete."
}

function seqid2taxid {
  # Map the headers to taxid
  echo -n "Mapping seqid to taxid..."
  make_seqid2tax_map.py assembly_summary.txt . $SEQ2TAXID
  echo " complete."
}

case "$1" in
  "human")
    # Humans are a special case
    mkdir -p $LIBRARY_DIR/human
    cd $LIBRARY_DIR/human
    echo "Downloading human sequences..."
    $RSYNC rsync://ftp.ncbi.nlm.nih.gov/genomes/Homo_sapiens/CHR_*/hs_ref_GRCh38.p7_*.fa.gz .
    echo " complete."

    # We know all sequences are human, so we can cheat and 
    # assign the taxid here directly
    
    # Empty the seqid file
    echo > $SEQ2TAXID

    human_taxid=9606
    echo -n "Mapping seqid to taxid..."
    for file in hs_ref_GRCh38.p7_*.fa.gz; do
      zcat $file | grep '^>' |
        while read header; do
          header=`echo $header | cut -d ' ' -f 1`
          echo -e "${header:1}\t$human_taxid" >> $SEQ2TAXID
        done
    done

    echo " complete."
    ;;
  *)
    mkdir -p $LIBRARY_DIR/$1
    cd $LIBRARY_DIR/$1
    wget -q ftp://ftp.ncbi.nlm.nih.gov/genomes/refseq/$1/assembly_summary.txt
    #### DEBUG START ####
    echo "WARNING: DEBUG ENABLED, DOWNLOADING ONLY 100 SAMPLES PER LIBRARY"
    head -n 100 assembly_summary.txt > t
    mv t assembly_summary.txt
    #### DEBUG END ####
    download
    seqid2taxid
    ;;
esac

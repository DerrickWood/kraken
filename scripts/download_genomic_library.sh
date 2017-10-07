#!/bin/bash

# Copyright 2013-2017, Derrick Wood <dwood@cs.jhu.edu>
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
# Supported choices are:
#   archaea - NCBI RefSeq complete archaeal genomes
#   bacteria - NCBI RefSeq complete bacterial genomes
#   plasmids - NCBI RefSeq plasmid sequences
#   viral - NCBI RefSeq complete viral DNA and RNA genomes
#   human - NCBI RefSeq GRCh38 human reference genome

set -u  # Protect against uninitialized vars.
set -e  # Stop on error

LIBRARY_DIR="$KRAKEN_DB_NAME/library"
NCBI_SERVER="ftp.ncbi.nlm.nih.gov"
FTP_SERVER="ftp://$NCBI_SERVER"
RSYNC_SERVER="rsync://$NCBI_SERVER"
THIS_DIR=$PWD

library_name="$1"
library_file="library.fna"
if [ -e "$LIBRARY_DIR/$library_name/.completed" ]; then
  echo "Skipping $library_name, already completed library download"
  exit 0
fi
case "$1" in
  "archaea" | "bacteria" | "viral" | "human" )
    mkdir -p $LIBRARY_DIR/$library_name
    cd $LIBRARY_DIR/$library_name
    rm -f assembly_summary.txt
    remote_dir_name=$library_name
    if [ "$library_name" = "human" ]; then
      remote_dir_name="vertebrate_mammalian/Homo_sapiens"
    fi
    if ! wget -q $FTP_SERVER/genomes/refseq/$remote_dir_name/assembly_summary.txt; then
      echo "Error downloading assembly summary file for $library_name, exiting." >/dev/fd/2
      exit 1
    fi
    if [ "$library_name" = "human" ]; then
      grep "Genome Reference Consortium" assembly_summary.txt > x
      mv x assembly_summary.txt
    fi
    rm -rf all/ library.f* manifest.txt rsync.err
    rsync_from_ncbi.pl assembly_summary.txt
    scan_fasta_file.pl $library_file > prelim_map.txt
    touch .completed
    ;;
  "plasmid")
    mkdir -p $LIBRARY_DIR/plasmid
    cd $LIBRARY_DIR/plasmid
    rm -f library.f* plasmid.*
    echo -n "Downloading plasmid files from FTP..."
    wget -q --no-remove-listing --spider $FTP_SERVER/genomes/refseq/plasmid/
    awk '{ print $NF }' .listing | perl -ple 'tr/\r//d' | grep '\.fna\.gz' > manifest.txt
    cat manifest.txt | xargs -n1 -I{} wget -q $FTP_SERVER/genomes/refseq/plasmid/{}
    cat manifest.txt | xargs -n1 -I{} gunzip -c {} > $library_file
    rm -f plasmid.* .listing
    scan_fasta_file.pl $library_file > prelim_map.txt
    touch .completed
    echo " done."
    ;;
  *)
    echo "Unsupported library.  Valid options are: "
    echo "  archaea bacteria plasmid viral human"
    ;;
esac

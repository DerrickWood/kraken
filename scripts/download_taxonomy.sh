#!/bin/bash

# Copyright 2013-2017, Derrick Wood <dwood@cs.jhu.edu>
#
# This file is part of the Kraken taxonomic sequence classification system.

# Download NCBI taxonomy information for Kraken.
# Designed to be called by kraken-build

set -u  # Protect against uninitialized vars.
set -e  # Stop on error

TAXONOMY_DIR="$KRAKEN_DB_NAME/taxonomy"
NCBI_SERVER="ftp.ncbi.nlm.nih.gov"
FTP_SERVER="ftp://$NCBI_SERVER"

mkdir -p "$TAXONOMY_DIR"
cd "$TAXONOMY_DIR"

if [ ! -e "accmap.dlflag" ]
then
  wget $FTP_SERVER/pub/taxonomy/accession2taxid/nucl_est.accession2taxid.gz
  wget $FTP_SERVER/pub/taxonomy/accession2taxid/nucl_gb.accession2taxid.gz
  wget $FTP_SERVER/pub/taxonomy/accession2taxid/nucl_gss.accession2taxid.gz
  wget $FTP_SERVER/pub/taxonomy/accession2taxid/nucl_wgs.accession2taxid.gz
  touch accmap.dlflag
  echo "Downloaded accession to taxon map(s)"
fi

if [ ! -e "taxdump.dlflag" ]
then
  wget $FTP_SERVER/pub/taxonomy/taxdump.tar.gz
  touch taxdump.dlflag
  echo "Downloaded taxonomy tree data"
fi

if ls | grep -q 'accession2taxid\.gz$'
then
  echo -n "Uncompressing taxonomy data... "
  gunzip *accession2taxid.gz
  echo "done."
fi

if [ ! -e "taxdump.untarflag" ]
then
  echo -n "Untarring taxonomy tree data... "
  tar zxf taxdump.tar.gz
  touch taxdump.untarflag
  echo "done."
fi

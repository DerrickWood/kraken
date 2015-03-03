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

# Build a 16S database from Greengenes data

set -u  # Protect against uninitialized vars.
set -e  # Stop on error
set -o pipefail  # Stop on failures in non-final pipeline commands

FTP_SERVER="ftp://greengenes.microbio.me/"
GG_VERSION="gg_13_5"
REMOTE_DIR="$FTP_SERVER/greengenes_release/$GG_VERSION"

check_for_jellyfish.sh

mkdir -p "$KRAKEN_DB_NAME"
pushd "$KRAKEN_DB_NAME"
mkdir -p data taxonomy library
pushd data
wget "$REMOTE_DIR/${GG_VERSION}.fasta.gz"
gunzip "${GG_VERSION}.fasta.gz"
wget "$REMOTE_DIR/${GG_VERSION}_taxonomy.txt.gz"
gunzip "${GG_VERSION}_taxonomy.txt.gz"

build_gg_taxonomy.pl "${GG_VERSION}_taxonomy.txt"
popd
mv data/names.dmp data/nodes.dmp taxonomy/
mv data/seqid2taxid.map .
mv "data/${GG_VERSION}.fasta" library/gg.fa
touch gi2seqid.map  # skip GI->seq ID map
popd

kraken-build --db $KRAKEN_DB_NAME --build --threads $KRAKEN_THREAD_CT \
               --minimizer-len 12 --jellyfish-hash-size 100M

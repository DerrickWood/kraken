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

# Build a 16S database from RDP data

set -u  # Protect against uninitialized vars.
set -e  # Stop on error
set -o pipefail  # Stop on failures in non-final pipeline commands

HTTP_SERVER="http://rdp.cme.msu.edu/"
REMOTE_DIR="$HTTP_SERVER/download/"

check_for_jellyfish.sh

mkdir -p "$KRAKEN_DB_NAME"
pushd "$KRAKEN_DB_NAME"
mkdir -p data taxonomy library
pushd data
wget "$REMOTE_DIR/current_Bacteria_unaligned.fa.gz"
gunzip "current_Bacteria_unaligned.fa.gz"
wget "$REMOTE_DIR/current_Archaea_unaligned.fa.gz"
gunzip "current_Archaea_unaligned.fa.gz"

build_rdp_taxonomy.pl current_*_unaligned.fa
popd
mv data/names.dmp data/nodes.dmp taxonomy/
mv data/seqid2taxid.map .
mv data/*.fa library/
touch gi2seqid.map  # skip GI->seq ID map
popd

kraken-build --db $KRAKEN_DB_NAME --build --threads $KRAKEN_THREAD_CT \
               --minimizer-len 12 --jellyfish-hash-size 100M

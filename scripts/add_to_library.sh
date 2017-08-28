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

# Copy specified file into a Kraken library

set -u  # Protect against uninitialized vars.
set -e  # Stop on error

LIBRARY_DIR="$KRAKEN_DB_NAME/library"

if [ ! -e "$1" ]
then
  echo "Can't add \"$1\": file does not exist"
  exit 1
fi
if [ ! -f "$1" ]
then
  echo "Can't add \"$1\": not a regular file"
  exit 1
fi

add_dir="$LIBRARY_DIR/added"
mkdir -p "$add_dir"
scan_fasta_file.pl "$1" > "$add_dir/temp_map.txt"

filename=$(cp_into_tempfile.pl -t "XXXXXXXXXX" -d "$add_dir" -s fna "$1")

cat "$add_dir/temp_map.txt" >> "$KRAKEN_DB_NAME/taxonomy/prelim_map.txt"
rm "$add_dir/temp_map.txt"

echo "Added \"$1\" to library ($KRAKEN_DB_NAME)"

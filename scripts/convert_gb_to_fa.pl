#!/usr/bin/env perl

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

# Pull sequence data with accession and taxid from Genbank file and output in fasta format  
# Adapted from @tseemann https://github.com/MDU-PHL/mdu-tools/blob/master/bin/genbank-to-kraken_fasta.pl

use strict;
use warnings;

@ARGV or die "Usage: $0 <file.gbk[.gz]> ...";

my $wrote=0;
my($seqid, $in_seq, $taxid);

my $input_file = $ARGV[0];

open(IN, "gunzip -c -f \Q$input_file\E |") or die "can’t open pipe to $input_file";

while (<IN>) {
  if (m/^VERSION\s+(\S+)/) {
    $seqid = $1;
  }
  elsif (m/taxon:(\d+)/) {
    $taxid = $1;
  }
  elsif (m/^ORIGIN/) {
    $in_seq = 1;
    print ">$seqid|kraken:taxid|$taxid\n";
  }
  elsif (m{^//}) {
    $in_seq = $taxid = $seqid = undef;
    $wrote++;
  }
  elsif ($in_seq) {
    substr $_, 0, 10, '';
    s/\s//g;
    print uc($_), "\n";
  }
}

close IN;


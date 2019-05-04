/*
 * Copyright 2013-2017, Derrick Wood, Jennifer Lu <jlu26@jhmi.edu>
 *
 * This file is part of the Kraken taxonomic sequence classification system.
 *
 * Kraken is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Kraken is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Kraken.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "kraken_headers.hpp"
#include "krakenutil.hpp"
#include "seqreader.hpp"

#define SKIP_LEN 50000

using namespace std;
using namespace kraken;

void parse_command_line(int argc, char **argv);
void usage(int exit_code=EX_USAGE);
uint64_t obtain_estimated_kmer_ct();

int Num_threads = 1;
int k = 0;
double multiplier = 1.0;

// MurmurHash3 finalizer
uint64_t hash_code(uint64_t h) {
  h ^= h >> 33;
  h *= 0xff51afd7ed558ccd;
  h ^= h >> 33;
  h *= 0xc4ceb9fe1a85ec53;
  h ^= h >> 33;
  return h;
}

int main(int argc, char **argv) {
  #ifdef _OPENMP
  omp_set_num_threads(1);
  #endif

  parse_command_line(argc, argv);

  KmerScanner::set_k(k);
  uint64_t estimate = multiplier * obtain_estimated_kmer_ct();
  cout << estimate << endl;

  return 0;
}

uint64_t obtain_estimated_kmer_ct() {
  FastaReader reader("/dev/fd/0");
  DNASequence dna;
  set<uint64_t> qualified_kmers;
  uint64_t qualification_limit = 4;
  uint64_t mod_value = 4096;
  
  while (true) {
    dna = reader.next_sequence();
    if (! reader.is_valid())
      break;
    #pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < dna.seq.size(); i += SKIP_LEN) {
      KmerScanner scanner(dna.seq, i, i + SKIP_LEN + k - 1);
      uint64_t *kmer_ptr;

      while ((kmer_ptr = scanner.next_kmer()) != NULL) {
        if (scanner.ambig_kmer())
          continue;
        uint64_t hc = hash_code(*kmer_ptr);
        if (hc % mod_value < qualification_limit) {
          #pragma omp critical(set_access)
          qualified_kmers.insert(*kmer_ptr);
        }
      }
    }
  }
  double estimate = (qualified_kmers.size() + 2) * ((double) mod_value / qualification_limit);
  return (uint64_t) estimate;
}

void parse_command_line(int argc, char **argv) {
  int opt;
  long long sig;

  if (argc > 1 && strcmp(argv[1], "-h") == 0)
    usage(0);
  while ((opt = getopt(argc, argv, "t:k:m:")) != -1) {
    switch (opt) {
      case 't' :
        sig = atoll(optarg);
        if (sig <= 0)
          errx(EX_USAGE, "can't use nonpositive thread count");
        #ifdef _OPENMP
        if (sig > omp_get_num_procs())
          errx(EX_USAGE, "thread count exceeds number of processors");
        Num_threads = sig;
        omp_set_num_threads(Num_threads);
        #endif
        break;
      case 'k' :
        sig = atoll(optarg);
        if (sig <= 0)
          errx(EX_USAGE, "k can't be <= 0");
        if (sig > 31)
          errx(EX_USAGE, "k can't be > 31");
        k = sig;
        break;
      case 'm' :
        multiplier = atof(optarg);
        break;
      default:
        usage();
        break;
    }
  }

  if (k == 0)
    usage(EX_USAGE);
}

void usage(int exit_code) {
  cerr << "Usage: estimator [options]" << endl
       << endl
       << "Options: (*mandatory)" << endl
       << "* -k #          Length of k-mers" << endl
       << "  -t #          Number of threads" << endl
       << "  -m FLOAT      Multiplier" << endl
       << "  -h            Print this message" << endl;
  exit(exit_code);
}

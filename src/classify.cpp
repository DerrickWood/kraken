/*
 * Copyright 2013-2015, Derrick Wood <dwood@cs.jhu.edu>
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
#include "krakendb.hpp"
#include "krakenutil.hpp"
#include "quickfile.hpp"
#include "seqreader.hpp"

const size_t DEF_WORK_UNIT_SIZE = 500000;

using namespace std;
using namespace kraken;

void parse_command_line(int argc, char **argv);
void usage(int exit_code=EX_USAGE);
void process_file(char *filename, ostream* kraken_output);
void classify_sequence(DNASequence &dna, ostringstream &koss,
                       ostringstream &coss, ostringstream &uoss);
string hitlist_string(vector<uint32_t> &taxa, vector<uint8_t> &ambig);
set<uint32_t> get_ancestry(uint32_t taxon);
double elapsed_seconds(struct timeval time1, struct timeval time2);
void report_stats(struct timeval time1, struct timeval time2);

int Num_threads = 1;
string DB_filename, Index_filename, Nodes_filename;
size_t DB_size = 0;
size_t Index_size = 0;
bool Pipe_DB = false;
bool Quick_mode = false;
bool Fastq_input = false;
bool Print_classified = false;
bool Print_unclassified = false;
bool Populate_memory = false;
bool Only_classified_kraken_output = false;
uint32_t Minimum_hit_count = 1;
map<uint32_t, uint32_t> Parent_map;
KrakenDB Database;
string Classified_output_file, Unclassified_output_file;
vector<string> Kraken_output_files;
ostream *Classified_output;
ostream *Unclassified_output;
vector<ostream *> Kraken_outputs;
size_t Work_unit_size = DEF_WORK_UNIT_SIZE;

uint64_t total_classified = 0;
uint64_t total_sequences = 0;
uint64_t total_bases = 0;

int main(int argc, char **argv) {
  #ifdef _OPENMP
  omp_set_num_threads(1);
  #endif

  parse_command_line(argc, argv);
  if (! Nodes_filename.empty())
    Parent_map = build_parent_map(Nodes_filename);

  struct timeval dtv1, dtv2;
  gettimeofday(&dtv1, NULL);

  QuickFile db_file;
  if (!Pipe_DB) {
    if (Populate_memory)
      cerr << "Loading database... ";

    db_file.open_file(DB_filename);
    if (Populate_memory)
      db_file.load_file();
    Database = KrakenDB(db_file.ptr());
  } else {
    ifstream in(DB_filename.c_str(), ios::in | ios::binary);
    if (!in) {
      cerr << "Couldn't open file " << DB_filename;
      exit(EXIT_FAILURE);
    }
    char* buffer;
    if (DB_size > 0) {
      buffer = (char*) malloc(DB_size);
      in.read(buffer, DB_size);
    } else {
      string db_string((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
      buffer = (char*)db_string.c_str();
    }
    Database = KrakenDB(buffer);
  }

  KmerScanner::set_k(Database.get_k());

  KrakenDBIndex db_index;
  QuickFile idx_file;
  if (!Pipe_DB) {
    idx_file.open_file(Index_filename);
    if (Populate_memory)
      idx_file.load_file();
    db_index = KrakenDBIndex(idx_file.ptr());
  } else {
    ifstream in(Index_filename.c_str(), ios::in | ios::binary);
    if (!in) {
      cerr << "Couldn't open file " << Index_filename;
      exit(EXIT_FAILURE);
    }
    char* buffer;
    if (Index_size > 0) {
      buffer = (char*) malloc(Index_size);
      in.read(buffer, Index_size);
    } else {
      string index_string((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
      buffer = (char*) index_string.c_str();
    }
    db_index = KrakenDBIndex(buffer);
  }

  Database.set_index(&db_index);

  if (Populate_memory)
    cerr << "complete." << endl;

  gettimeofday(&dtv2, NULL);
  double seconds = elapsed_seconds(dtv1, dtv2);
  fprintf(stderr,
          "database loaded in %dm %.2fs. \n",
          (int) seconds / 60,  fmod(seconds, 60));

  if (Print_classified) {
    if (Classified_output_file == "-")
      Classified_output = &cout;
    else
      Classified_output = new ofstream(Classified_output_file.c_str());
  }

  if (Print_unclassified) {
    if (Unclassified_output_file == "-")
      Unclassified_output = &cout;
    else
      Unclassified_output = new ofstream(Unclassified_output_file.c_str());
  }

  for (vector<string>::iterator it = Kraken_output_files.begin(); it != Kraken_output_files.end(); ++it) {
    if (*it == "-") {
      Kraken_outputs.push_back(&cout);
    } else {
      ostream* kraken_output = new ofstream((*it).c_str());
      Kraken_outputs.push_back(kraken_output);
    }
  }

  size_t n_inputs = argc - optind;
  size_t n_outputs = Kraken_outputs.size();
  if (n_outputs && n_inputs != n_outputs) {
    cerr << "Number of output files doesn't match inputs";
    exit(EXIT_FAILURE);
  }

  int j = 0;
  for (int i = optind; i < argc; i++) {
    struct timeval tv1, tv2;
    gettimeofday(&tv1, NULL);

    total_classified = 0;
    total_sequences = 0;
    total_bases = 0;
    ostream* kraken_output;
    if (!n_outputs) {
      kraken_output = &cout;
    } else {
      kraken_output = Kraken_outputs[j];
    }
    process_file(argv[i], kraken_output);
    j++;
    gettimeofday(&tv2, NULL);

    report_stats(tv1, tv2);
  }

  return 0;
}

double elapsed_seconds(struct timeval time1, struct timeval time2) {
  time2.tv_usec -= time1.tv_usec;
  time2.tv_sec -= time1.tv_sec;
  if (time2.tv_usec < 0) {
    time2.tv_sec--;
    time2.tv_usec += 1000000;
  }
  double seconds = time2.tv_usec;
  seconds /= 1e6;
  seconds += time2.tv_sec;
  return seconds;
}

void report_stats(struct timeval time1, struct timeval time2) {
  double seconds = elapsed_seconds(time1, time2);

  if (isatty(fileno(stderr)))
    cerr << "\r";
  fprintf(stderr,
          "%llu sequences (%.2f Mbp) processed in %.3fs (%.1f Kseq/m, %.2f Mbp/m).\n",
          (unsigned long long) total_sequences, total_bases / 1.0e6, seconds,
          total_sequences / 1.0e3 / (seconds / 60),
          total_bases / 1.0e6 / (seconds / 60) );
  fprintf(stderr, "  %llu sequences classified (%.2f%%)\n",
          (unsigned long long) total_classified, total_classified * 100.0 / total_sequences);
  fprintf(stderr, "  %llu sequences unclassified (%.2f%%)\n",
          (unsigned long long) (total_sequences - total_classified),
          (total_sequences - total_classified) * 100.0 / total_sequences);
}

void process_file(char *filename, ostream* kraken_output) {
  string file_str(filename);
  DNASequenceReader *reader;
  DNASequence dna;

  if (Fastq_input)
    reader = new FastqReader(file_str);
  else
    reader = new FastaReader(file_str);

  #pragma omp parallel
  {
    vector<DNASequence> work_unit;
    ostringstream kraken_output_ss, classified_output_ss, unclassified_output_ss;

    while (reader->is_valid()) {
      work_unit.clear();
      size_t total_nt = 0;
      #pragma omp critical(get_input)
      {
        while (total_nt < Work_unit_size) {
          dna = reader->next_sequence();
          if (! reader->is_valid())
            break;
          work_unit.push_back(dna);
          total_nt += dna.seq.size();
        }
      }
      if (total_nt == 0)
        break;

      kraken_output_ss.str("");
      classified_output_ss.str("");
      unclassified_output_ss.str("");
      for (size_t j = 0; j < work_unit.size(); j++)
        classify_sequence( work_unit[j], kraken_output_ss,
                           classified_output_ss, unclassified_output_ss );

      #pragma omp critical(write_output)
      {
        (*kraken_output) << kraken_output_ss.str();
        if (Print_classified)
          (*Classified_output) << classified_output_ss.str();
        if (Print_unclassified)
          (*Unclassified_output) << unclassified_output_ss.str();
        total_sequences += work_unit.size();
        total_bases += total_nt;
        if (isatty(fileno(stderr)))
          cerr << "\rProcessed " << total_sequences << " sequences (" << total_bases << " bp) ...";
      }
    }
  }  // end parallel section

  delete reader;
  (*kraken_output) << std::flush;
  if (Print_classified)
    (*Classified_output) << std::flush;
  if (Print_unclassified)
    (*Unclassified_output) << std::flush;
}

void classify_sequence(DNASequence &dna, ostringstream &koss,
                       ostringstream &coss, ostringstream &uoss) {
  vector<uint32_t> taxa;
  vector<uint8_t> ambig_list;
  map<uint32_t, uint32_t> hit_counts;
  uint64_t *kmer_ptr;
  uint32_t taxon = 0;
  uint32_t hits = 0;  // only maintained if in quick mode

  uint64_t current_bin_key;
  int64_t current_min_pos = 1;
  int64_t current_max_pos = 0;

  if (dna.seq.size() >= Database.get_k()) {
    KmerScanner scanner(dna.seq);
    while ((kmer_ptr = scanner.next_kmer()) != NULL) {
      taxon = 0;
      if (scanner.ambig_kmer()) {
        ambig_list.push_back(1);
      }
      else {
        ambig_list.push_back(0);
        uint32_t *val_ptr = Database.kmer_query(
                              Database.canonical_representation(*kmer_ptr),
                              &current_bin_key,
                              &current_min_pos, &current_max_pos
                            );
        taxon = val_ptr ? *val_ptr : 0;
        if (taxon) {
          hit_counts[taxon]++;
          if (Quick_mode && ++hits >= Minimum_hit_count)
            break;
        }
      }
      taxa.push_back(taxon);
    }
  }

  uint32_t call = 0;
  if (Quick_mode)
    call = hits >= Minimum_hit_count ? taxon : 0;
  else
    call = resolve_tree(hit_counts, Parent_map);

  if (call)
    #pragma omp atomic
    total_classified++;

  if (Print_unclassified || Print_classified) {
    ostringstream *oss_ptr = call ? &coss : &uoss;
    bool print = call ? Print_classified : Print_unclassified;
    if (print) {
      if (Fastq_input) {
        (*oss_ptr) << "@" << dna.header_line << endl
            << dna.seq << endl
            << "+" << endl
            << dna.quals << endl;
      }
      else {
        (*oss_ptr) << ">" << dna.header_line << endl
            << dna.seq << endl;
      }
    }
  }

  if (call) {
    koss << "C\t";
  }
  else {
    if (Only_classified_kraken_output)
      return;
    koss << "U\t";
  }
  koss << dna.id << "\t" << call << "\t" << dna.seq.size() << "\t";

  if (Quick_mode) {
    koss << "Q:" << hits;
  }
  else {
    if (taxa.empty())
      koss << "0:0";
    else
      koss << hitlist_string(taxa, ambig_list);
  }

  koss << endl;
}

string hitlist_string(vector<uint32_t> &taxa, vector<uint8_t> &ambig)
{
  int64_t last_code;
  int code_count = 1;
  ostringstream hitlist;

  if (ambig[0])   { last_code = -1; }
  else            { last_code = taxa[0]; }

  for (size_t i = 1; i < taxa.size(); i++) {
    int64_t code;
    if (ambig[i]) { code = -1; }
    else          { code = taxa[i]; }

    if (code == last_code) {
      code_count++;
    }
    else {
      if (last_code >= 0) {
        hitlist << last_code << ":" << code_count << " ";
      }
      else {
        hitlist << "A:" << code_count << " ";
      }
      code_count = 1;
      last_code = code;
    }
  }
  if (last_code >= 0) {
    hitlist << last_code << ":" << code_count;
  }
  else {
    hitlist << "A:" << code_count;
  }
  return hitlist.str();
}

set<uint32_t> get_ancestry(uint32_t taxon) {
  set<uint32_t> path;

  while (taxon > 0) {
    path.insert(taxon);
    taxon = Parent_map[taxon];
  }
  return path;
}

void parse_command_line(int argc, char **argv) {
  int opt;
  long long sig;

  if (argc > 1 && strcmp(argv[1], "-h") == 0)
    usage(0);
  while ((opt = getopt(argc, argv, "d:i:D:I:t:u:n:m:o:pqfcC:U:M")) != -1) {
    switch (opt) {
      case 'd' :
        DB_filename = optarg;
        break;
      case 'i' :
        Index_filename = optarg;
        break;
      case 'D' :
        DB_size = atoll(optarg);
        break;
      case 'I' :
        Index_size = atoll(optarg);
        break;
      case 'p' :
        Pipe_DB = true;
        break;
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
      case 'n' :
        Nodes_filename = optarg;
        break;
      case 'q' :
        Quick_mode = true;
        break;
      case 'm' :
        sig = atoll(optarg);
        if (sig <= 0)
          errx(EX_USAGE, "can't use nonpositive minimum hit count");
        Minimum_hit_count = sig;
        break;
      case 'f' :
        Fastq_input = true;
        break;
      case 'c' :
        Only_classified_kraken_output = true;
        break;
      case 'C' :
        Print_classified = true;
        Classified_output_file = optarg;
        break;
      case 'U' :
        Print_unclassified = true;
        Unclassified_output_file = optarg;
        break;
      case 'o' :
        Kraken_output_files.push_back(optarg);
        break;
      case 'u' :
        sig = atoll(optarg);
        if (sig <= 0)
          errx(EX_USAGE, "can't use nonpositive work unit size");
        Work_unit_size = sig;
        break;
      case 'M' :
        Populate_memory = true;
        break;
      default:
        usage();
        break;
    }
  }

  if (DB_filename.empty()) {
    cerr << "Missing mandatory option -d" << endl;
    usage();
  }
  if (Index_filename.empty()) {
    cerr << "Missing mandatory option -i" << endl;
    usage();
  }
  if (Nodes_filename.empty() && ! Quick_mode) {
    cerr << "Must specify one of -q or -n" << endl;
    usage();
  }
  if (optind == argc) {
    cerr << "No sequence data files specified" << endl;
  }
}

void usage(int exit_code) {
  cerr << "Usage: classify [options] <fasta/fastq file(s)>" << endl
       << endl
       << "Options: (*mandatory)" << endl
       << "* -d filename      Kraken DB filename" << endl
       << "* -i filename      Kraken DB index filename" << endl
       << "  -D filename      Kraken DB size" << endl
       << "  -I filename      Kraken DB index size" << endl
       << "  -p               Kraken DB are pipes" << endl
       << "  -n filename      NCBI Taxonomy nodes file" << endl
       << "  -o filename      Output file for Kraken output" << endl
       << "  -t #             Number of threads" << endl
       << "  -u #             Thread work unit size (in bp)" << endl
       << "  -q               Quick operation" << endl
       << "  -m #             Minimum hit count (ignored w/o -q)" << endl
       << "  -C filename      Print classified sequences" << endl
       << "  -U filename      Print unclassified sequences" << endl
       << "  -f               Input is in FASTQ format" << endl
       << "  -c               Only include classified reads in output" << endl
       << "  -M               Preload database files" << endl
       << "  -h               Print this message" << endl
       << endl
       << "At least one FASTA or FASTQ file must be specified." << endl
       << "Kraken output is to standard output by default." << endl;
  exit(exit_code);
}

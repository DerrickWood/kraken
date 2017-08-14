Kraken taxonomic sequence classification system
===============================================

Please see the [Kraken webpage] or the [Kraken manual]
for information on installing and operating Kraken.
A local copy of the [Kraken manual] is also present here
in the `docs/` directory (`MANUAL.html` and `MANUAL.markdown`).

[Kraken webpage]:   http://ccb.jhu.edu/software/kraken/
[Kraken manual]:    http://ccb.jhu.edu/software/kraken/MANUAL.html

Changelog accession
-------------------
* Made the download function compatible with NCBI accession numbers
* No longer unpack fasta files on disk, use zcat instead
* Added support for all databases listed at ftp.ncbi.nlm.nih.gov/genomes/refseq/
* Added support for Human
* Removed support for plasmids
* Use rsync instead of wget for all downloads
* Removed the taxon-to-seqid mapping step completely
* Extract the taxon information directly from assembly\_summary.txt

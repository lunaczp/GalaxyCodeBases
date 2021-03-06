1 Introduction
    pIRS is a program for simulating Illumina PE reads, with a series of characters generated by 
  Illumina sequencing platform, such as insert size distribution, sequencing error, quality score 
  and GC bias. 
    As all know, the insert size follows a normal distribution, so users should set the mean value 
  and standard deviation. We usually set the standard deviation to be 1/20 of the mean value. We 
  simulate the normal distribution by Box-Muller method. 
    The program simulates sequencing error, quality score and GC bias according to the empirical 
  distribution profile. We provide some default profiles counted from lots of real sequencing data.
    To simulate reads from diploid genome, users should simulate the diploid genome sequence firstly 
  by setting the ratio of heterozygosis SNP, heterozygosis InDel and structure variation. 
    At last, several another should be determined: whether simulate GC bias and quality scores, in 
  addition, the read length, coverage depth, input reference sequence, output prefix and output type.

2 Program framework
  2.1 Profile Generator
    Five tools are supplied. SOAP2 or BWA, soap.coverage (http://soap.genomics.org.cn/soapaligner.html) 
  are required. The full process is shown in getprofile.sh as an example.
    2.1.1 GC-Depth Stat.
      a). run soap and soap.coverage to get .depth single file(s). gzip is OK to over it.
      b). run gc_coverage_bias on all depth single files. You will get gc-depth stat by 1 GC% and other files.
      c). run gc_coverage_bias_plot on the gc-depth stat file. You'll get PNG plot and a .gc file by 5 GC%.
      d). Manually check the .gc file for any abnormal levels due to the lower depth on certain GC% windows.
    2.1.2 Error Matrix Stat:
      a). run soap or bwa to get .{soap,single} or .sam file(s).
      b). run error_matrix_calculator on those file(s). You will get *.{count,ratio}.matrix .
      c). You can use error_matrix_merger to merge several .{count,ratio}.matrix files.
         However, it is up to you to keep the read length matches.
    2.1.3 Insert size & mapping ratio stat:
      a). run soap or bwa to get .{soap,single} or .sam file(s).
      b). run alignment_stator *.
         * alignment_stator cannot stat. mapping ratio for sam files now.
  2.2 Simulator
    Two executable file:
    simulate_diploid_genome: use for simulating diploid genome sequence. Read the input genome sequence and 
                             then simulate SNP, InDel, SV(structure variation) on it. At last, output the 
                             result genome sequence.
    simulate_illumina_reads: use for simulatingIllumina data, output PE-read file.
    Note:
      a) If you only want to simulate the diploid genome sequence, "simulate_diploid_genome" is enough.
      b) If you want to simulate sequencing data of haploid genome, only you need is "simulate_Illumina_reads".
      c) If you want to simulate sequencing data of diploid genome, you first need to run "simu-late_diploid_genome" 
         to get the other diploid genome sequence, and then run "simulate_Illumina_reads" using both the original 
         genome sequence and the previous output sequence as the input.

3 Usage
  3.1 simulate_diploid_genome
    Simulate the diploid genome sequence
    
    Usageㄩsimulate_diploid_genome [OPTIONS]
      -i <string> input reference sequence (file.fa / file.fa.gz)
      -s <double> heterozygous SNP rate (default=0.001)
      -a <double> value of transition divided by transvertion for heterSNP (default=2)
      -d <double> small heterozygous InDel rate (default=0.0001)
      -v <double> structure variation (large insertion, deletion, invertion) rate (default=1e-6)
      -c <int>    output file type, 0: text, 1: gz compressed (default=1)
      -o <string> output prefix (default=ref_sequence)
      -h          output help information and exit
      
  3.2 simulate_Illumina_reads
    Simulate sequencing data of Illumina sequencing platform
    
    Usageㄩsimulate_illumina_reads [OPTIONS]
      -i <string> input reference sequence (file.fa / file.fa.gz)
      -I <string> another input reference for diploid genome (generated by "simu-late_diploid_genome", 
                  set only when simulating reads of diploid genome)
      -s <string> input Base-calling profile (default=$Bin/Profiles/Base-calling_profile/hum20110701.bwanosnp.count.matrix)
      -d <string> input GC-depth profile for simulating GC bias, the default GC bias pro-files are determined 
                  based on the twice of read length
      -l <int>    read length, read1 and read2 are the same length (default=100)
      -x <double> sequencing coverage depth (default=40)
      -m <int>    average insert size (default=500)
      -v <int>    the standard deviation of insert size (default=5% of average insert size)
      -e <double> average error rate (default=average error rate of the Base-calling profile)
      -g <int>    whether simulate GC bias, 0: no, 1: yes (default=1)
      -q <int>    whether simulate quality value, 0: no, 1: yes (default=1)
      -Q <int>    Quality value ASCII shift, generally 64 or 33 for Illumina data, default:64
      -f <int>    whether cyclize insert fragment (influence on PE reads＊ direction) 0:read1-forward, read2-reverse, 
                  1: contrary to parameter 0 (default=0)
      -c <int>    output file type, 0: text, 1: gz compressed (default=1)
      -o <string> output prefix (default=Illumina)
      -h          output help information and exit

4 Output files
  4.1 Simulation result of diploid genome sequence.
    Example command line:
    simulate_diploid_genome 每iHuman_ref.fa 每s 0.001 每a 2 每d 0.00001 每v 0.000001 每o Human >simulate_seq.o 2>simulate_seq.e
    Output files:
      a) Human.snp.indel.invertion.fa.gz: another diploid genome sequence.
      b) Human_indel.lst: InDel information list.
      c) Human_snp.lst:SNP information list.
      d) Human_invertion.lst:invertion information list.
      e) simulate_seq.o, simulate_seq.e: records of the program running information.
  4.2 Simulation result of Illumina sequencing data for haploid genome.
    Example command line:
    simulate_Illumina_reads -iHuman_ref.fa -m 170 -l 90 -x 5 -v 10 -o Hu-man>simulate_170.o 2>simulate_170.e
    Output files:
      a) Human_90_170_1.fq.gz, Human_90_170_2.fq.gz: the pair-end read files.
      b) Human_90_170.error_rate.distr: the error distribution file.
      c) Human_90_170.insertsize.distr: the insert size distribution file.
      d) simulate_170.o, simulate_170.e: records of the program running information.
  4.3 Simulation result ofIllumina sequencing data for diploid genome.
    Example command line:
    simulate_Illumina_reads -i Human_ref.fa -I Human.snp.indel.invertion.fa.gz -m 800 -l 70 -x 5 -v 10 
                            -o Human >simulate_800.o 2>simulate_800.e
    Output files:
      a) Human_70_800_1.fq.gz, Human_70_800_2.fq.gz: the pair-end read files.
      b) Human_70_800.error_rate.distr: the error distribution file.
      c) Human_70_800.insertsize.distr: the insert size distribution file.
      d) simulate_800.e, simulate_800.o: records of the program running information.

5 Output file format
  a) *.fq/*.fq.gz
    For example: the output file name: *_ 70_800_1.fq.gz, it mean 70bp reads, the average insert size length is 800bp.
    In read file1:

      @read_800_21/1 seq 1 856549 70 813
      ACGGAAAAGTTACGCTATCGCATGCGTGTAAGAACACTGCTCCTACGCCCATTTTATCGATGGCGCCCAG
      +
      egcggdggfgfgggggfeggggYbcgegfgggbggg^e]egfegggfbSeggdggegg`^eJgggcbEeb
      @read_800_22/1 seq 2 129025 70 786 32,C;70,A;
      CACGGGGGGACTTTATTTAATGAGCGGCTGTAACTTGGTCCGTCGTTTGAGAGGGGACACCTCATATGAT
      +
      gggggegcgeggggcgfcgc_gf_ggfefcgVgegcfcdgf`geggdd[ge`ggafeggggdgdgee^gg

    note: 
    In Row-1, "800" represent for the mean insert size; "21" represent for the order of read in file; 
    The next "1" represent for read file1; "seq" represent for id of chromosome; The next "1" represent for 
    this read come from the reference sequence which is input by -i, for simulated data of haploid genome, 
    it will be always "1", for simulated data of diploid genome, "1" represent for this read generates from 
    the reference sequence which is input by -i, "2" represent for this read generates from the reference 
    sequence which is input by -I; "856549" represent for the start position(1-based) in the chromosome; 
    "70" represent for the length of read; "813" represent for the insert size.
    In Row-5, "32,C;70,A;" represent for two sequencing errors in this read, "32" and "74" represent for 
    the error positions(1-based); "C" and "A" represent for the right Bases.
  b) *_snp.lst
    For example:
    
      I	3	G	A 
      I	45342	C	T 
      I	104775	C	T
      ＃＃
    Column 1: chromosome sequence ID.
    Column 2: SNP position(1-based).
    Column 3: Base on raw chromosome sequence.
    Column 4: SNP-Base.

      I 3 G A
      Ref: a t G c a // 3 is the position of G base in the chromosome base.
         : a t A c a // G base is a A in another diploid sequence. 

  c) *_indel.lst
    For example:
      I  - 3 1 C
      IV + 5 2 AC
      ＃＃
    Column 1: chromosomesequence ID.
    Column 2: "-" Deletion; "+" Insertion.
    Column 3: InDel position (1-based) of raw chromosome sequence. Note that this position represent 
              for the prev-base of InDel region.
    Column 4: InDel Base number.
    Column 5: InDel Base.

      I - 3 1 C
      Ref: a t G c a // 3 is the position of G base in the chromosome base.
         : a t G - a // following the G base is a deletion of "C" base.
      IV + 5 2 A C
      Ref: t t g c A - - g t t// 5 is the position of A base in the chromosome base.
         : t t g c A A C g t t// following the A base is a insertion of "AC" bases.

  d) *_invertion.lst
    For example:
      I	50191	100 
      I	948984	200
    Column 1: chromosome sequence ID.
    Column 2: invertion position (1-based) of raw chromosome sequence. Note that this position represent 
              for the prev-base of invertion region.
    Column 3: invertion length.

6 Note
  a) When simulate small InDel: insertion and deletion share 1/2 of the total rate respectively, 1~6bp bases 
  as the InDel number ,and the rate distribution of each type as below: 1bp-64.82%, 2bp-17.17%, 3bp-7.20%, 
  4bp-7.29%, 5bp-2.18%, 6bp-1.34%, which is the empirical distribution from panda re-sequencing data, we can 
  set the total small InDel rate with option -d. When simulate SV (structural variation): large insertion, 
  deletion and invertion, each of them share 1/3 of total rate, here we use this rate distribution for simplicity: 
  100bp-70%, 200bp-20%, 500bp-7%, 1000bp-2%, 2000bp-1%, we can set the total SV rate with option -v.
  b) Program do not simulate reads contain "N" char. If the input reference contains "N" char, reads which 
  generated in this region will be discarded.
  c) The allowed max length of read bases on the cycle number of Base-calling profile. User should set read length 
  less than half of cycle number.
  d) The default error rate of simulate data is in accordance with the Base-calling profile and the quality score 
  can well reflect the probability of sequencing error. However, this situation will become inefficient when user 
  set the average error rate different from that of profile.
  e) Reference sequences are parsed one chromosome at a time, and so be the output.Indiploid mode, each reference 
  file is sampled with half depth.
  f) Random-seed in this program is set with system time, so when you need to simulate data in batches, each task 
  has to run in different seconds.
  

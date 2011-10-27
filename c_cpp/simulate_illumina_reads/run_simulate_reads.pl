#! /usr/bin/perl
use strict;
use warnings;
use FindBin qw($Bin);
use Getopt::Long;

=head1 Description

	It is a program for simulating illumina PE reads,with a series of problems generate by 
	illumina sequencing machine in thought,such as insertsize distribution,error rate,heterozygosis
	SNP and heterozygosis Indel in diploid. User should set the value of insertsize_mean and 
	insertsize_sd ,they are the mean value and standard deviation of the normal distribution
	that used as the model function when simulating insertsize distribution,usually we set the 
	insertsize_sd to be 1/20 of the insertsize_mean.The normal distribution function model we
	used in this program is f(x)=1/��/sqrt(2*pi)*exp((x-��)**2 / (2*��**2)),To simulate illumina 
	error rates on different cycles,we use the function f(x)=0.00001*x**4 as a model function,
	because the error rate of a number of bases at the end is much larger than other area in 
	a read. User should set the heterozygosis SNP rate and heterozygosis Indel rate as also, 
	if you want to use those function,but remember that heterozygosis SNP rate and heterozygosis 
	Indel rate is only exists in diploid. At last ,you should set another several parameters, 
	whether simulate GC bias ,quality value , in addition, set the read length ,coverage of reads,
	input sequence, max number of running jobs ,running mode ,memory requirements and output prefix,
	the input sequence must be set ,because there is not default value.
	
=head1 History and Contributions

	This program originally developed by Lu jianliang and Yue zhen(2010-03-01 version 1.0). 
	Later the version 2.0 program was proposed by Fan Wei. Shi Yujian and Hu xuesong contributes to
	statistic analysis. According to the statistic analysis, the codes was revised by Yuan Jianying.

=head1 Contact and Version

	Contact:	yuanjianying@genomics.org.cn shiyujian@genomics.org.cn
	Version:	2.0		Data:	2011-4-25
	
=head1 Usage

	perl run_simulate_reads.pl [Option] <lib.lst>

=head1 Option

	-i	<string>	input reference genome sequence, *.fa/*.fa.gz, no default value
	-s	<float>	set the heterozygous SNP rate of the diploid genome,default:0
	-d	<float>	set the heterozygous indel rate of the diploid genome,default:0
	-e	<float>	set the average error rate over all cycles, should be set between 0.001 and 1, or set 0,default:0.01
	-g	<int>	set whether simulate GC bias, 0:no, 1:yes, default:0
	-q	<int>	whether simulate quality value, 0:no, 1:yes, default:0
	-j	<int>	set the max job, default: 1
	-o	<string>	output file prefix default:illumina
	-c	<int>	set output file type, 0:text, 1:*.gz, default:1.
	-r	<int>	set program running mode, 1:run directly(for testing node), 0:submit job (qsub), default:0
	-m	<int>	set the memory requirements while running mode is set 0(submit job), unit of memory:G, default:0.5
	-h		output help infomation
	
=head1 <lib.lst>

	In <lib.lst>, you must set parameter below:
	
	insertsize_mean	<int>	set the average value of insert size
	read_length	<int>	set read length, read1 and read2 have the same length
	coverage	<float>	set the sequencing coverage(sometimes called depth)
	insertsize_sd	<int>	set the standard deviation of insert sizes,usually set to be 1/20 of the insertsize_mean
	is_cyclization	<int>	set whether cyclize insert sequence,(influence on PE-reads direction),
				0: read1-forward read2-reverse, 1: read1-reverse read2-forward, default:0.
	
	when you creat the lib.lst file, you need to set parameter orderly as follow:
	
	insertsize_mean/read_length/coverage/insertsize_sd/is_cyclization
	
	for example:
	170		100		20	10	0
	500		100		20	20	0
	800		100		10	40	0
	2500		100		5	100	1

=head1 Example

	1. perl run_simulate_reads.pl -i ref_sequence.fa lib.lst
	every parameter use the default one. 
	2. perl run_simulate_reads.pl -i ref_sequence.fa -g 1 -q 1 lib.lst
	set simulate GC bias and quality value (*.fq)
	3. perl run_simulate_reads.pl -i ref_sequence.fa -e 0.02 -c 0 lib.lst
	set the average error rate over all cycles and output file is text type.
	4. perl run_simulate_reads.pl -i ref_sequence.fa -j 2 -o test lib.lst
	set the max number job and the output file prefix.
	5. perl run_simulate_reads.pl -i ref_sequence.fa -r 0 -m 0.8 lib.lst
	set the running mode and the memory size.
	6. perl run_simulate_reads.pl -i ref_sequence.fa -s 0.001 -d 0.0001 lib.lst
	the genome is diploid and you want to produce heterozygosis SNPs  heterozygosis Indels in reads.
	
=cut
################################################################################################

my ($inputref,$heterSNP_rate,$heterIndel_rate,$Is_simulate_GC_bias,$Is_simulate_quality,$error_rate,$max_job,$output_type,$output_prefix,$run_mode,$memory_size,$help);

GetOptions(
	"i:s"=>\$inputref,
	"s:f"=>\$heterSNP_rate,
	"d:f"=>\$heterIndel_rate,
	"e:f"=>\$error_rate,
	"g:i"=>\$Is_simulate_GC_bias,
	"q:i"=>\$Is_simulate_quality,
	"j:i"=>\$max_job,
	"c:i"=>\$output_type,
	"o:s"=>\$output_prefix,
	"r:i"=>\$run_mode,
	"m:f"=>\$memory_size,
	"h"=>\$help
);
die `pod2text $0` if ($help);
die `pod2text $0` if (!$inputref);
my $lib_lst = shift;

#############################Set default value#############################################
$heterSNP_rate=0 if (!defined $heterSNP_rate);
$heterIndel_rate=0 if (!defined $heterIndel_rate);
$Is_simulate_GC_bias=0 if	(!defined $Is_simulate_GC_bias);
$Is_simulate_quality=0 if	(!defined $Is_simulate_quality);
$error_rate=0.01 if	(!defined $error_rate);
$max_job=1 if (!defined $max_job);
$output_type=1 if (!defined $output_type);
$output_prefix="illumina" if (!defined $output_prefix);
$run_mode=0 if (!defined $run_mode);
$memory_size=0.5 if (!defined $memory_size);

die ("the -s(heterSNP_rate) should be set between 0 and 1\n") if ($heterSNP_rate<0 || $heterSNP_rate>=1);
die ("the -d(heterIndel_rate) should be set between 0 and 1\n") if ($heterIndel_rate<0 || $heterIndel_rate>=1);
die ("the -g(Is_simulate_GC_bias) should be set 0 or 1\n")if ($Is_simulate_GC_bias!=0 && $Is_simulate_GC_bias!=1);
die ("the -q(Is_simulate_quality) should be set 0 or 1\n")if ($Is_simulate_quality!=0 && $Is_simulate_quality!=1);
die ("the -e(error_rate) should be set between 0.001 and 1, or set 0\n") if ($error_rate>=1 || ($error_rate <= 0.001 && $error_rate !=0));
die	("the -j(max number job) should be set greater than 1\n")	if ($max_job<1);
die ("the -c(ouput_type) should be set 0 or 1\n") if ($output_type!=0 && $output_type!=1);
die ("the -r(run_mode) should be set 0 or 1\n") if ($run_mode!=0 && $run_mode!=1);

#############################Generate Run Script#############################################
if($heterSNP_rate != 0 || $heterIndel_rate != 0)
{
	open OUT1, ">simulate_snp_indel_seq.sh" or die "$!";
	print OUT1 "$Bin/simulate_snp_indel_seq -i $inputref -s $heterSNP_rate -d $heterIndel_rate -c $output_type -o ./$output_prefix && echo OK \n";
	print STDERR "Start simulate snp or indel of reference ... \n";
	if($run_mode){
		`sh simulate_snp_indel_seq.sh`;
	}else{
#  `qsub -l vf=0.3G -cwd simulate_snp_indel_seq.sh`;
	 my $memory = $memory_size."G";
 	 `perl $Bin/qsub-sge.pl -verbose -maxjob 1 -resource vf=$memory -reqsub simulate_snp_indel_seq.sh`;
	}
  print STDERR "Finished. \n";
  close OUT1;
}


open IN, $lib_lst or die "$!";
open OUT2,">simulate_illumina_reads.sh" or die "$!";
my $sleep_time = 0;
while(<IN>){
	chomp;
	my ($insertsize_mean,$read_length,$coverage,$insertsize_sd,$is_cyclization) = split(/\s+/);
	if(!defined $insertsize_mean){last;}
	die ("the mean insertsize should be set greater than 0\n") if ($insertsize_mean<=0);
	die ("reads length should be set greater than 0\n") if ($read_length<=0);
  die ("coverage should be set greater than 0\n") if ($coverage<=0);
	die ("the insertsize_sd should be set greater than 0\n") if ($insertsize_sd<=0);
	die ("the is_cyclization should be set 0 or 1\n") if ($is_cyclization!=0 && $is_cyclization!=1);
	
	my $ouput_dir = $output_prefix."_".$insertsize_mean."_".$read_length."_".$coverage."_".$insertsize_sd."_".$is_cyclization;
	`mkdir $ouput_dir`;
	
	my $ref_snp_indel_seq = $output_prefix;
	if($heterSNP_rate != 0)
	{
		$ref_snp_indel_seq = $ref_snp_indel_seq.".snp";
	}
	if($heterIndel_rate != 0)
	{
		$ref_snp_indel_seq = $ref_snp_indel_seq.".indel";
	}
	if(!$output_type)
	{
		$ref_snp_indel_seq = $ref_snp_indel_seq.".fa";
	}else{
		$ref_snp_indel_seq = $ref_snp_indel_seq.".fa.gz";
	}
	
	$sleep_time++;

	if($heterSNP_rate != 0 || $heterIndel_rate != 0)
	{
		if($run_mode == 1){
			print OUT2 "sleep $sleep_time && $Bin/simulate_illumina_reads -i $inputref -I $ref_snp_indel_seq -m $insertsize_mean -l $read_length -x $coverage -e $error_rate -v $insertsize_sd -g $Is_simulate_GC_bias -q $Is_simulate_quality -f $is_cyclization -c $output_type -o ./$ouput_dir/$output_prefix >./$ouput_dir/simulate_$insertsize_mean.o 2>./$ouput_dir/simulate_$insertsize_mean.e && echo OK \n";
		}else{
			print OUT2 "sleep $sleep_time && $Bin/simulate_illumina_reads -i $inputref -I $ref_snp_indel_seq -m $insertsize_mean -l $read_length -x $coverage -e $error_rate -v $insertsize_sd -g $Is_simulate_GC_bias -q $Is_simulate_quality -f $is_cyclization -c $output_type -o ./$ouput_dir/$output_prefix  && echo OK \n";
		}
	}else{
		if($run_mode == 1){
			print OUT2 "sleep $sleep_time && $Bin/simulate_illumina_reads -i $inputref -m $insertsize_mean -l $read_length -x $coverage -e $error_rate -v $insertsize_sd -g $Is_simulate_GC_bias -q $Is_simulate_quality -f $is_cyclization -c $output_type -o ./$ouput_dir/$output_prefix >./$ouput_dir/simulate_$insertsize_mean.o 2>./$ouput_dir/simulate_$insertsize_mean.e && echo OK \n";
		}else{
			print OUT2 "sleep $sleep_time && $Bin/simulate_illumina_reads -i $inputref -m $insertsize_mean -l $read_length -x $coverage -e $error_rate -v $insertsize_sd -g $Is_simulate_GC_bias -q $Is_simulate_quality -f $is_cyclization -c $output_type -o ./$ouput_dir/$output_prefix && echo OK \n";
		}
	}
}
close IN;
close OUT2;

#############################Contral Jobs Running#############################################
print STDERR "Start simulate reads ... \n";
if($run_mode){
	open IN2, "simulate_illumina_reads.sh" or die "$!";
  open OUT3, ">job_contral.stat" or die "$!";
	close OUT3;
	my $max_job_tem = $max_job;
	my $job_num = 0;
	while(<IN2>){
		chomp;
		$job_num++;
#		print STDERR "$job_num  $max_job_tem  \n";
#		print STDERR "Run: $max_job $job_num ... \n";
		if($job_num < $max_job_tem){
			my $run_script = $_." & ";
#			print STDERR "Run: $run_script... \n";
			system("nohup $_ && echo $job_num >>job_contral.stat &");
		}else{
			if($job_num == $max_job_tem){system("nohup $_ && echo $job_num >>job_contral.stat &"); }
			##contral process##
			while(1){
				sleep 10;
				open IN3, "job_contral.stat" or die "$!";
				my $line_num = 0;
				while(<IN3>){
					$line_num++;
				}
				close IN3;
				if($job_num - $line_num >= 0 && $job_num - $line_num < $max_job){ $max_job_tem = $max_job_tem + ($max_job - ($job_num - $line_num));last;}
			}
		}
	}
	##wait for all the jobs finished##
  while(1){
  	sleep 10;
  	open IN3, "job_contral.stat" or die "$!";
  	my $line_num2 = 0;
  	while(<IN3>){
  		$line_num2++;
  	}
  	close IN3;
#  	print STDERR "$job_num  $line_num2  \n";
  	if($job_num == $line_num2){last;}
  }
  
	`rm job_contral.stat`;

#	print STDERR "BEGIN!\n";
#	`sh simulate_reads.sh &`;
}else{
	my $memory = $memory_size."G";
	`perl $Bin/qsub-sge.pl -verbose -maxjob $max_job -resource vf=$memory -reqsub simulate_illumina_reads.sh`;
}

close IN2;
print STDERR "All finished! \n";
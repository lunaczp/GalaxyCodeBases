#!/bin/env perl
#use lib "/ifs1/ST_ASMB/USER/huxuesong/public/lib";
use strict;
use warnings;
use Time::HiRes qw ( gettimeofday tv_interval );
use Galaxy::ShowHelp;

$main::VERSION=0.0.1;
our $opts='o:b';
our($opt_o, $opt_b);

#our $desc='';
our $help=<<EOH;
\t-o output prefix (./allmatrix).{mcount,mratio}
\t-b No pause for batch runs
EOH
our $ARG_DESC='matrix_count_files';

ShowHelp();
$opt_o='./allmatrix' if ! $opt_o;

print STDERR "From [@ARGV] to [$opt_o]\n";
unless ($opt_b) {print STDERR 'press [Enter] to continue...'; <STDIN>;}

my $start_time = [gettimeofday];
#BEGIN
my $READLEN=0;
my $Qcount=41;
my ($TotalReads,$TotalBase,$MisBase,%BaseCountTypeRef)=(0,0,0);
my ($mapBase,$mapReads)=(0,0);
my $type='N/A';
my %Stat;   # $Stat{Ref}{Cycle}{Read-Quality}
my @BQHeader;
while (<>) {
    if (/^#Input \[(\w+)\] file of mapped Reads: (\d+) , mapped Bases (\d+) \(no base stat for sam files\)$/) {
        $type=$1 if $type ne 'sam';
        $mapReads += $2;
        $mapBase += $3 if $type ne 'sam';
    }
    if (/^#Total statistical Bases: (\d+) , Reads: (\d+) of ReadLength (\d+)$/) {
        print " >$ARGV|   Reads:$2   ReadLen:$3   Bases:$1\n";
        $TotalReads += $2;
        $READLEN = $3 if $READLEN < $3;
    }
    if (/^#Ref\tCycle\t/) {
        #s/^#//;
        chomp;
        (undef,undef,@BQHeader)=split /\t/;
        pop @BQHeader if $BQHeader[-1] eq 'RowSum';
    }
    if (/^#Dimensions:.+?Quality_number (\d+)$/) {
        $Qcount = $1 if $Qcount<$1;
    }
    if (/^#Mismatch_base: (\d+)/) {
        $MisBase += $1;
    }
    next if /^#/;
    next if /^$/;
    chomp;
    my ($ref,$cycle,@BQ)=split /\t/;
    #print "$ref,$cycle,@BQ\n";
    next unless $ref =~ /^[ATCG]$/;
    #die "[$_]\n$ref,$cycle,[@BQ]\n$#BQ < $#BQHeader " if $#BQ < $#BQHeader;
    for my $key (@BQHeader) {
        my $value=shift @BQ;
        $Stat{$ref}{$cycle}{$key}+=$value;
        $BaseCountTypeRef{$ref}+=$value;
        $TotalBase+=$value;
        #print "{$ref}{$cycle}{$key}$value\n";
    }
}
#print $TotalReads,"\t",$READLEN,"\n";
#print join("\t",@BQHeader),"\n";
open OA,'>',$opt_o.'.mcount' or die "Error: $!\n";
open OB,'>',$opt_o.'.mratio' or die "Error: $!\n";
my $tmp;
chomp(my $user=`id -nru`);
@ARGV=('/etc/passwd');
chomp(my @passwd=<>);
($_)=grep /$user/,@passwd;
$tmp=(split /:/)[4];
my $mail='';
$mail=" <$tmp>" unless $tmp =~ /^\s*$/;
my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)=localtime(time);
my $date=sprintf "%02d:%02d:%02d,%4d-%02d-%02d",$hour,$min,$sec,$year+1900,$mon+1,$mday;
my $Cycle=2*$READLEN;
my $MisRate=100*$MisBase/$TotalBase;
$tmp="#Generate @ $date by ${user}$mail
#Input [$type] file of Reads: $mapReads , Bases $mapBase (no base stat for sam files)
#Total statistical Bases: $TotalBase , Reads: $TotalReads of ReadLength $READLEN
#Dimensions: Ref_base_number 4, Cycle_number $Cycle, Seq_base_number 4, Quality_number $Qcount
#Mismatch_base: $MisBase, Mismatch_rate: $MisRate %
#Reference Base Ratio in reads: ";
my @BaseOrder=sort keys %BaseCountTypeRef;  # qw{A T C G};
for (@BaseOrder) {
    $tmp .= $_.' '. int(0.5+100*1000*$BaseCountTypeRef{$_}/$TotalBase)/1000 .' %;   ';
}

$tmp .= "\n#".join("\t",'Ref','Cycle',@BQHeader);
print OA $tmp;
print OB $tmp;
print OA "\tRowSum";
print OB "\n";
my ($count,$countsum);
for my $ref (@BaseOrder) {
    print OA "\n";
    for my $cycle (sort {$a<=>$b} keys %{$Stat{$ref}}) {
        $tmp="$ref\t$cycle\t";
        print OA $tmp; print OB $tmp;
        my (@Counts,@Rates)=();
        for my $bq (@BQHeader) {
            push @Counts,$Stat{$ref}{$cycle}{$bq};
        }
        #print "[",join("|",@Counts),"\n";
        $countsum=0;
        $countsum += $_ for @Counts;
        push @Rates,$_/$countsum for @Counts;
        print OA join("\t",@Counts,$countsum),"\n";
        print OB join("\t",@Rates),"\n";
    }
}
close OA;
close OB;

#END
my $stop_time = [gettimeofday];

print STDERR "\nTime Elapsed:\t",tv_interval( $start_time, $stop_time )," second(s).\n";
__END__

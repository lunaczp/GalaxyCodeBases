#!/bin/env perl
#use lib "/ifs1/ST_ASMB/USER/huxuesong/public/lib";
use strict;
use warnings;
use Time::HiRes qw ( gettimeofday tv_interval );
use Galaxy::ShowHelp;

$main::VERSION=0.0.1;
our $opts='r:o:b';
our($opt_o, $opt_r, $opt_b);

#our $desc='';
our $help=<<EOH;
\t-r ref fasta file (./ref/human.fa)
\t-o output prefix (./matrix).{raw,ratio}
\t-b No pause for batch runs
EOH
our $ARG_DESC='sampe_files';

ShowHelp();
$opt_r='./ref/human.fa' if ! $opt_r;
$opt_o='./matrix' if ! $opt_o;
die "[x]-r $opt_r not exists !\n" unless -f $opt_r;

print STDERR "From [@ARGV] with [$opt_r] to [$opt_o]\n";
unless ($opt_b) {print STDERR 'press [Enter] to continue...'; <STDIN>;}

my $start_time = [gettimeofday];
#BEGIN

warn "[!]Reading Reference Genome:\n";
my %Genome;
open GENOME,'<',$opt_r or die "Error: $!\n";
while (<GENOME>) {
    s/^>//;
	/^(\S+)/ or next;
	my $seqname = $1;
    print STDERR " >$seqname ...";
	$/=">";
	my $genome=<GENOME>;
	chomp $genome;
	$genome=~s/\s//g;
	$/="\n";
    $Genome{$seqname}=$genome;
    print STDERR "\b\b\b",length $Genome{$seqname},".\n";
	$genome='';
}
close GENOME;
sub getBases($$$) {
    my ($chr,$start,$len)=@_;
    return substr $Genome{$chr},$start-1,$len;
}

my $READLEN=100;
my $MaxQ=40;

my ($TotalBase,$TotalReads,%BaseCountTypeRef);
my %Stat;   # $Stat{Ref}{Cycle}{Read}{Quality}
sub statRead($$$$$) {
    my ($ref,$isReverse,$read,$Qstr,$cyclestart)=@_;
    my $PEpos=-1;
    for (my $i=0;$i<$READLEN;$i++) {
        my $refBase=substr $ref,$i,1;
        next if $refBase eq 'N';
        my $readBase=substr $read,$i,1;
        next if $readBase eq 'N';
        my $QstrSingle=substr $Qstr,$i,1;
        my $Qval=ord($QstrSingle)-33;
        $MaxQ=$Qval if $MaxQ<$Qval;
        if ($isReverse) {
            $PEpos=$cyclestart+$READLEN-1-$i;
        } else {
            $PEpos=$cyclestart+$i;
        }
        ++$Stat{$refBase}{$PEpos}{$readBase}{$Qval};
        ++$BaseCountTypeRef{$refBase};
        ++$TotalBase;
#print "$isReverse {$refBase}{$PEpos}{$readBase}{$Qval} ",($refBase eq $readBase)?'=':'x',"\n";
    }
    ++$TotalReads unless $PEpos==-1;
}

while (<>) {
    next if /^@\w\w\t\w\w:/;
    chomp;
    my @read1=split /\t/;
    chomp($_=<>) or last;
    my @read2=split /\t/;
#print join("\t",@read1),"\n-",join("\t",@read2),"\n";
    die "[x]Not PE sam file.\n" if $read1[0] ne $read2[0];
    next unless $read1[1] & 3;
    next if $read1[1] >= 256;
    next unless $read2[1] & 3;
    next if $read2[1] >= 256;
    next unless $read1[5] =~ /^(\d+)M$/;
    next unless $1 == $READLEN;
    next unless $read2[5] =~ /^(\d+)M$/;
    next unless $1 == $READLEN;
    next unless $read1[6] eq '=';
    next unless $read2[6] eq '=';
    next if $read1[11] eq 'XT:A:R';
    next if $read2[11] eq 'XT:A:R';
    my $ref1=uc getBases($read1[2],$read1[3],$READLEN) or print join("\t",@read1),"\n";
    my $ref2=uc getBases($read2[2],$read2[3],$READLEN) or print join("\t",@read2),"\n";
    #my ($QNAME,$FLAG,$RNAME,$POS,$MAPQ,$CIAGR,$MRNM,$MPOS,$ISIZE,$SEQ,$QUAL,$OPT)=@read1;
    #       0      1    2       3   4       5   6       7     8     9    10    11
    statRead($ref1,$read1[1] & 16,$read1[9],$read1[10],1);
    statRead($ref2,$read2[1] & 16,$read2[9],$read2[10],101);
}

open OA,'>',$opt_o.'.mcount' or die "Error: $!\n";
open OB,'>',$opt_o.'.mratio' or die "Error: $!\n";
my $tmp;
$tmp="#Total statistical Bases: $TotalBase , Reads: $TotalReads of ReadLength $READLEN
#Reference Base Ratio in reads: ";
my @BaseOrder=sort keys %BaseCountTypeRef;  # qw{A T C G};
for (@BaseOrder) {
    $tmp .= $_.' '. int(0.5+100*1000*$BaseCountTypeRef{$_}/$TotalBase)/1000 .' %;   ';
}
my @BaseQ;
for my $base (@BaseOrder) {
    push @BaseQ,"$base-$_" for (1..$MaxQ);
}
$tmp .= "\n#".join("\t",'Ref','Cycle',@BaseQ)."\n";
print OA $tmp;
print OB $tmp;
my ($count,$countsum);
for my $ref (@BaseOrder) {
    for my $cycle (1..(2*$READLEN)) {
        $tmp="$ref\t$cycle\t";
        print OA $tmp; print OB $tmp;
        my (@Counts,@Rates)=();
        for my $base (@BaseOrder) {
            for my $q (1..$MaxQ) {
                if (exists $Stat{$ref}{$cycle} and exists $Stat{$ref}{$cycle}{$base} and exists $Stat{$ref}{$cycle}{$base}{$q}) {
                    $count=$Stat{$ref}{$cycle}{$base}{$q};
                } else {$count=0;}
                push @Counts,$count;
            }
        }
        $countsum=0;
        $countsum += $_ for @Counts;
        push @Rates,$_/$countsum for @Counts;
        print OA join("\t",@Counts),"\n";
        print OB join("\t",@Rates),"\n";
    }
}
close OA;
close OB;
#END
my $stop_time = [gettimeofday];

print STDERR "\nTime Elapsed:\t",tv_interval( $start_time, $stop_time )," second(s).\n";
__END__


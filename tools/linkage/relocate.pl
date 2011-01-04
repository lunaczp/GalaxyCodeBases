#!/bin/env perl
use strict;
use warnings;
use lib '/nas/RD_09C/resequencing/soft/lib';
use Galaxy::ShowHelp;
use Time::HiRes qw ( gettimeofday tv_interval );

$main::VERSION=0.0.1;

our $opts='i:o:n:g:v:r:c:l:h:bqd';
our($opt_i, $opt_n, $opt_g, $opt_r, $opt_c, $opt_l, $opt_o, $opt_v, $opt_b, $opt_q, $opt_d, $opt_h);

our $help=<<EOH;
\t-i Blast filted file
\t-c Chromosome NFO file (chr.nfo) in format: /^ChrID\\s+ChrLen\\s?.*\$/
\t-l linkage map list (./linkagemap.lst)
\t-n N zone file (Pa64.Nzone)
\t-o Output file
\t-v Verbose level (undef=0)
\t-b No pause for batch runs
\t-d Debug Mode, for test only
EOH

ShowHelp();

die "[x]-i $opt_i not exists !\n" unless -f $opt_i;
$opt_c='chr.nfo' if ! $opt_c;
$opt_l='./linkagemap.lst' if ! $opt_l;
$opt_n='Pa64.Nzone' if ! $opt_n;

print STDERR "From [$opt_i] to [$opt_o] refer to [$opt_n][$opt_l][$opt_c]\n";
print STDERR "DEBUG Mode on !\n" if $opt_d;
print STDERR "Verbose Mode [$opt_v] !\n" if $opt_v;
unless ($opt_b) {print STDERR 'press [Enter] to continue...'; <>;}

my $start_time = [gettimeofday];
#BEGIN
my %Scaffords;	# \%( $chr->[$sPosRel,$scM,$sWeight] )
my %LinkageMap;

open L,'<',$opt_l or die "Error: $!\n";
while (<L>) {
	chomp;
	my ($ChrID,$File)=split /\t/;
	open LM,'<',$File or die "Error: $!\n";
	while (<LM>) {
		next if /^#/;
		my ($Chr,$Pos,$cM)=split /\t/;
		die "[x][$ChrID] not match [$File] !\n" if $Chr ne $ChrID;
		$LinkageMap{$Chr}{$Pos}=$cM;
	}
}
close L;

sub getRel($$$$$) {
	my ($Qid,$Qs,$Qe,$Ss,$Se,$BTOP)=@_;
	my ($mChr,$mPos,$mSiderLen)=@{&splitMarkerid($Qid)};
	my $cM=$LinkageMap{$mChr}{$mPos};
	my $LeftBPalongQ=$mSiderLen-$Qs+1;
	my $WalkingOn=$LeftBPalongQ;
	my @btop=split /(\D+)/,$BTOP;
# $ perl -le '$a="45YT9-Ac-c-c-11TC4";@b=split /(\D+)/,$a;print "[",join("|",@b),"]"'
# [45|YT|9|-Ac-c-c-|11|TC|4]
	my $LeftBPalongS=0;
	for (@btop) {
		if ($WalkingOn <= 0) {
			print STDERR "$Qid [$_] " if $opt_v and $WalkingOn == 0;
			last;
		}
		print STDERR "-$Qid [$_]-" if $opt_v>1;
		if (/\d/) {
			$LeftBPalongS += $_;
			$WalkingOn -= $_;
		} else {
			my @bin=split /([\w-]{2})/;
# $ perl -le '$a="-Ac-c-c-";@b=split /([\w-]{2})/,$a,0;print "[",join("|",@b),"]"'
# [|-A||c-||c-||c-]
			for (@bin) {
				next unless $_;
				my @op=split //;
				if (/-/) {
					--$WalkingOn if $op[1] eq '-' and $op[0] ne '-';
					++$LeftBPalongS if $op[0] eq '-' and $op[1] ne '-';
				} else {
					--$WalkingOn;
					++$LeftBPalongS;
				}
			}
		}
		print STDERR "-$WalkingOn $LeftBPalongS\n" if $opt_v>1;
		my $NOP;
	}
	my $strand;
	if ($Ss < $Se) {
		$strand=1;
	} else {
		$strand=-1;
	}
	my $Spos=$Ss + $strand*$LeftBPalongS;
	warn "$mChr,$Spos,$cM\n";
	return [$mChr,$Spos,$cM];
}
sub splitMarkerid($) {
	my $MarkerID=$_[0];
	my ($mChr,$mPos,$mSiderLen)=split /[_m]/,$MarkerID,3;
	return [$mChr,$mPos,$mSiderLen];
}
sub pushScaf($) {
	my ($Qid,$Sid,$Pidentity,$AlnLen,$identical,$mismatches,$Gap,$Qs,$Qe,$Ss,$Se,$E,$bitScore,$BTOP,$Hit)=@{$_[0]};
	my ($sPosRel,$scM,$sWeight)=(0,0,0);
	my ($mChr,$mPos,$mSiderLen)=@{&splitMarkerid($Qid)};
	my $weight=-log($E)/$Hit;
	my %Dat;
	if (exists $Scaffords{$Sid}) {
		%Dat=%{$Scaffords{$Sid}};
		if (exists $Scaffords{$Sid}{$mChr}) {
			($sPosRel,$scM,$sWeight)=@{$Dat{$mChr}};
		}
	} else {
		%Dat=($mChr=>[$sPosRel,$scM,$sWeight]);
	}
	my ($chr,$pos,$cM)=@{&getRel($Qid,$Qs,$Qe,$Ss,$Se,$BTOP)};
	$sPosRel += $pos*$weight;
	$scM += $cM*$weight;
	$sWeight += $weight;
	$Dat{$mChr}=[$sPosRel,$scM,$sWeight];
	$Scaffords{$Sid}=\%Dat;
}

open IN,'<',$opt_i or die "Error: $!\n";
while (<IN>) {
	next if /^#/;
# Fields: query id, subject id, % identity, alignment length, identical, mismatches, gap opens, q. start, q. end, s. start, s. end, evalue, bit score, BTOP
#-outfmt '6 qseqid sseqid pident length nident mismatch gapopen qstart qend sstart send evalue bitscore btop'
#Chr01_457584m45 Scaffold011460  95.45   88      87      4       0       1       88      1020    933     1e-34    147    16CA2WA25MA6WA35        1
	chomp;
	my @Dat=split /\t/;
	my ($Qid,$Sid,$Pidentity,$AlnLen,$identical,$mismatches,$Gap,$Qs,$Qe,$Ss,$Se,$E,$bitScore,$BTOP,$Hit)=@Dat;
	next if $Sid =~ /^chr/i;	# Just skip ...
	&pushScaf(\@Dat);
}



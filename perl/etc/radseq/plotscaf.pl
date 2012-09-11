#!/usr/bin/env perl
=pod
Author: Hu Xuesong @ BIOPIC <galaxy001@gmail.com>
Version: 1.0.0 @ 20120720
=cut
use strict;
use warnings;
use Data::Dump qw(ddx);
use Galaxy;

die "Usage: $0 <prefix> [out midfix]\n" if @ARGV<1;
my $inf=shift;
my $outf=shift;

if ($outf) {
	$outf = ".$outf";
} else {
	$outf = '';
}

my $scaffnfo = '/bak/seqdata/2012/tiger/120512_TigerRefGenome/chr.nfo';
my $markerdat = '/share/users/huxs/work/tiger/paper/rec.npa';
print "From [$inf] to [$inf$outf.(dat|svg)]\n";

my %Stat;

sub getVal($) {
	my %dat = %{$_[0]};
	my ($sum,$cnt,$max,$major)=(0,0,0,0);
	my $mjcnt=0;
	for my $k (keys %dat) {
		$sum += $k * $dat{$k};
		$cnt += $dat{$k};
		$max = $k if $max < $k;
		if ($k and $mjcnt<$dat{$k}) {
			$major = $k;
			$mjcnt = $dat{$k};
		}
	}
	if ($cnt) {
		return ($major,$max);
		return $sum/$cnt;
	} else {
		return -1;
	}
}

my %ScaffoldLen;
open I,'<',$scaffnfo or die $!;
while (<I>) {
	next if /^#/;
	chomp;
	my @items = split /\t/;
	$ScaffoldLen{$items[0]} = $items[1];
}
close I;
print "\n",scalar keys(%ScaffoldLen)," scaffold(s) Load.\n";

my %MarkerDat;
open I,'<',$markerdat or die $!;
while (<I>) {
	next if /^#/;
	chomp;
	my @items = split /\t/;
	die unless exists $ScaffoldLen{$items[1]};
	push @{$MarkerDat{$items[1]}},[ $items[2],$items[9],int(0.5-1000*log($items[9])/log(10))/1000 ];	# pos,p,lg(p) round to 0.001
}
close I;
#ddx \%MarkerDat;
my (%OrderbyChr,%OrderedOnce,@DirectOrder,@notOrdered);
open I,'<',$inf.'.order' or die $!;
while (<I>) {
	next if /^#/;
	chomp;
	my ($scaff,$chr) = split /\t/ or next;	# skip blank lines
	next if exists $OrderedOnce{$scaff};
	push @{$OrderbyChr{$chr}},$scaff;
	push @DirectOrder,$scaff;
	++$OrderedOnce{$scaff};
}
close I;
my $TotalLen=0;
my @order = map {$_='scaffold'.$_} sort {$a<=>$b} map {s/^scaffold//;$_;} keys %MarkerDat;
for my $scaff (@order) {
	next unless exists $MarkerDat{$scaff};
	if (exists $OrderedOnce{$scaff}) {
		$Stat{Len_Ordered} += $ScaffoldLen{$scaff} or die;
		++$Stat{Scaffold_Ordered};
	} else {
		push @notOrdered,$scaff;
		$Stat{Len_notOrdered} += $ScaffoldLen{$scaff} or die;
		++$Stat{Scaffold_notOrdered};
	}
	$TotalLen += $ScaffoldLen{$scaff};
}
if ($Stat{Scaffold_notOrdered}) {
	$Stat{AvgLen_notOrdered} = $Stat{Len_notOrdered} / $Stat{Scaffold_notOrdered};
}
if ($Stat{Scaffold_Ordered}) {
	$Stat{AvgLen_Ordered} = $Stat{Len_Ordered} / $Stat{Scaffold_Ordered};
}

# ------ BEGIN PLOT --------
my @color = qw(Red Brown Navy Green Maroon Blue Purple Orange Lime Teal);
my $Xrange = 1000;
my $Yrange = 500;
my $YmaxVal = 5;
my $ArrowLen = 20;	# 16+4
my $axisTick = 4;
my $OutBorder = 24;
my $InBorder = 40;
my $Xtotal = $Xrange + $ArrowLen + 2*$OutBorder;
my $Yitem = $Yrange + $ArrowLen + $InBorder;

my $perUnit = int($TotalLen/10);	# 279.330936 M /10 = 27.933093 M
my $numlevel = int(log($perUnit)/log(10));	# 7
my $numSuflevel = int($numlevel/3);	# 2
my $numSuf=( '', qw( K M G T P E Z Y ) )[$numSuflevel];	# M <---
$numSuflevel = 10 ** (3*$numSuflevel);	# 1,000,000 <---
my $roundTo = 5 * (10 ** ($numlevel-1));	# 5e6
my $unit = $perUnit + (-$perUnit % $roundTo);	# 30 M
my $countMax = int($TotalLen/$unit) + (($TotalLen%$unit)?1:0);
#print join(",",$TotalLen/$numSuflevel,$perUnit/$numSuflevel,$numlevel,$numSuf,$numSuflevel,$roundTo/$numSuflevel,$unit/$numSuflevel,$countMax),"\n";
my $BasepPx = 10*$unit/$Xrange;

my @Yticks;
for my $i (1 .. $YmaxVal) {	# 0 will be shared with X-axie
	my $pY = $Yrange - $i * ($Yrange/$YmaxVal);
	push @Yticks,$pY;
}

my (%PlotDat,%PlotScaffRange);
my $start = 0;
for my $scaff (@notOrdered,@DirectOrder) {	# Well, Ordered last to be far away from X-axie.
	unless (exists $MarkerDat{$scaff}) {
		warn "[!marker]$scaff\n";
		++$Stat{'Marker_Not_found'};
		next;
	}
	my $maxlgp = 0;
	for my $mditem (@{$MarkerDat{$scaff}}) {
		my ($pos,$p,$lgp) = @$mditem;
		my $posOchr = $start + $pos;
		if ($posOchr < 0) {
			$posOchr = 0;
			++$Stat{'Marker_Pos_Minus'};
		} elsif ($pos > $ScaffoldLen{$scaff}) {
			$posOchr = $start + $ScaffoldLen{$scaff};
			++$Stat{'Marker_Pos_Overflow'};
		}
		++$PlotDat{$scaff}{int(0.5+10*$posOchr/$BasepPx)/10}{$lgp};
		$maxlgp = $lgp if $maxlgp < $lgp;
	}
	$PlotScaffRange{$scaff} = [ ( map {int(0.5+10*$_/$BasepPx)/10} ($start+1,$start+$ScaffoldLen{$scaff}) ),$maxlgp ];
	$start += $ScaffoldLen{$scaff};
}

my $ChrCount = keys %PlotDat;
$ChrCount = 1;
my $Ytotal = $Yitem*$ChrCount - $InBorder + $ArrowLen + 2*$OutBorder;

open O,'>',$inf.$outf.'.svg' or die $!;
print O <<HEAD;
<?xml version="1.0"?>
<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" version="1.2" baseProfile="tiny"
 width="${Xtotal}px" height="${Ytotal}px">
<title>Plot $inf</title>
<!--
  <rect x="0" y="0" width="$Xtotal" height="$Ytotal" fill="none" stroke="red" stroke-width="2" />
-->
HEAD

print O <<'DEF1';
  <defs>
    <g id="axis" stroke="black" font-size="16" font-family="Arial" stroke-width="0" text-anchor="middle">
      <polyline fill="none" points="-2,-4 0,-20 2,-4 0,-20 
DEF1
for (@Yticks) {
	print O "        0,$_ -$axisTick,$_ 0,$_ \n"
}
print O "        0,$Yrange\n";

for my $i (1 .. 10) {
	my $x = $i*$Xrange/10;
	print O '        ',$x,',',$Yrange,' ',$x,',',$Yrange+$axisTick,' ',$x,',',$Yrange,"\n";
}
print O '        ',$Xrange+$ArrowLen,',',$Yrange,' ',
	$Xrange+$axisTick,',',$Yrange-2,' ',$Xrange+$axisTick,',',$Yrange+2,' ',
	$Xrange+$ArrowLen,',',$Yrange,'" stroke-width="2"/>',"\n";
for my $i (1 .. $YmaxVal) {
	my $y = $Yticks[$i-1];
	print O <<TXTAX;
      <text x="0" y="$y" dx="-20" dy="5" text-anchor="middle">$i</text>
      <line x1="0" y1="$y" x2="$Xrange" y2="$y" stroke-width="1" stroke-dasharray="1,9"/>
TXTAX
}
for my $i (0 .. 10) {
	my $x = $i*$Xrange/10;
	my $l = $unit*$i/$numSuflevel;
	$l .= " $numSuf" if $l;
	print O <<TXT1;
      <text x="$x" y="$Yrange" dy="20" text-anchor="middle">$l</text>
TXT1
}
print O <<DEF2;
    </g>
  </defs>
  <g transform="translate($OutBorder,$OutBorder)" stroke-width="2" stroke="black" font-size="16" font-family="Arial">
DEF2

my $thisChrNo=0;
for my $chr ('Tiger') {
	my $topY = $ArrowLen + $Yitem*$thisChrNo;
	print O <<TXT2;
    <g transform="translate(0,$topY)" stroke-width="1">
      <use x="0" y="0" xlink:href="#axis" stroke-width="2"/>
      <text x="5" y="-6" stroke-width="0">$chr</text>
TXT2
	my $scaffcnt=0;
	for my $scaff (@DirectOrder,@notOrdered) {
		next unless exists $MarkerDat{$scaff};
		my @Poses = sort {$a<=>$b} keys %{$PlotDat{$scaff}};
		my $thiscolor = $color[$scaffcnt%scalar(@color)];
		my ($pa,$pb,$maxlgp) = @{$PlotScaffRange{$scaff}};
		print O '      <g stroke="',$thiscolor,'" focusable = "true">',"\n        <title>$scaff, max=$maxlgp</title>\n";
		for my $pos (@Poses) {
			my ($major,$max) = getVal($PlotDat{$scaff}{$pos});
			my ($pYmajor,$pYmax) = map {int(10*$Yrange*(1-$_/$YmaxVal))/10;} ($major,$max);
			print O <<TXTL;
        <line x1="$pos" y1="$pYmajor" x2="$pos" y2="$pYmax"/>
TXTL
		}
		#my ($pa,$pb) = @{$PlotScaffRange{$scaff}};
		print O <<TXTLB;
        <line x1="$pa" y1="$Yrange" x2="$pb" y2="$Yrange" stroke-width="2"/>
      </g>
TXTLB
		++$scaffcnt;
	}
	print O "    </g>\n";
#	++$thisChrNo;
}

print O "  </g>\n</svg>\n";
=pod
print O "
  </g>
  <rect x=\"$OutBorder\" y=\"$OutBorder\" width=\"",$Xrange+$ArrowLen,"\" height=\"",$Ytotal-2*$OutBorder,"\" fill=\"none\" stroke=\"blue\" stroke-width=\"1\" />
  <line x1=\"",$Xrange+$OutBorder,"\" y1=\"$OutBorder\" x2=\"",$Xrange+$OutBorder,"\" y2=\"",$Ytotal-$OutBorder,"\" stroke=\"blue\" stroke-width=\"1\"/>
</svg>
";
=cut
close O;

ddx \%Stat;
print commify($TotalLen),"\n";
__END__

perl -F',' -lane "map {s/'//g;s/ /_/} @F;print join(\"\t\",@F);" ./tig2cat.csv > ./tig2cat.tsv
sort -t $'\t' -k 1,1 -k 2,2n -k 3,3n ./tig2cat.tsv > ./tig2cat.tsv.s
awk '{print "scaffold"$4"\tchr"$1}' tig2cat.tsv.s|uniq > tig2cat.order
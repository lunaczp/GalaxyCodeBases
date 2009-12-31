#!/usr/bin/perl -w
use threads;
use strict;
use warnings;
use DBI;
use Time::HiRes qw ( gettimeofday tv_interval );
use Galaxy::ShowHelp;
#use File::Basename;

$main::VERSION=0.2.1;

our $opts='i:o:s:bva';
our($opt_i, $opt_o, $opt_s, $opt_v, $opt_b, $opt_a);

our $help=<<EOH;
\t-i Input GFF3 annotation file (human.gff) [ *.gz/*.bz2 is OK ]
\t  [Chromosome name will be striped.]
\t-s Specie of the GFF3 (human)
\t  [Specie name MUST be ths SAME throughout !]
\t-o Output SQLite data file (_dbGFF.sqlite)
\t-a Additive mode to share one SQLite data file among multi-Species
\t   [Output file will be OVERWRITE without -a !]
\t-v show verbose info to STDOUT
\t-b No pause for batch runs
EOH

ShowHelp();

$opt_i='human.gff' if ! defined $opt_i;
$opt_o='_dbGFF.sqlite' if ! $opt_o;
$opt_s='human' if ! $opt_s;

print STDERR "From [$opt_i] to [$opt_o], Specie:[$opt_s]\n";
print STDERR "[!]Additive mode SET.\n" if $opt_a;
if (! $opt_b) {print STDERR 'press [Enter] to continue...'; <>;}

my $start_time = [gettimeofday];
#BEGIN

### /dev/shm
my $shm_real='/dev/shm/sqlite_mirror.'.$$;
unlink $shm_real;	# Well, what if the computer rebooted and you are so lucky ...
system 'cp','-pf',$opt_i,$shm_real if $opt_a;
###

my %attr = (
    RaiseError => 0,
    PrintError => 1,
    AutoCommit => 0
);
my $dbh = DBI->connect('dbi:SQLite:dbname='.$shm_real,'','',\%attr) or die $DBI::errstr;

my $sql=q/
CREATE TABLE IF NOT EXISTS gff{---}
(  chrid TEXT,
   primary_inf TEXT,
   start INTEGER,
   end INTEGER,
   strand TEXT,
   frame TEXT,
   name TEXT,
   groups TEXT );
/;
for (split /;/,$sql) {
	next if /^\s*$/;
	s/{---}/$opt_s/g;
#print "[$_]\n";
	$dbh->do($_) or die $dbh->errstr;
}
$dbh->commit;

my $sth = $dbh->prepare( "INSERT INTO gff$opt_s ( chrid,primary_inf,start,end,strand,frame,groups,name ) VALUES ( ?,?,?,?,?,?,?,? )" );

my $infile;
if ($opt_i =~ /.bz2$/) {
	open( $infile,"-|","bzip2 -dc $opt_i") or die "Error: $!\n";
} elsif ($opt_i =~ /.gz$/) {
 	open( $infile,"-|","gzip -dc $opt_i") or die "Error: $!\n";
} else {open( $infile,"<",$opt_i) or die "Error: $!\n";}

sub unescape {
  my $v = shift;
  $v =~ tr/+/ /;
  $v =~ s/%([0-9a-fA-F]{2})/chr hex($1)/ge;
  return $v;
}

sub read_gene_gff($$) {
    my $file=$_[0];
    my $dbh=$_[1];
    my @dat;
    while (<$infile>) {
	next if /^\s+$/ or /^;+/;
	chomp;
	s/\r//g;	# There DO be some file with \r. DAMN the MS and APPLE !
	my ($seqname, $source, $primary, $start, $end, 
	$score, $strand, $frame, $groups) = split /\t/;	# well, reading file no need to Optimize
	#$seqname=substr($seqname,length($seqname)-2);
#	$seqname =~ s/^chr(?>o?m?o?s?o?m?e?)//i;	#chromosome
	$seqname =~ s/^chr
			(?>
				((?<=^chr)o)?
				((?<=^chro)m)?
				((?<=^chrom)o)?
				((?<=^chromo)s)?
				((?<=^chromos)o)?
				((?<=^chromoso)m)?
				((?<=^chromosom)e)?
			)//xi;
	my @groups = split(/\s*;\s*/, $groups);
	my (%groups,$name);
    for my $group (@groups) {
	my ($tag,$value) = split /=/,$group;
	$tag             = unescape($tag);
#	my @values       = map {unescape($_)} split /,/,$value;
	$groups{$tag}=$value;
	#for my $v ( @values ) { $feat->add_tag_value($tag,$v); }
    }
	my @name_order=qw/Parent ID/;
	@name_order=qw/ID Parent/ if $primary =~ /mRNA/;
	for (@name_order) {
		if ($groups{$_}) {$name=$groups{$_};last;}
	}
#	my ($tag,$name) = split /=/,$groups[0];
#	warn "No name found at [$groups] as the first one !\n" if ! ($tag eq 'ID' or $tag eq 'Parent');
	@dat=($seqname,$primary,$start,$end,$strand,$frame,$groups,$name);
	$$dbh->execute( @dat );
	print "$seqname,$primary,$start,$end,$strand,$frame,$name\n" if $opt_v;
#	sleep 1;
    }
    $$dbh->finish;
    return 1;
}

read_gene_gff($infile,\$sth);

close $infile;
$dbh->commit;

$sql=q/
CREATE INDEX IF NOT EXISTS cse{---} ON gff{---}(chrid,start,end);
CREATE INDEX IF NOT EXISTS n{---} ON gff{---}(name);
/;
for (split /;/,$sql) {
	next if /^\s*$/;
	s/{---}/$opt_s/g;
#print "[$_]\n";
	$dbh->do($_) or warn $dbh->errstr;
}
$dbh->commit;

$dbh->disconnect;

### /dev/shm
#my $shm_real='/dev/shm/'.basename($opt_o);
my $read_time = [gettimeofday];
my $thr1 = async { system 'cp','-pf',$shm_real,$opt_o; };
my $thr2 = async {
	system 'bzip2','-9k',$shm_real;
	system 'mv','-f',$shm_real.'.bz2',$opt_o.'.bz2';
};
$thr1->join();
my $copy_time = [gettimeofday];
$thr2->join();
unlink $shm_real;
unlink $shm_real.'.bz2';
###

#END
my $stop_time = [gettimeofday];

$|=1;
print STDERR "\nTime Elapsed:\t",tv_interval( $start_time, $stop_time ),
	" second(s).\n   Parseing GFF file used\t",tv_interval( $start_time, $read_time ),
	" second(s).\n   Moving  SQLite file used\t",tv_interval( $read_time, $copy_time )," second(s).\n";

print STDERR "\033[32;1m Please use [$opt_s] as Specie name in later steps.\033[0;0m\n";
__END__

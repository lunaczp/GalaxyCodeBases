#!/usr/bin/env perl
use strict;
use warnings;
use Bio::AlignIO;
use DBI;

use Data::Dump qw (ddx);
#use IO::Unread;

use Bio::EnsEMBL::Registry;

## Load the registry automatically
my $reg = "Bio::EnsEMBL::Registry";
#my $url = 'mysql://anonymous@ensembldb.ensembl.org';
my $url = 'mysql://root@localhost';
$reg->load_registry_from_url($url);

my $compara_db_adaptor = $reg->get_DBAdaptor('Multi', 'compara');
my $genome_db_adaptor = $compara_db_adaptor->get_GenomeDBAdaptor();
my $dbh = $compara_db_adaptor->dbc->db_handle;

my $sql = qq{
SELECT DISTINCT ss.genome_db_id, gdb.taxon_id, gdb.name, gdb.assembly FROM species_set_tag sst
 JOIN species_set ss USING (species_set_id)
 JOIN genome_db gdb USING (genome_db_id)
 WHERE sst.value = 'mammals';
};

my $sth = $dbh->prepare($sql);
$sth->execute();

my (@IDsGnomeDB, %GnomeDBnfo);
while(my @retdat = $sth->fetchrow()) {
	#warn join("\t",@retdat),"\n";
	#$GnomeDBnfo{$retdat[0]} = [@retdat[1,2,3]];
	my $GnomedbID = shift @retdat;
	push @IDsGnomeDB,$GnomedbID;
	$GnomeDBnfo{$GnomedbID} = \@retdat;
}

ddx \%GnomeDBnfo;

#my $MammaliaTaxID = 40674; # http://www.ncbi.nlm.nih.gov/Taxonomy/Browser/wwwtax.cgi?mode=Undef&id=40674&lvl=3&keep=1&srchmode=1&unlock
#my $MammaliaGDBs = $genome_db_adaptor->fetch_all_by_ancestral_taxon_id($MammaliaTaxID,1);
#ddx $MammaliaGDBs;

__END__
USE ensembl_compara_80;

SELECT DISTINCT ss.species_set_id, ss1.genome_db_id, gdb1.taxon_id, gdb1.name, ss.genome_db_id, gdb.taxon_id, gdb.name, gdb.assembly FROM species_set ss
  JOIN species_set ss0 ON ss.genome_db_id=ss0.genome_db_id
  JOIN species_set ss1 ON ss.species_set_id=ss1.species_set_id
  JOIN species_set ss2 ON ss1.genome_db_id=ss2.genome_db_id
  JOIN species_set_tag sst0 ON sst0.species_set_id=ss0.species_set_id
  JOIN species_set_tag sst2 ON sst2.species_set_id=ss2.species_set_id
  JOIN genome_db gdb1 ON gdb1.genome_db_id=ss1.genome_db_id
  JOIN genome_db gdb ON gdb.genome_db_id=ss.genome_db_id
  WHERE sst0.value = 'mammals' AND sst2.value = 'mammals' AND ss1.genome_db_id != ss.genome_db_id;
=>5046

SELECT DISTINCT ss.species_set_id FROM species_set ss
  JOIN species_set ss0 ON ss.genome_db_id=ss0.genome_db_id
  JOIN species_set ss1 ON ss.species_set_id=ss1.species_set_id
  JOIN species_set ss2 ON ss1.genome_db_id=ss2.genome_db_id
  JOIN species_set_tag sst0 ON sst0.species_set_id=ss0.species_set_id
  JOIN species_set_tag sst2 ON sst2.species_set_id=ss2.species_set_id
  JOIN genome_db gdb1 ON gdb1.genome_db_id=ss1.genome_db_id
  JOIN genome_db gdb ON gdb.genome_db_id=ss.genome_db_id
  WHERE sst0.value = 'mammals' AND sst2.value = 'mammals' AND ss1.genome_db_id != ss.genome_db_id;
=>746

EXPLAIN SELECT DISTINCT method_link_species_set_id, method_link_species_set.name, ss.species_set_id FROM method_link_species_set
  JOIN method_link USING (method_link_id)
  JOIN species_set ss ON ss.species_set_id=method_link_species_set.species_set_id
  JOIN species_set ss0 ON ss.genome_db_id=ss0.genome_db_id
  JOIN species_set ss1 ON ss.species_set_id=ss1.species_set_id
  JOIN species_set ss2 ON ss1.genome_db_id=ss2.genome_db_id
  JOIN species_set_tag sst0 ON sst0.species_set_id=ss0.species_set_id
  JOIN species_set_tag sst2 ON sst2.species_set_id=ss2.species_set_id
  WHERE sst0.value = 'mammals' AND sst2.value = 'mammals' AND ss1.genome_db_id != ss.genome_db_id AND method_link.type='ENSEMBL_ORTHOLOGUES';
+------+-------------+-------------------------+-------+-----------------------------+----------------+---------+--------------------------------------------------------------------------+------+-----------------------------------------------------------+
| id   | select_type | table                   | type  | possible_keys               | key            | key_len | ref                                                                      | rows | Extra                                                     |
+------+-------------+-------------------------+-------+-----------------------------+----------------+---------+--------------------------------------------------------------------------+------+-----------------------------------------------------------+
|    1 | SIMPLE      | method_link             | const | PRIMARY,type                | type           | 52      | const                                                                    |    1 | Using temporary                                           |
|    1 | SIMPLE      | sst0                    | ALL   | tag_species_set_id          | NULL           | NULL    | NULL                                                                     |    9 | Using where                                               |
|    1 | SIMPLE      | ss0                     | ref   | species_set_id,genome_db_id | species_set_id | 4       | ensembl_compara_80.sst0.species_set_id                                   |   50 | Using where; Using index                                  |
|    1 | SIMPLE      | ss                      | ref   | species_set_id,genome_db_id | genome_db_id   | 5       | ensembl_compara_80.ss0.genome_db_id                                      |   72 |                                                           |
|    1 | SIMPLE      | method_link_species_set | ref   | method_link_id              | method_link_id | 9       | const,ensembl_compara_80.ss.species_set_id                               |   11 |                                                           |
|    1 | SIMPLE      | sst2                    | ALL   | tag_species_set_id          | NULL           | NULL    | NULL                                                                     |    9 | Using where; Distinct; Using join buffer (flat, BNL join) |
|    1 | SIMPLE      | ss2                     | ref   | species_set_id,genome_db_id | species_set_id | 4       | ensembl_compara_80.sst2.species_set_id                                   |   50 | Using where; Using index; Distinct                        |
|    1 | SIMPLE      | ss1                     | ref   | species_set_id,genome_db_id | species_set_id | 9       | ensembl_compara_80.ss.species_set_id,ensembl_compara_80.ss2.genome_db_id |   11 | Using index; Distinct                                     |
+------+-------------+-------------------------+-------+-----------------------------+----------------+---------+--------------------------------------------------------------------------+------+-----------------------------------------------------------+
8 rows in set (0.01 sec)

EXPLAIN SELECT DISTINCT method_link_species_set_id, method_link_species_set.name, method_link_species_set.species_set_id FROM method_link_species_set
  JOIN method_link USING (method_link_id)
  WHERE method_link.type='ENSEMBL_ORTHOLOGUES' AND method_link_species_set.species_set_id IN (
    SELECT DISTINCT ss.species_set_id FROM species_set ss
      JOIN species_set ss0 ON ss.genome_db_id=ss0.genome_db_id
      JOIN species_set ss1 ON ss.species_set_id=ss1.species_set_id
      JOIN species_set ss2 ON ss1.genome_db_id=ss2.genome_db_id
      JOIN species_set_tag sst0 ON sst0.species_set_id=ss0.species_set_id
      JOIN species_set_tag sst2 ON sst2.species_set_id=ss2.species_set_id
      JOIN genome_db gdb1 ON gdb1.genome_db_id=ss1.genome_db_id
      JOIN genome_db gdb ON gdb.genome_db_id=ss.genome_db_id
      WHERE sst0.value = 'mammals' AND sst2.value = 'mammals' AND ss1.genome_db_id != ss.genome_db_id
  );
+------+--------------+-------------------------+--------+-----------------------------+----------------+---------+--------------------------------------------------------------------------+------+-----------------------------------------------------------+
| id   | select_type  | table                   | type   | possible_keys               | key            | key_len | ref                                                                      | rows | Extra                                                     |
+------+--------------+-------------------------+--------+-----------------------------+----------------+---------+--------------------------------------------------------------------------+------+-----------------------------------------------------------+
|    1 | PRIMARY      | method_link             | const  | PRIMARY,type                | type           | 52      | const                                                                    |    1 | Using temporary                                           |
|    1 | PRIMARY      | method_link_species_set | ALL    | method_link_id              | NULL           | NULL    | NULL                                                                     | 2633 | Using where                                               |
|    1 | PRIMARY      | <subquery2>             | eq_ref | distinct_key                | distinct_key   | 4       | func                                                                     |    1 | Distinct                                                  |
|    2 | MATERIALIZED | sst0                    | ALL    | tag_species_set_id          | NULL           | NULL    | NULL                                                                     |    9 | Using where; Distinct                                     |
|    2 | MATERIALIZED | ss0                     | ref    | species_set_id,genome_db_id | species_set_id | 4       | ensembl_compara_80.sst0.species_set_id                                   |   50 | Using where; Using index; Distinct                        |
|    2 | MATERIALIZED | gdb                     | eq_ref | PRIMARY                     | PRIMARY        | 4       | ensembl_compara_80.ss0.genome_db_id                                      |    1 | Using index; Distinct                                     |
|    2 | MATERIALIZED | ss                      | ref    | species_set_id,genome_db_id | genome_db_id   | 5       | ensembl_compara_80.ss0.genome_db_id                                      |   72 | Distinct                                                  |
|    2 | MATERIALIZED | sst2                    | ALL    | tag_species_set_id          | NULL           | NULL    | NULL                                                                     |    9 | Using where; Distinct; Using join buffer (flat, BNL join) |
|    2 | MATERIALIZED | ss2                     | ref    | species_set_id,genome_db_id | species_set_id | 4       | ensembl_compara_80.sst2.species_set_id                                   |   50 | Using where; Using index; Distinct                        |
|    2 | MATERIALIZED | ss1                     | ref    | species_set_id,genome_db_id | species_set_id | 9       | ensembl_compara_80.ss.species_set_id,ensembl_compara_80.ss2.genome_db_id |   11 | Using index; Distinct                                     |
|    2 | MATERIALIZED | gdb1                    | eq_ref | PRIMARY                     | PRIMARY        | 4       | ensembl_compara_80.ss2.genome_db_id                                      |    1 | Using index; Distinct                                     |
+------+--------------+-------------------------+--------+-----------------------------+----------------+---------+--------------------------------------------------------------------------+------+-----------------------------------------------------------+
11 rows in set (0.02 sec)

SELECT DISTINCT method_link_species_set_id, method_link_species_set.name, ss.species_set_id FROM method_link_species_set
  JOIN method_link USING (method_link_id)
  JOIN species_set ss ON ss.species_set_id=method_link_species_set.species_set_id
  JOIN species_set ss0 ON ss.genome_db_id=ss0.genome_db_id
  JOIN species_set ss1 ON ss.species_set_id=ss1.species_set_id
  JOIN species_set ss2 ON ss1.genome_db_id=ss2.genome_db_id
  JOIN species_set_tag sst0 ON sst0.species_set_id=ss0.species_set_id
  JOIN species_set_tag sst2 ON sst2.species_set_id=ss2.species_set_id
  WHERE sst0.value = 'mammals' AND sst2.value = 'mammals' AND ss1.genome_db_id != ss.genome_db_id AND method_link.type='ENSEMBL_ORTHOLOGUES';
=>741 rows in set (1.02 sec)


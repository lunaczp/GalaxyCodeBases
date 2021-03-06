samtools depth Blood-MDA.sort.bam Blood-MAL.sort.bam Sperm23-MDA.sort.bam Sperm24-MDA.sort.bam Sperm28-MDA.sort.bam SpermS01.sort.bam SpermS02.sort.bam SpermS03.sort.bam | gzip -c > depths.gz

# zcat depths.gz|head -3000|perl -lane 'BEDIN {@SUM;$CNT} next if $F[0]=~/[XYM]/i; $chr=shift @F; $pos=shift @F; for $i (0 .. $#F) { $SUM[$i]+=$F[$i]; ++$CNT } END { $_ = int(1000*$_/$CNT)/1000 for (@SUM);print "@SUM"; }'
# zcat depths.gz|perl -lane 'BEDIN {@SUM;$CNT} next if $F[0]=~/[XYM]/i; $chr=shift @F; $pos=shift @F; for $i (0 .. $#F) { $SUM[$i]+=$F[$i]; ++$CNT } END { $_ = int(1000*$_/$CNT)/1000 for (@SUM);print "@SUM"; }'
# zcat depths.gz|perl -lane 'BEDIN {@SUM;$CNT} next if $F[0]=~/[XYM]/i; $chr=shift @F; $pos=shift @F; for $i (0 .. $#F) { $SUM[$i]+=$F[$i]; ++$CNT } print "@SUM"; END { $_ = int(1000*$_/$CNT)/1000 for (@SUM);print "@SUM"; }'
# zcat depths.gz|perl -lane 'BEDIN {@SUM;$CNT} next if $F[0]=~/[XYM]/i; $chr=shift @F; $pos=shift @F; for $i (0 .. $#F) { $SUM[$i]+=$F[$i]; ++$CNT } print "$chr:$pos\t@SUM" unless $CNT%1000000 ; END { $_ = int(1000*$_/$CNT)/1000 for (@SUM);print "@SUM"; }'
# zcat depths.gz|perl -lane 'BEDIN {@SUM;$CNT} next if $F[0]=~/[XYM]/i; $chr=shift @F; $pos=shift @F; for $i (0 .. $#F) { $SUM[$i]+=$F[$i]; ++$CNT } print "$chr:$pos\t@SUM\t",$SUM[1]/(0.00000001+$SUM[0]) unless $CNT%1000000 ; END { $_ = int(1000*$_/$CNT)/1000 for (@SUM);print "@SUM"; }'


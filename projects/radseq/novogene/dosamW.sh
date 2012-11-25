#!/bin/bash
#$ -v PERL5LIB,PATH,PYTHONPATH,LD_LIBRARY_PATH
#$ -cwd -r y -l vf=3.2g,p=5
#$ -e /dev/null
#$ -S /bin/bash -t 1-20

MY_HOST=`/bin/hostname`
MY_DATE=`/bin/date`
env
echo "================================================================"
echo JOB_NAME=$JOB_NAME
echo JOB_ID=$JOB_ID
echo SGE_TASK_ID=$SGE_TASK_ID
echo SGE_TASK_FIRST=$SGE_TASK_FIRST
echo SGE_TASK_LAST=$SGE_TASK_LAST
echo NSLOTS=$NSLOTS
echo QUEUE=$QUEUE
echo SGE_CWD_PATH=$SGE_CWD_PATH
echo PATH=$PATH
echo SGE_STDIN_PATH=$SGE_STDIN_PATH
echo SGE_STDOUT_PATH=$SGE_STDOUT_PATH
echo SGE_STDERR_PATH=$SGE_STDERR_PATH
echo SGE_O_HOST=$SGE_O_HOST
echo SGE_O_PATH=$SGE_O_PATH
echo "================================================================"

echo "Running job JOB_NAME=$JOB_NAME task SGE_TASK_ID=$SGE_TASK_ID on $MY_HOST at $MY_DATE"
echo "Running job JOB_NAME=$JOB_NAME task SGE_TASK_ID=$SGE_TASK_ID on $MY_HOST at $MY_DATE" >out/p/$MAIN.samlog

SEEDFILE=ng/fqmW.lst
SEED=$(/bin/sed -n -e "${SGE_TASK_ID} p" $SEEDFILE)
PATH=$(/bin/sed -n -e "${SGE_TASK_ID} p" $SEEDFILE|/bin/awk -F"\t" '{print $1}')
MAIN=$(/bin/sed -n -e "${SGE_TASK_ID} p" $SEEDFILE|/bin/awk -F"\t" '{print $2}')
/usr/local/bin/bwa aln -l 17 -q 10 -t 6 cat62 $PATH/$MAIN.1.fq.gz >out/p/$MAIN.1.sai 2>>out/p/$MAIN.samlog
/bin/date >> out/p/$MAIN.samlog
/usr/local/bin/bwa aln -l 17 -q 10 -t 6 cat62 $PATH/$MAIN.2.fq.gz >out/p/$MAIN.2.sai 2>>out/p/$MAIN.samlog
/bin/date >> out/p/$MAIN.samlog
/usr/local/bin/bwa sampe -a 800 cat62 out/p/$MAIN.1.sai out/p/$MAIN.2.sai $PATH/$MAIN.1.fq.gz $PATH/$MAIN.2.fq.gz 2>>out/p/$MAIN.samlog | /bin/gzip -9c >out/p/$MAIN.rawsam.gz
/bin/date >> out/p/$MAIN.samlog
/usr/bin/perl ng/pickR1orR2XTUreads.pl out/p/$MAIN.rawsam.gz out/p/$MAIN.sam.gz
/bin/date >> out/p/$MAIN.samlog
/usr/local/bin/samtools view -bS out/p/$MAIN.sam.gz >out/p/$MAIN.bam 2>>out/p/$MAIN.samlog
/usr/local/bin/samtools sort -m 3200000000 out/p/$MAIN.bam out/p/$MAIN.sort
#/usr/local/bin/samtools rmdup out/p/$MAIN.sort.bam out/p/$MAIN.rmdup.bam
/bin/date >> out/p/$MAIN.samlog


#include <stdio.h>
#include <stdlib.h> //EXIT_FAILURE
#include <err.h>
#include <string.h>
#include <ctype.h>
#include "cfgparser.h"

/*
 *去除字符串右端空格
 */
char *strtrimr(char *pstr)
{
    int i;
    i = strlen(pstr) - 1;
    while (isspace(pstr[i]) && (i >= 0))
        pstr[i--] = '\0';
    return pstr;
}
/*
 *去除字符串左端空格
 */
char *strtriml(char *pstr)
{
    int i = 0,j;
    j = strlen(pstr) - 1;
    while (isspace(pstr[i]) && (i <= j))
        i++;
    if (0<i)
        strcpy(pstr, &pstr[i]);
    return pstr;
}
/*
 *去除字符串两端空格
 */
char *strtrim(char *pstr)
{
    char *p;
    p = strtrimr(pstr);
    return strtriml(p);
}

void write_example_cfg(const char * const filename) {
    fputs(".\b", stderr);
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) err(EXIT_FAILURE, "Cannot write to [%s]", filename);
    fputs("\
# Estimated Genome Size in M bp\n\
Genome_Size=3000\n\
\n\
# Heterozygosity rate\n\
Het=0.01\n\
\n\
# DNA sequence error rate\n\
Seq_Err=0.02\n\
\n\
# Sequence Depth\n\
Seq_Depth=20\n\
\n\
# Mutation to extra kmer effective ratio (that less than 'k' or '2k-1')\n\
Mut_Effect=0.8\n\
\n\
[Sequence Files]\n\
./fullpath/to/fasta.fa\n\
./fullpath/to/fastaq.fq.any.filename.is.OK\n\
./fullpath/to/fasta.or.fastaq.gz\n\
", fp);
    if ( fclose(fp) )
        err(EXIT_FAILURE, "Error writing to [%s]", filename);
    fputs("\n", stderr);
}

SDLConfig *read_SDL_cfg(const char * const filename){
    SDLConfig *psdlcfg = malloc(sizeof(SDLConfig));
    psdlcfg->fp = fopen(filename, "r");
    psdlcfg->seqfilename=NULL;
    psdlcfg->seqfilename_length=0u;
    if (psdlcfg->fp == NULL) err(EXIT_FAILURE, "Cannot write to [%s]", filename);
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    unsigned char ConfigCount=0;
    while ((read = getline(&line, &len, psdlcfg->fp)) != -1) {
        if (*line=='\n' || line[0]=='#') continue;
        if (line[0]=='[' && line[read-2]==']') {
            *(line+(--read))='\0';  //chop
            break;
        }
        if (*(line+read-1)=='\n') *(line+(--read))='\0';    // line[read-1] = '\0';
        char *p2 = strchr(line, '=');
        if (p2 == NULL) err(EXIT_FAILURE, "Invalid file format:[%s]. See example input_config.", line);
        *p2++ = '\0';
        p2 = strtrim(p2);
        line = strtrim(line);
        if (strcmp(line,"Genome_Size")==0) {
            psdlcfg->GenomeSize=atof(p2);
            ++ConfigCount;
            continue;
        }
        if (strcmp(line,"Het")==0) {
            psdlcfg->HetRatio=atof(p2);
            ++ConfigCount;
            continue;
        }
        if (strcmp(line,"Seq_Err")==0) {
            psdlcfg->SeqErrRate=atof(p2);
            ++ConfigCount;
            continue;
        }
        if (strcmp(line,"Seq_Depth")==0) {
            psdlcfg->SeqDepth=atof(p2);
            ++ConfigCount;
            continue;
        }
        if (strcmp(line,"Mut_Effect")==0) {
            psdlcfg->MutEffect=atof(p2);
            ++ConfigCount;
            continue;
        }
    }
    if (SDLConfig_Count != ConfigCount)
        err(EXIT_FAILURE, "Invalid file format:(%u != %u). See example input_config.", ConfigCount,SDLConfig_Count);
    if (! strcmp(line,"[Sequence Files]") ) {
        err(EXIT_FAILURE, "The first section is \"%s\", not \"[Sequence Files]\". See example input_config.", line);
    }
    free(line);
    return psdlcfg;
}

ssize_t get_next_seqfile(SDLConfig * const psdlcfg){
    ssize_t read = getline(&(psdlcfg->seqfilename), &(psdlcfg->seqfilename_length), psdlcfg->fp);
    return read;
}

void destory_seqfile(SDLConfig * psdlcfg){
    fclose(psdlcfg->fp);
    free(psdlcfg->seqfilename);
    free(psdlcfg);
}

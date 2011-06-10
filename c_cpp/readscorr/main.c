#define _GNU_SOURCE
#include <stdlib.h> //EXIT_FAILURE
#include <stdio.h>
#include <stdint.h>
#include <string.h>
//#include <errno.h>
#include <err.h>
#include <argp.h>
#include <math.h>
#include "MurmurHash3.h"
#include "getch.h"
#include "2bitarray.h"
#include "gFileIO.h"
#include "sdleft.h"
#include "chrseq.h"
#include "cfgparser.h"

const char *argp_program_version =
    "readscorr 0.1 @"__TIME__ "," __DATE__;
const char *argp_program_bug_address =
    "<huxuesong@genomics.org.cn>";

/* Program documentation. */
static char doc[] =
    "a Solexa Reads Kmer Counter using simplied D-Left hashing"
#ifdef DEBUG
    " (Debug Version)"
#endif
;

/* A description of the arguments we accept. */
static char args_doc[] = "input_config (of nfo & FASTA or FASTQ files)";

/* The options we understand. */
static struct argp_option options[] = {
    {"interactive", 'i', 0,   0,  "Prompt arguments before procedure"},
    {"verbose",  'v', 0,      0,  "Produce verbose output" },
    {"quiet",    'q', 0,      0,  "Don't produce any output" },
    //{"silent",   's', 0,      OPTION_ALIAS },
    {"outprefix",'o', "./out",0,  "Output to [./out.{dat,stat,log}]" },
    {"kmersize", 'k', "21",   0,  "K-mer size, must be odd number" },
    {"countbit",'c', "9",  0,  "length of kmer freq. Count in bit"  },
    {"arrayklen",'a', "512",  0,  "Size in kilo(x1024) for Simplied D-Left Array"  },
    {"subarray",'s', "32",  0,  "Size of single sub array of the SDLA"  },
    {"remainkeybit",'r', "31",  0,  "length of SDLA Remain-Key in bit.\nWill elongate to align(8) with countbit."  },
    {"example",  'e', 0,      0,  "OVERWRITE an example to [input_config]"},
    { 0 }
};

/* Used by main to communicate with parse_opt. */
struct arguments {
    char *args[1];                /* arg1 */
    uint_fast8_t silent, verbose, interactive, writexample;   //_Bool is the same under amd64, as typedef unsigned char uint_fast8_t;
    int kmersize;
    unsigned char CountBit, rBit;
    size_t ArraySizeK;
    uint16_t SubItemCount;
    char *outprefix;
};

/* Parse a single option. */
static error_t
parse_opt (int key, char *arg, struct argp_state *state) {
/* Get the input argument from argp_parse, which we
  know is a pointer to our arguments structure. */
    struct arguments *arguments = state->input;
    
    int tmpArgValue;
    switch (key) {
        case 'q':// case 's':
            arguments->silent = 1;
            break;
        case 'v':
            arguments->verbose = 1;
            break;
        case 'e':
            arguments->writexample = 1;
            break;
        case 'i':
            arguments->interactive = 1;
            break;
        case 'o':
            arguments->outprefix = arg;
            break;
        case 'k':
            tmpArgValue = atoi(arg);
            if (tmpArgValue>2 && tmpArgValue%2 && tmpArgValue <= UINT16_MAX) {   // odd numbers only
               arguments->kmersize = tmpArgValue;
            } else {
               errx(2,"-k \"%s\"=%i is not a odd number of [3,%d] !",arg,tmpArgValue,UINT16_MAX);
            }
            break;
        case 'c':
            tmpArgValue = atoi(arg);
            if (tmpArgValue>0 && tmpArgValue <= 64) {
               arguments->CountBit = tmpArgValue;
            } else {
               errx(2,"-c \"%s\"=%i is not a integer of [1,64] !",arg,tmpArgValue);
            }
            break;
        case 'a':
            tmpArgValue = atoi(arg);
            if (tmpArgValue>0 && tmpArgValue <= SIZE_MAX/1024) {
               arguments->ArraySizeK = tmpArgValue;
            } else {
               errx(2,"-a \"%s\"=%i is not a integer of [1,%zu] !",arg,tmpArgValue,SIZE_MAX/1024);
            }
            break;
        case 's':
            tmpArgValue = atoi(arg);
            if (tmpArgValue>0 && tmpArgValue <= UINT16_MAX) {
               arguments->SubItemCount = tmpArgValue;
            } else {
               errx(2,"-s \"%s\"=%i is not a integer of [1,%u] !",arg,tmpArgValue,UINT16_MAX);
            }
            break;
        case 'r':
            tmpArgValue = atoi(arg);
            if (tmpArgValue>0 && tmpArgValue <= 64) {
               arguments->rBit = tmpArgValue;
            } else {
               errx(2,"-r \"%s\"=%i is not a integer of [1,64] !",arg,tmpArgValue);
            }
            break;
        
        case ARGP_KEY_ARG:
            if (state->arg_num >= 1)
             /* Too many arguments. */
             argp_usage (state);
            arguments->args[state->arg_num] = arg;
            break;
        
        case ARGP_KEY_END:
            if (state->arg_num < 1)   /* Not enough arguments. */
               //argp_usage (state);
               argp_state_help (state,stderr,ARGP_HELP_STD_HELP);
            break;
        
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

/* Our argp parser. */
static struct argp argp = { options, parse_opt, args_doc, doc };

char *strlinker(char *main, char *suffix) {
    char *outstr = malloc(strlen(main)+strlen(suffix)+1);
    strcpy(outstr,main);
    strcat(outstr,suffix);
    return outstr;
}

int main (int argc, char **argv) {
    struct arguments arguments;
    
    // Default values.
    arguments.silent = 0;
    arguments.verbose = 0;
    arguments.interactive = 0;
    arguments.writexample = 0;
    arguments.outprefix = "./out";
    arguments.kmersize = 21;
    arguments.CountBit = 9;
    arguments.rBit = 31;
    arguments.ArraySizeK = 512;
    arguments.SubItemCount = 32;
    
    // Parse our arguments; every option seen by parse_opt will be reflected in arguments.
    argp_parse (&argp, argc, argv, 0, 0, &arguments);
    
    if (arguments.writexample) {
        printf("[!] Going to OVERWRITE an example to [%s] !\n",arguments.args[0]);
        pressAnyKey();
        write_example_cfg(arguments.args[0]);
        exit(EXIT_SUCCESS);
    }
    if (arguments.interactive) {
      unsigned char rBit = arguments.rBit;
      uint16_t SubItemCount = arguments.SubItemCount;
      unsigned char itemByte = GETitemByte_PADrBit_trimSubItemCount(arguments.CountBit,&rBit,&SubItemCount);
      uint64_t SDLsize = SubItemCount*itemByte*arguments.ArraySizeK*1024;
      printf ("input_config = %s\nOutputPrefix = %s\n"
           "VERBOSE = %s\nSILENT = %s\n"
           "SDLA { CountBit:%u. rBit:%u(%u), ArraySize:%lu(k), SubItemCount:%u(%u) }\n"
           "kmersize = %i, SDLAsize = %lu MiB (%lu Bytes)\n",
           arguments.args[0],
           arguments.outprefix,
           arguments.verbose ? "yes" : "no",
           arguments.silent ? "yes" : "no",
           arguments.CountBit,rBit,arguments.rBit,arguments.ArraySizeK,SubItemCount,arguments.SubItemCount,
           arguments.kmersize,SDLsize/1048576,SDLsize);
      pressAnyKey();
    }
    char *outStat = strlinker(arguments.outprefix, ".stat");
    char *outDat  = strlinker(arguments.outprefix, ".dat");
    char *outLog  = strlinker(arguments.outprefix, ".log");
//printf("Out[%s][%s][%s]\n",outStat,outDat,outLog);
    SDLConfig *psdlcfg=read_SDL_cfg((double)arguments.kmersize,arguments.args[0]);
    ssize_t read;
    char *line;
    uint64_t insertedCount=0;

    SDLeftArray_t *dleftp = dleft_arrayinit(29,27,1000,48); // (9,31,5000000,48);
    fputs("SDLA nfo: ", stderr);
    fprintSDLAnfo(stderr,dleftp);

    fputs("\nParsing Sequence Files:\n", stderr);
    while ((read = get_next_seqfile(psdlcfg)) != -1) {
    	ssize_t readlength;
    	line=psdlcfg->seqfilename;
        fprintf(stderr, " <%s> ...", line);
        //sleep(1);    // the Call ...
        SeqFileObj *seqobj = inSeqFinit(line,GFIOCHRBASE);
        if (seqobj) {
        	while ( (readlength = (*seqobj->getNextSeq)(seqobj) >= 0) ) {
        	#ifdef DEBUG
        		puts(line);
	        	printf("-ID:[%s,%s] %zu %zu\nSeq:[%s]\nQ:[%s] *%zx,%u\n",
        			seqobj->name,seqobj->comment,seqobj->readlength,seqobj->binMallocedQQWord,
				    seqobj->seq,seqobj->qual,(size_t)seqobj->seq,seqobj->type);
				/*char *tmpseq;
			    //NormalizeChrSeq((char*)seqobj->seq);
			    printf("rc[%s]\n",tmpseq=ChrSeqRevComp(seqobj->seq,seqobj->readlength));
			    free(tmpseq);*/
			#endif
                size_t insertedKmer = dleft_insert_read(arguments.kmersize,seqobj->seq,seqobj->readlength,dleftp);
                insertedCount += insertedKmer;
            #ifdef DEBUG
                printf("Inserted:[%zu of %lu]\n",insertedKmer,insertedCount);
            #endif
        	}
        } else continue;
        fputs("\b\b\b\b, done !\n", stderr);
        fprintf(stderr, "[!]Inserted Kmer:[%lu] times\n", insertedCount);
        inSeqFdestroy(seqobj);
    }
/*
    rewind(fp);
    fputs("\nSecond pass:\n", stderr);
    while ((read = getline(&line, &len, fp)) != -1) {
        if (*(line+read-1)=='\n') *(line+(--read))='\0';
        printf("[%zu]+%zu <%s>\n", read, len, line);
    }
*/
    fputs("\nHashing Done!\n", stderr);
    destory_seqfile(psdlcfg);
    fputs("SDLA nfo: ", stderr);
    fprintSDLAnfo(stderr,dleftp);
    FILE *fp;
    fp = fopen(outStat, "w");
    fprintf(fp,"#KmerSize:%d\n#Kmer_theory_count:%.34Lg\n",arguments.kmersize,powl(4.0,(long double)arguments.kmersize));   // 34 from http://en.wikipedia.org/wiki/Quadruple_precision
    SDLeftStat_t *SDLeftStat = dleft_stat(dleftp,fp);
    fclose(fp);
    fp = fopen(outDat, "w");
    SDLdumpHead *DatHead = malloc(sizeof(SDLdumpHead));
    DatHead->kmersize = arguments.kmersize;
    DatHead->HistMaxCntVal = SDLeftStat->HistMaxCntVal;
    DatHead->HistMaxHistVal = SDLeftStat->HistMaxHistVal;
    DatHead->HistMean = SDLeftStat->HistMean;
    DatHead->HistSStd = SDLeftStat->HistSStd;
    free(SDLeftStat);
    dleft_dump(dleftp,DatHead,fp);
    fclose(fp);
    free(DatHead);
    dleft_arraydestroy(dleftp);
    
    free(outStat); free(outDat); free(outLog);
    exit(EXIT_SUCCESS);
}

// by Hu Xuesong
#ifndef _G_SDLEFT_H
#define _G_SDLEFT_H

#include "gtypendef.h"
#include <stddef.h> //size_t
#include <stdint.h> //uint64_t
#include <stdio.h>  //FILE
#include "gFileIO.h"
#include "cfgparser.h"

#define HASH_LENB (128u)
//#define SDLA_ITEMARRAY 32u
#define SUBARRAY_SIZE (32u*1024u)
//gcc -### -march=native -E /usr/include/stdlib.h 2>&1 | grep l1-cache-size

typedef struct __SDLeftArray_t {
    unsigned char CountBit, rBit, ArrayBit;
    unsigned char itemByte; //, HashCnt;
    uint16_t SubItemCount;  //max should be 32768
    size_t ArraySize,SDLAbyte;
    //uint64_t maxCount; == Item_CountBitMask
    //unsigned char ArrayCount;
    uint64_t ItemInsideAll, CellOverflowCount, CountBitOverflow; // ItemInsideAll = ItemInsideArray + CellOverflowCount
    double FalsePositiveRatio;
    void *pDLA, *pextree;
    uint64_t maxCountSeen;
    //uint64_t *outhash;
    uint64_t outhash[2];    // both ArrayBit and rBit is (0,64], so HashCnt==1 for MurmurHash3_x64_128
    uint128_t Item_rBitMask;
    uint64_t Hash_ArrayBitMask, Hash_rBitMask, Item_CountBitMask;
} SDLeftArray_t;

typedef struct __SDLdumpHead_t {
    char FileID[4]; //"GDSD"
    unsigned char FileVersion[2];    //0,1
    uint16_t kmersize;
    unsigned char CountBit, rBit;
    uint16_t SubItemCount;
    uint64_t ArraySize, SDLAbyte, extreebyte;
    uint64_t ItemInsideAll, CellOverflowCount, CountBitOverflow;
    uint64_t maxCountSeen;
    uint64_t HistMaxCntVal;
    uint64_t HistMaxHistVal;
    double HistMean;
    double HistSStd;
    uint32_t crc32c;
} __attribute__ ((packed)) SDLdumpHead;

SDLeftArray_t *dleft_arraynew(unsigned char CountBit, const SDLConfig * const psdlcfg);
SDLeftArray_t *dleft_arrayinit(unsigned char CountBit, unsigned char rBit, size_t ArraySize, uint16_t SubItemCount);
size_t dleft_insert_read(unsigned int k, char const *const inseq, size_t len, SDLeftArray_t *dleftobj);

unsigned char GETitemByte_PADrBit_trimSubItemCount(const unsigned char CountBit, unsigned char *prBit, uint16_t *pSubItemCount);

void fprintSDLAnfo(FILE *stream, const SDLeftArray_t * dleftobj);
void dleft_arraydestroy(SDLeftArray_t * const dleftobj);
void dleft_dump(const SDLeftArray_t * const, SDLdumpHead * const, FILE *);

//#include "sdleftTF.h"
typedef struct __SDLeftStat_t {
    uint64_t HistMaxCntVal;
    uint64_t HistMaxHistVal;
    double HistMean;
    double HistSStd;
} SDLeftStat_t;

typedef SDLeftStat_t *(G_SDLeftArray_IN)(SDLeftArray_t * const, FILE *);    //*G_SDLeftArray_IN() is OK,too .

SDLeftStat_t * dleft_stat(SDLeftArray_t * const dleftobj, FILE *stream);

#endif /* sdleft.h */

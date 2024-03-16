// Patched version of BZip2 v0.1, which now compiles under
// Open Watcom for Win16 target. Original code has been remade
// as a "ready to use" library for decompressing data arrays.

// This (ugly) patched version by Magnetic-Fox, 16th March 2024.

// IMPORTANT: THIS IS NOT AN ORIGINAL CODE OF BZIP2 NOR I'M
// THE AUTHOR OF THIS WONDERFUL COMPRESSION/DECOMPRESSION TOOL!
// FOR THE MOST TESTED AND ERROR FREE VERSION YOU SHOULD ALWAYS
// USE OFFICIAL AND UP-TO-DATE VERSION OF BZIP2 CODE, WHICH CAN
// BE FOUND HERE:

// https://sourceware.org/bzip2/

#ifndef UNBZIP2_H
#define UNBZIP2_H

#include <stdlib.h>
#include <malloc.h>

#ifdef __cplusplus
    extern "C"
    {
#endif

#define Int32               long int __huge
#define UInt32              unsigned long int __huge
#define Int16               int __huge
#define UInt16              unsigned int __huge
#define Char                char __huge
#define UChar               unsigned char __huge

#define Bool                unsigned char __huge
#define True                1
#define False               0

#define IntNative           int __huge

#define NUM_OVERSHOOT_BYTES 20

#define MAX_ALPHA_SIZE      258
#define MAX_CODE_LEN        23

#define RUNA                0
#define RUNB                1

#define N_GROUPS            6
#define G_SIZE              50
#define N_ITERS             4

#define LESSER_ICOST        0
#define GREATER_ICOST       15

#define SMALL_THRESH        20
#define DEPTH_THRESH        10

#define QSORT_STACK_SIZE    1000

#define MAX_SELECTORS       (2L + (900000L / G_SIZE))

#define WEIGHTOF(zz0)       ((zz0) & 0xffffff00L)
#define DEPTHOF(zz1)        ((zz1) & 0x000000ffL)
#define MYMAX(zz2,zz3)      ((zz2) > (zz3) ? (zz2) : (zz3))
#define BIGFREQ(b)          (ftab[((b)+1L) << 8L] - ftab[(b) << 8L])
#define SETMASK             (1L << 21L)
#define CLEARMASK           (~(SETMASK))

#define UPDATE_CRC(crcVar,cha) {            \
    crcVar = (crcVar << 8) ^                \
            crc32Table[(crcVar >> 24) ^     \
                        ((UChar)cha)];      \
}

#define bsNEEDR(nz) {                           \
    while (bsLive < nz) {                       \
        Int32 zzi = bsStream[srcBufferPos++];   \
        if(srcBufferPos>=srcBufferMax) {        \
            zzi=0;                              \
        }                                       \
        bsBuff = (bsBuff << 8) | (zzi & 0xffL); \
        bsLive += 8;                            \
    }                                           \
}

#define bsR1(vz) {                      \
    bsNEEDR(1);                         \
    vz = (bsBuff >> (bsLive-1)) & 1;    \
    bsLive--;                           \
}

#define ADDWEIGHTS(zw1,zw2)                 \
    (WEIGHTOF(zw1)+WEIGHTOF(zw2)) |         \
    (1 + MYMAX(DEPTHOF(zw1),DEPTHOF(zw2)))

#define UPHEAP(z) {                                 \
    Int32 zz, tmp;                                  \
    zz = z; tmp = heap[zz];                         \
    while (weight[tmp] < weight[heap[zz >> 1]]) {   \
        heap[zz] = heap[zz >> 1];                   \
        zz >>= 1;                                   \
    }                                               \
    heap[zz] = tmp;                                 \
}

#define DOWNHEAP(z) {                               \
    Int32 zz, yy, tmp;                              \
    zz = z; tmp = heap[zz];                         \
    while (True) {                                  \
        yy = zz << 1;                               \
        if (yy > nHeap) break;                      \
        if (yy < nHeap &&                           \
            weight[heap[yy+1]] < weight[heap[yy]])  \
            yy++;                                   \
        if (weight[tmp] < weight[heap[yy]]) break;  \
        heap[zz] = heap[yy];                        \
        zz = yy;                                    \
    }                                               \
    heap[zz] = tmp;                                 \
}

#define SET_LL4(i,n) {                                          \
    if (((i) & 0x1) == 0)                                       \
        ll4[(i) >> 1] = (ll4[(i) >> 1] & 0xf0) | (n); else      \
        ll4[(i) >> 1] = (ll4[(i) >> 1] & 0x0f) | ((n) << 4);    \
    }

#define GET_LL4(i)                          \
    (((UInt32)(ll4[(i) >> 1])) >> (((i) << 2) & 0x4) & 0xF)

#define SET_LL(i,n) {                       \
    ll16[i] = (UInt16)(n & 0x0000ffff);     \
    SET_LL4(i, n >> 16);                    \
}

#define GET_LL(i)                           \
    (((UInt32)ll16[i]) | (GET_LL4(i) << 16))

#define GET_MTF_VAL(lval) {                 \
    Int32 zt, zn, zvec, zj;                 \
    if (groupPos == 0) {                    \
        groupNo++;                          \
        groupPos = G_SIZE;                  \
    }                                       \
    groupPos--;                             \
    zt = selector[groupNo];                 \
    zn = minLens[zt];                       \
    zvec = bsR ( zn );                      \
    while (zvec > limit[zt][zn]) {          \
        zn++; bsR1(zj);                     \
        zvec = (zvec << 1) | zj;            \
    };                                      \
    lval = perm[zt][zvec - base[zt][zn]];   \
}

#define swap(lv1, lv2)                          \
    { Int32 tmp = lv1; lv1 = lv2; lv2 = tmp; }

#define push(lz,hz,dz) { stack[sp].ll = lz; \
                         stack[sp].hh = hz; \
                         stack[sp].dd = dz; \
                         sp++; }

#define pop(lz,hz,dz) { sp--;               \
                        lz = stack[sp].ll;  \
                        hz = stack[sp].hh;  \
                        dz = stack[sp].dd; }

#define RAND_DECLS      \
    Int32 rNToGo = 0;   \
    Int32 rTPos  = 0;   \

#define RAND_MASK ((rNToGo == 1) ? 1 : 0)

#define RAND_UPD_MASK                           \
    if (rNToGo == 0) {                          \
        rNToGo = rNums[rTPos];                  \
        rTPos++; if (rTPos == 512) rTPos = 0;   \
    }                                           \
    rNToGo--;

void initialiseCRC(void);
UInt32 getFinalCRC(void);
UInt32 getGlobalCRC(void);
void setGlobalCRC(UInt32);
void bsSetStream(UChar*, Bool);
void bsFinishedWithStream(void);
UInt32 bsR(Int32);
UChar bsGetUChar(void);
UInt32 bsGetUInt32(void);
UInt32 bsGetIntVS(UInt32);
Int32 bsGetInt32(void);
void hbMakeCodeLengths(UChar*, Int32*, Int32, Int32);
void hbAssignCodes(Int32*, UChar*, Int32, Int32, Int32);
void hbCreateDecodeTables(Int32*, Int32*, Int32*, UChar*, Int32, Int32, Int32);
void setDecompressStructureSizes(Int32);
void makeMaps(void);
void recvDecodingTables(void);
void getAndMoveToFrontDecode(void);
Bool fullGtU(Int32, Int32);
void simpleSort(Int32, Int32, Int32);
void vswap(Int32, Int32, Int32);
UChar med3(UChar, UChar, UChar);
void qSort3(Int32, Int32, Int32);
void sortIt(void);
Int32 indexIntoF(Int32, Int32*);
void undoReversibleTransformation_small(UChar*);
void undoReversibleTransformation_fast(UChar*);
Bool uncompressStream(UChar*, UChar*);
Bool initialize(void);
void freeAll(void);
void setSourceBufferSize(UInt32);
void setDestinationBufferSize(UInt32);
UInt32 uncompressData(UChar*, UInt32, UChar*, UInt32);
UInt32 uncompressDataInPlace(UChar*, UInt32, UInt32);

#ifdef __cplusplus
    } /* extern "C" */
#endif

#endif

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

// Below is the part of the original comment from bzip2.c file,
// version 0.1 from August 1997 to include information about
// people, whose work was used to create this code.
// There are also some occasional original comments in the code.

// ORIGINAL COMMENT STARTS HERE
/*-----------------------------------------------------------*/
/*--- A block-sorting, lossless compressor        bzip2.c ---*/
/*-----------------------------------------------------------*/

/*--
  This program is bzip2, a lossless, block-sorting data compressor,
  version 0.1pl0, dated 17-Aug-1997.

  Copyright (C) 1996, 1997 by Julian Seward.
     Guildford, Surrey, UK
     email: jseward@acm.org

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

  The GNU General Public License is contained in the file LICENSE.

  This program is based on (at least) the work of:
     Mike Burrows
     David Wheeler
     Peter Fenwick
     Alistair Moffat
     Radford Neal
     Ian H. Witten
     Robert Sedgewick
     Jon L. Bentley

  For more information on these sources, see the file ALGORITHMS.
--*/
// ORIGINAL COMMENT ENDS HERE

#include "unbzip2.h"

UInt32  bytesOut;
Bool    smallMode;
UInt32  globalCrc;

UChar   *block;
UInt16  *quadrant;
Int32   *zptr;
UInt16  *szptr;
Int32   *ftab;

UInt16  *ll16;
UChar   *ll4;

Int32   *tt;
UChar   *ll8;

Int32   unzftab[256];
Int32   last;
Int32   origPtr;
Int32   blockSize100k;
Int32   workFactor;
Int32   workDone;
Int32   workLimit;
Bool    blockRandomised;
Bool    firstAttempt;
Int32   nBlocksRandomised;
Bool    inUse[256];
Int32   nInUse;

UChar   seqToUnseq[256];
UChar   unseqToSeq[256];

Int32   nMTF;

UChar   *selector;
UChar   *selectorMtf;

Int32   *mtfFreq;

UChar   **len;

Int32   **limit;
Int32   **base;
Int32   **perm;
Int32   *minLens;

Int32   **code;
Int32   **rfreq;

UInt32  bsBuff;
Int32   bsLive;
UChar*  bsStream;
UInt32  srcBufferPos;
UInt32  dstBufferPos;

UInt32  srcBufferMax;
UInt32  dstBufferMax;

typedef struct {
        Int32 ll;
        Int32 hh;
        Int32 dd;
} StackElem;

const Int32 incs[14] = {
    1, 4, 13, 40, 121, 364, 1093, 3280,
    9841, 29524, 88573, 265720,
    797161, 2391484
};

const Int32 rNums[512] = { 
    619, 720, 127, 481, 931, 816, 813, 233, 566, 247, 
    985, 724, 205, 454, 863, 491, 741, 242, 949, 214, 
    733, 859, 335, 708, 621, 574,  73, 654, 730, 472, 
    419, 436, 278, 496, 867, 210, 399, 680, 480,  51, 
    878, 465, 811, 169, 869, 675, 611, 697, 867, 561, 
    862, 687, 507, 283, 482, 129, 807, 591, 733, 623, 
    150, 238,  59, 379, 684, 877, 625, 169, 643, 105, 
    170, 607, 520, 932, 727, 476, 693, 425, 174, 647, 
     73, 122, 335, 530, 442, 853, 695, 249, 445, 515, 
    909, 545, 703, 919, 874, 474, 882, 500, 594, 612, 
    641, 801, 220, 162, 819, 984, 589, 513, 495, 799, 
    161, 604, 958, 533, 221, 400, 386, 867, 600, 782, 
    382, 596, 414, 171, 516, 375, 682, 485, 911, 276, 
     98, 553, 163, 354, 666, 933, 424, 341, 533, 870, 
    227, 730, 475, 186, 263, 647, 537, 686, 600, 224, 
    469,  68, 770, 919, 190, 373, 294, 822, 808, 206, 
    184, 943, 795, 384, 383, 461, 404, 758, 839, 887, 
    715,  67, 618, 276, 204, 918, 873, 777, 604, 560, 
    951, 160, 578, 722,  79, 804,  96, 409, 713, 940, 
    652, 934, 970, 447, 318, 353, 859, 672, 112, 785, 
    645, 863, 803, 350, 139,  93, 354,  99, 820, 908, 
    609, 772, 154, 274, 580, 184,  79, 626, 630, 742, 
    653, 282, 762, 623, 680,  81, 927, 626, 789, 125, 
    411, 521, 938, 300, 821,  78, 343, 175, 128, 250, 
    170, 774, 972, 275, 999, 639, 495,  78, 352, 126, 
    857, 956, 358, 619, 580, 124, 737, 594, 701, 612, 
    669, 112, 134, 694, 363, 992, 809, 743, 168, 974, 
    944, 375, 748,  52, 600, 747, 642, 182, 862,  81, 
    344, 805, 988, 739, 511, 655, 814, 334, 249, 515, 
    897, 955, 664, 981, 649, 113, 974, 459, 893, 228, 
    433, 837, 553, 268, 926, 240, 102, 654, 459,  51, 
    686, 754, 806, 760, 493, 403, 415, 394, 687, 700, 
    946, 670, 656, 610, 738, 392, 760, 799, 887, 653, 
    978, 321, 576, 617, 626, 502, 894, 679, 243, 440, 
    680, 879, 194, 572, 640, 724, 926,  56, 204, 700, 
    707, 151, 457, 449, 797, 195, 791, 558, 945, 679, 
    297,  59,  87, 824, 713, 663, 412, 693, 342, 606, 
    134, 108, 571, 364, 631, 212, 174, 643, 304, 329, 
    343,  97, 430, 751, 497, 314, 983, 374, 822, 928, 
    140, 206,  73, 263, 980, 736, 876, 478, 430, 305, 
    170, 514, 364, 692, 829,  82, 855, 953, 676, 246, 
    369, 970, 294, 750, 807, 827, 150, 790, 288, 923, 
    804, 378, 215, 828, 592, 281, 565, 555, 710,  82, 
    896, 831, 547, 261, 524, 462, 293, 465, 502,  56, 
    661, 821, 976, 991, 658, 869, 905, 758, 745, 193, 
    768, 550, 608, 933, 378, 286, 215, 979, 792, 961, 
     61, 688, 793, 644, 986, 403, 106, 366, 905, 644, 
    372, 567, 466, 434, 645, 210, 389, 550, 919, 135, 
    780, 773, 635, 389, 707, 100, 626, 958, 165, 504, 
    920, 176, 193, 713, 857, 265, 203,  50, 668, 108, 
    645, 990, 626, 197, 510, 357, 358, 850, 858, 364, 
    936, 638
};

const UInt32 crc32Table[256] = {
    0x00000000L, 0x04c11db7L, 0x09823b6eL, 0x0d4326d9L,
    0x130476dcL, 0x17c56b6bL, 0x1a864db2L, 0x1e475005L,
    0x2608edb8L, 0x22c9f00fL, 0x2f8ad6d6L, 0x2b4bcb61L,
    0x350c9b64L, 0x31cd86d3L, 0x3c8ea00aL, 0x384fbdbdL,
    0x4c11db70L, 0x48d0c6c7L, 0x4593e01eL, 0x4152fda9L,
    0x5f15adacL, 0x5bd4b01bL, 0x569796c2L, 0x52568b75L,
    0x6a1936c8L, 0x6ed82b7fL, 0x639b0da6L, 0x675a1011L,
    0x791d4014L, 0x7ddc5da3L, 0x709f7b7aL, 0x745e66cdL,
    0x9823b6e0L, 0x9ce2ab57L, 0x91a18d8eL, 0x95609039L,
    0x8b27c03cL, 0x8fe6dd8bL, 0x82a5fb52L, 0x8664e6e5L,
    0xbe2b5b58L, 0xbaea46efL, 0xb7a96036L, 0xb3687d81L,
    0xad2f2d84L, 0xa9ee3033L, 0xa4ad16eaL, 0xa06c0b5dL,
    0xd4326d90L, 0xd0f37027L, 0xddb056feL, 0xd9714b49L,
    0xc7361b4cL, 0xc3f706fbL, 0xceb42022L, 0xca753d95L,
    0xf23a8028L, 0xf6fb9d9fL, 0xfbb8bb46L, 0xff79a6f1L,
    0xe13ef6f4L, 0xe5ffeb43L, 0xe8bccd9aL, 0xec7dd02dL,
    0x34867077L, 0x30476dc0L, 0x3d044b19L, 0x39c556aeL,
    0x278206abL, 0x23431b1cL, 0x2e003dc5L, 0x2ac12072L,
    0x128e9dcfL, 0x164f8078L, 0x1b0ca6a1L, 0x1fcdbb16L,
    0x018aeb13L, 0x054bf6a4L, 0x0808d07dL, 0x0cc9cdcaL,
    0x7897ab07L, 0x7c56b6b0L, 0x71159069L, 0x75d48ddeL,
    0x6b93dddbL, 0x6f52c06cL, 0x6211e6b5L, 0x66d0fb02L,
    0x5e9f46bfL, 0x5a5e5b08L, 0x571d7dd1L, 0x53dc6066L,
    0x4d9b3063L, 0x495a2dd4L, 0x44190b0dL, 0x40d816baL,
    0xaca5c697L, 0xa864db20L, 0xa527fdf9L, 0xa1e6e04eL,
    0xbfa1b04bL, 0xbb60adfcL, 0xb6238b25L, 0xb2e29692L,
    0x8aad2b2fL, 0x8e6c3698L, 0x832f1041L, 0x87ee0df6L,
    0x99a95df3L, 0x9d684044L, 0x902b669dL, 0x94ea7b2aL,
    0xe0b41de7L, 0xe4750050L, 0xe9362689L, 0xedf73b3eL,
    0xf3b06b3bL, 0xf771768cL, 0xfa325055L, 0xfef34de2L,
    0xc6bcf05fL, 0xc27dede8L, 0xcf3ecb31L, 0xcbffd686L,
    0xd5b88683L, 0xd1799b34L, 0xdc3abdedL, 0xd8fba05aL,
    0x690ce0eeL, 0x6dcdfd59L, 0x608edb80L, 0x644fc637L,
    0x7a089632L, 0x7ec98b85L, 0x738aad5cL, 0x774bb0ebL,
    0x4f040d56L, 0x4bc510e1L, 0x46863638L, 0x42472b8fL,
    0x5c007b8aL, 0x58c1663dL, 0x558240e4L, 0x51435d53L,
    0x251d3b9eL, 0x21dc2629L, 0x2c9f00f0L, 0x285e1d47L,
    0x36194d42L, 0x32d850f5L, 0x3f9b762cL, 0x3b5a6b9bL,
    0x0315d626L, 0x07d4cb91L, 0x0a97ed48L, 0x0e56f0ffL,
    0x1011a0faL, 0x14d0bd4dL, 0x19939b94L, 0x1d528623L,
    0xf12f560eL, 0xf5ee4bb9L, 0xf8ad6d60L, 0xfc6c70d7L,
    0xe22b20d2L, 0xe6ea3d65L, 0xeba91bbcL, 0xef68060bL,
    0xd727bbb6L, 0xd3e6a601L, 0xdea580d8L, 0xda649d6fL,
    0xc423cd6aL, 0xc0e2d0ddL, 0xcda1f604L, 0xc960ebb3L,
    0xbd3e8d7eL, 0xb9ff90c9L, 0xb4bcb610L, 0xb07daba7L,
    0xae3afba2L, 0xaafbe615L, 0xa7b8c0ccL, 0xa379dd7bL,
    0x9b3660c6L, 0x9ff77d71L, 0x92b45ba8L, 0x9675461fL,
    0x8832161aL, 0x8cf30badL, 0x81b02d74L, 0x857130c3L,
    0x5d8a9099L, 0x594b8d2eL, 0x5408abf7L, 0x50c9b640L,
    0x4e8ee645L, 0x4a4ffbf2L, 0x470cdd2bL, 0x43cdc09cL,
    0x7b827d21L, 0x7f436096L, 0x7200464fL, 0x76c15bf8L,
    0x68860bfdL, 0x6c47164aL, 0x61043093L, 0x65c52d24L,
    0x119b4be9L, 0x155a565eL, 0x18197087L, 0x1cd86d30L,
    0x029f3d35L, 0x065e2082L, 0x0b1d065bL, 0x0fdc1becL,
    0x3793a651L, 0x3352bbe6L, 0x3e119d3fL, 0x3ad08088L,
    0x2497d08dL, 0x2056cd3aL, 0x2d15ebe3L, 0x29d4f654L,
    0xc5a92679L, 0xc1683bceL, 0xcc2b1d17L, 0xc8ea00a0L,
    0xd6ad50a5L, 0xd26c4d12L, 0xdf2f6bcbL, 0xdbee767cL,
    0xe3a1cbc1L, 0xe760d676L, 0xea23f0afL, 0xeee2ed18L,
    0xf0a5bd1dL, 0xf464a0aaL, 0xf9278673L, 0xfde69bc4L,
    0x89b8fd09L, 0x8d79e0beL, 0x803ac667L, 0x84fbdbd0L,
    0x9abc8bd5L, 0x9e7d9662L, 0x933eb0bbL, 0x97ffad0cL,
    0xafb010b1L, 0xab710d06L, 0xa6322bdfL, 0xa2f33668L,
    0xbcb4666dL, 0xb8757bdaL, 0xb5365d03L, 0xb1f740b4L
};

void initialiseCRC(void) {
    globalCrc = 0xffffffffL;
}

UInt32 getFinalCRC(void) {
    return ~globalCrc;
}

UInt32 getGlobalCRC(void) {
    return globalCrc;
}

void setGlobalCRC(UInt32 newCrc) {
    globalCrc = newCrc;
}

void bsSetStream(UChar* f, Bool wr) {
    bsStream = f;
    bsLive = 0;
    bsBuff = 0;
    bytesOut = 0;
}

void bsFinishedWithStream(void) {
    bsStream = NULL;
}

UInt32 bsR(Int32 n) {
    UInt32 v;
    bsNEEDR ( n );
    v = (bsBuff >> (bsLive-n)) & ((1 << n)-1);
    bsLive -= n;
    return v;
}

UChar bsGetUChar(void) {
    return (UChar)bsR(8);
}

UInt32 bsGetUInt32(void) {
    UInt32 u;
    u = 0;
    u = (u << 8) | bsR(8);
    u = (u << 8) | bsR(8);
    u = (u << 8) | bsR(8);
    u = (u << 8) | bsR(8);
    return u;
}

UInt32 bsGetIntVS(UInt32 numBits) {
    return (UInt32)bsR(numBits);
}

Int32 bsGetInt32(void) {
    return (Int32)bsGetUInt32();
}

void hbMakeCodeLengths(UChar *len, Int32 *freq, Int32 alphaSize, Int32 maxLen) {
    /*--
        Nodes and heap entries run from 1.  Entry 0
        for both the heap and nodes is a sentinel.
    --*/
    Int32 nNodes, nHeap, n1, n2, i, j, k;
    Bool  tooLong;

    Int32 heap   [ MAX_ALPHA_SIZE + 2 ];
    Int32 weight [ MAX_ALPHA_SIZE * 2 ];
    Int32 parent [ MAX_ALPHA_SIZE * 2 ]; 

    for(i=0; i<alphaSize; i++)
        weight[i+1] = (freq[i] == 0 ? 1 : freq[i]) << 8;

    while(True) {
        nNodes = alphaSize;
        nHeap = 0;

        heap[0] = 0;
        weight[0] = 0;
        parent[0] = -2;

        for (i = 1; i <= alphaSize; i++) {
            parent[i] = -1;
            nHeap++;
            heap[nHeap] = i;
            UPHEAP(nHeap);
        }
   
        while (nHeap > 1) {
            n1 = heap[1]; heap[1] = heap[nHeap]; nHeap--; DOWNHEAP(1);
            n2 = heap[1]; heap[1] = heap[nHeap]; nHeap--; DOWNHEAP(1);
            nNodes++;
            parent[n1] = parent[n2] = nNodes;
            weight[nNodes] = ADDWEIGHTS(weight[n1], weight[n2]);
            parent[nNodes] = -1;
            nHeap++;
            heap[nHeap] = nNodes;
            UPHEAP(nHeap);
        }

        tooLong = False;
        for (i = 1; i <= alphaSize; i++) {
            j = 0;
            k = i;
            while (parent[k] >= 0) { k = parent[k]; j++; }
            len[i-1] = j;
            if (j > maxLen) tooLong = True;
        }
      
        if (! tooLong) break;

        for (i = 1; i < alphaSize; i++) {
            j = weight[i] >> 8;
            j = 1 + (j / 2);
            weight[i] = j << 8;
        }
    }
}

void hbAssignCodes(Int32 *code, UChar *length, Int32 minLen, Int32 maxLen, Int32 alphaSize) {
    Int32 n, vec, i;

    vec = 0;
    for (n = minLen; n <= maxLen; n++) {
        for (i = 0; i < alphaSize; i++)
            if (length[i] == n) { code[i] = vec; vec++; };
        vec <<= 1;
    }
}

void hbCreateDecodeTables(Int32 *limit, Int32 *base, Int32 *perm, UChar *length, Int32 minLen, Int32 maxLen, Int32 alphaSize) {
    Int32 pp, i, j, vec;

    pp = 0;
    for (i = minLen; i <= maxLen; i++)
        for (j = 0; j < alphaSize; j++)
            if (length[j] == i) { perm[pp] = j; pp++; };

    for (i = 0; i < MAX_CODE_LEN; i++) base[i] = 0;
    for (i = 0; i < alphaSize; i++) base[length[i]+1]++;

    for (i = 1; i < MAX_CODE_LEN; i++) base[i] += base[i-1];

    for (i = 0; i < MAX_CODE_LEN; i++) limit[i] = 0;
    vec = 0;

    for (i = minLen; i <= maxLen; i++) {
        vec += (base[i+1] - base[i]);
        limit[i] = vec-1;
        vec <<= 1;
    }
    for (i = minLen + 1; i <= maxLen; i++)
        base[i] = ((limit[i-1] + 1) << 1) - base[i];
}

void setDecompressStructureSizes(Int32 newSize100k) {
    if (newSize100k == blockSize100k) return;

    blockSize100k = newSize100k;

    if (ll16  != NULL) hfree ( ll16  );
    if (ll4   != NULL) hfree ( ll4   );
    if (ll8   != NULL) hfree ( ll8   );
    if (tt    != NULL) hfree ( tt    );

    if (newSize100k == 0) return;

    if (smallMode) {
        Int32 n = 100000 * newSize100k;
        ll16    = halloc ( n, sizeof(UInt16) );
        ll4     = halloc ( ((n+1) >> 1), sizeof(UChar) );
    }
    else {
        Int32 n = 100000 * newSize100k;
        ll8     = halloc ( n, sizeof(UChar) );
        tt      = halloc ( n, sizeof(Int32) );
    }
}

void makeMaps(void) {
    Int32 i;
    nInUse = 0;
    for (i = 0; i < 256; i++)
        if (inUse[i]) {
            seqToUnseq[nInUse] = i;
            unseqToSeq[i] = nInUse;
            nInUse++;
        }
}

void recvDecodingTables(void) {
    Int32 i, j, t, nGroups, nSelectors, alphaSize;
    Int32 minLen, maxLen;
    Bool inUse16[16];

    /*--- Receive the mapping table ---*/
    for (i = 0; i < 16; i++)
        if (bsR(1) == 1) 
            inUse16[i] = True; else 
            inUse16[i] = False;

    for (i = 0; i < 256; i++) inUse[i] = False;

    for (i = 0; i < 16; i++)
        if (inUse16[i])
            for (j = 0; j < 16; j++)
                if (bsR(1) == 1) inUse[i * 16 + j] = True;

    makeMaps();
    alphaSize = nInUse+2;

    /*--- Now the selectors ---*/
    nGroups = bsR ( 3 );
    nSelectors = bsR ( 15 );
    for (i = 0; i < nSelectors; i++) {
        j = 0;
        while (bsR(1) == 1) j++;
        selectorMtf[i] = j;
    }

    /*--- Undo the MTF values for the selectors. ---*/
    {
        UChar pos[N_GROUPS], tmp, v;
        for (v = 0; v < nGroups; v++) pos[v] = v;
   
        for (i = 0; i < nSelectors; i++) {
            v = selectorMtf[i];
            tmp = pos[v];
            while (v > 0) { pos[v] = pos[v-1]; v--; }
            pos[0] = tmp;
            selector[i] = tmp;
        }
    }

    /*--- Now the coding tables ---*/
    for (t = 0; t < nGroups; t++) {
        Int32 curr = bsR ( 5 );
        for (i = 0; i < alphaSize; i++) {
            while (bsR(1) == 1) {
                if (bsR(1) == 0) curr++; else curr--;
            }
            len[t][i] = curr;
        }
    }

    /*--- Create the Huffman decoding tables ---*/
    for (t = 0; t < nGroups; t++) {
        minLen = 32;
        maxLen = 0;
        for (i = 0; i < alphaSize; i++) {
            if (len[t][i] > maxLen) maxLen = len[t][i];
            if (len[t][i] < minLen) minLen = len[t][i];
        }
        hbCreateDecodeTables ( 
            &limit[t][0], &base[t][0], &perm[t][0], &len[t][0],
            minLen, maxLen, alphaSize
        );
        minLens[t] = minLen;
    }
}

void getAndMoveToFrontDecode(void) {
    UChar  yy[256];
    Int32  i, j, nextSym, limitLast;
    Int32  EOB, groupNo, groupPos;

    limitLast = 100000 * blockSize100k;
    origPtr   = bsGetIntVS ( 24 );

    recvDecodingTables();
    EOB      = nInUse+1;
    groupNo  = -1;
    groupPos = 0;

    /*--
        Setting up the unzftab entries here is not strictly
        necessary, but it does save having to do it later
        in a separate pass, and so saves a block's worth of
        cache misses.
    --*/
    for (i = 0; i <= 255; i++) unzftab[i] = 0;

    for (i = 0; i <= 255; i++) yy[i] = (UChar) i;

    last = -1;

    GET_MTF_VAL(nextSym);

    while(True) {
        if (nextSym == EOB) break;

        if (nextSym == RUNA || nextSym == RUNB) {
            UChar ch;
            Int32 s = -1;
            Int32 N = 1;
            do {
                if (nextSym == RUNA) s = s + (0+1) * N; else
                if (nextSym == RUNB) s = s + (1+1) * N;
                N = N * 2;
                GET_MTF_VAL(nextSym);
            }
            while (nextSym == RUNA || nextSym == RUNB);

            s++;
            ch = seqToUnseq[yy[0]];
            unzftab[ch] += s;

            if (smallMode)
                while (s > 0) {
                    last++; 
                    ll16[last] = ch;
                    s--;
                }
            else
                while (s > 0) {
                    last++;
                    ll8[last] = ch;
                    s--;
                };

            // if (last >= limitLast) blockOverrun();
            continue;
        }
        else {
            UChar tmp;
            last++;// if (last >= limitLast) blockOverrun();

            tmp = yy[nextSym-1];
            unzftab[seqToUnseq[tmp]]++;
            if (smallMode)
                ll16[last] = seqToUnseq[tmp]; else
                ll8[last]  = seqToUnseq[tmp];

            /*--
                This loop is hammered during decompression,
                hence the unrolling.

                for (j = nextSym-1; j > 0; j--) yy[j] = yy[j-1];
            --*/

            j = nextSym-1;
            for (; j > 3; j -= 4) {
                yy[j]   = yy[j-1];
                yy[j-1] = yy[j-2];
                yy[j-2] = yy[j-3];
                yy[j-3] = yy[j-4];
            }
            for (; j > 0; j--) yy[j] = yy[j-1];

            yy[0] = tmp;
            GET_MTF_VAL(nextSym);
            continue;
        }
    }
}

Bool fullGtU(Int32 i1, Int32 i2) {
    Int32 k;
    UChar c1, c2;
    UInt16 s1, s2;

    #if DEBUG
        /*--
            shellsort shouldn't ask to compare
            something with itself.
        --*/
        assert (i1 != i2);
    #endif

    c1 = block[i1];
    c2 = block[i2];
    if (c1 != c2) return (c1 > c2);
    i1++; i2++;

    c1 = block[i1];
    c2 = block[i2];
    if (c1 != c2) return (c1 > c2);
    i1++; i2++;

    c1 = block[i1];
    c2 = block[i2];
    if (c1 != c2) return (c1 > c2);
    i1++; i2++;

    c1 = block[i1];
    c2 = block[i2];
    if (c1 != c2) return (c1 > c2);
    i1++; i2++;

    c1 = block[i1];
    c2 = block[i2];
    if (c1 != c2) return (c1 > c2);
    i1++; i2++;

    c1 = block[i1];
    c2 = block[i2];
    if (c1 != c2) return (c1 > c2);
    i1++; i2++;

    k = last + 1;

    do {
        c1 = block[i1];
        c2 = block[i2];
        if (c1 != c2) return (c1 > c2);
        s1 = quadrant[i1];
        s2 = quadrant[i2];
        if (s1 != s2) return (s1 > s2);
        i1++; i2++;

        c1 = block[i1];
        c2 = block[i2];
        if (c1 != c2) return (c1 > c2);
        s1 = quadrant[i1];
        s2 = quadrant[i2];
        if (s1 != s2) return (s1 > s2);
        i1++; i2++;

        c1 = block[i1];
        c2 = block[i2];
        if (c1 != c2) return (c1 > c2);
        s1 = quadrant[i1];
        s2 = quadrant[i2];
        if (s1 != s2) return (s1 > s2);
        i1++; i2++;

        c1 = block[i1];
        c2 = block[i2];
        if (c1 != c2) return (c1 > c2);
        s1 = quadrant[i1];
        s2 = quadrant[i2];
        if (s1 != s2) return (s1 > s2);
        i1++; i2++;

        if (i1 > last) { i1 -= last; i1--; };
        if (i2 > last) { i2 -= last; i2--; };

        k -= 4;
        workDone++;
    }
    while (k >= 0);

    return False;
}

void simpleSort(Int32 lo, Int32 hi, Int32 d) {
    Int32 i, j, h, bigN, hp;
    Int32 v;

    bigN = hi - lo + 1;
    if (bigN < 2) return;

    hp = 0;
    while (incs[hp] < bigN) hp++;
    hp--;

    for (; hp >= 0; hp--) {
        h = incs[hp];

        i = lo + h;
        while(True) {
            /*-- copy 1 --*/
            if (i > hi) break;
            v = zptr[i];
            j = i;
            while ( fullGtU ( zptr[j-h]+d, v+d ) ) {
                zptr[j] = zptr[j-h];
                j = j - h;
                if (j <= (lo + h - 1)) break;
            }
            zptr[j] = v;
            i++;

            /*-- copy 2 --*/
            if (i > hi) break;
            v = zptr[i];
            j = i;
            while ( fullGtU ( zptr[j-h]+d, v+d ) ) {
                zptr[j] = zptr[j-h];
                j = j - h;
                if (j <= (lo + h - 1)) break;
            }
            zptr[j] = v;
            i++;

            /*-- copy 3 --*/
            if (i > hi) break;
            v = zptr[i];
            j = i;
            while ( fullGtU ( zptr[j-h]+d, v+d ) ) {
                zptr[j] = zptr[j-h];
                j = j - h;
                if (j <= (lo + h - 1)) break;
            }
            zptr[j] = v;
            i++;

            if (workDone > workLimit && firstAttempt) return;
        }
    }
}

void vswap(Int32 p1, Int32 p2, Int32 n) {
    while (n > 0) {
        swap(zptr[p1], zptr[p2]);
        p1++; p2++; n--;
    }
}

UChar med3(UChar a, UChar b, UChar c) {
    UChar t;
    if (a > b) { t = a; a = b; b = t; };
    if (b > c) { t = b; b = c; c = t; };
    if (a > b)          b = a;
    return b;
}

void qSort3(Int32 loSt, Int32 hiSt, Int32 dSt) {
    Int32 unLo, unHi, ltLo, gtHi, med, n, m;
    Int32 sp, lo, hi, d;
    StackElem stack[QSORT_STACK_SIZE];

    sp = 0;
    push ( loSt, hiSt, dSt );

    while(sp > 0) {
        pop ( lo, hi, d );

        if (hi - lo < SMALL_THRESH || d > DEPTH_THRESH) {
            simpleSort ( lo, hi, d );
            if (workDone > workLimit && firstAttempt) return;
            continue;
        }

        med = med3( block[zptr[ lo         ]+d],
                    block[zptr[ hi         ]+d],
                    block[zptr[ (lo+hi)>>1 ]+d] );

        unLo = ltLo = lo;
        unHi = gtHi = hi;

        while(True) {
            while(True) {
                if (unLo > unHi) break;
                n = ((Int32)block[zptr[unLo]+d]) - med;
                if (n == 0) { swap(zptr[unLo], zptr[ltLo]); ltLo++; unLo++; continue; };
                if (n >  0) break;
                unLo++;
            }
            while(True) {
                if (unLo > unHi) break;
                n = ((Int32)block[zptr[unHi]+d]) - med;
                if (n == 0) { swap(zptr[unHi], zptr[gtHi]); gtHi--; unHi--; continue; };
                if (n <  0) break;
                unHi--;
            }
            if (unLo > unHi) break;
            swap(zptr[unLo], zptr[unHi]); unLo++; unHi--;
        }
        #if DEBUG
            assert (unHi == unLo-1);
        #endif

        if (gtHi < ltLo) {
            push(lo, hi, d+1 );
            continue;
        }

        n = min(ltLo-lo, unLo-ltLo); vswap(lo, unLo-n, n);
        m = min(hi-gtHi, gtHi-unHi); vswap(unLo, hi-m+1, m);

        n = lo + unLo - ltLo - 1;
        m = hi - (gtHi - unHi) + 1;

        push ( lo, n, d );
        push ( n+1, m-1, d+1 );
        push ( m, hi, d );
    }
}

void sortIt(void) {
    Int32 i, j, ss, sb;
    Int32 runningOrder[256];
    Int32 copy[256];
    Bool bigDone[256];
    UChar c1, c2;
    Int32 numQSorted;

    /*--
        In the various block-sized structures, live data runs
        from 0 to last+NUM_OVERSHOOT_BYTES inclusive.  First,
        set up the overshoot area for block.
    --*/

    for (i = 0; i < NUM_OVERSHOOT_BYTES; i++)
        block[last+i+1] = block[i % (last+1)];
    for (i = 0; i <= last+NUM_OVERSHOOT_BYTES; i++)
        quadrant[i] = 0;

    block[-1] = block[last];

    if(last < 4000) {
        /*--
            Use simpleSort(), since the full sorting mechanism
            has quite a large constant overhead.
        --*/
        for (i = 0; i <= last; i++) zptr[i] = i;
        firstAttempt = False;
        workDone = workLimit = 0;
        simpleSort ( 0, last, 0 );
    }
    else {
        numQSorted = 0;
        for (i = 0; i <= 255; i++) bigDone[i] = False;

        for (i = 0; i <= 65536; i++) ftab[i] = 0;

        c1 = block[-1];
        for (i = 0; i <= last; i++) {
            c2 = block[i];
            ftab[(c1 << 8) + c2]++;
            c1 = c2;
        }

        for (i = 1; i <= 65536; i++) ftab[i] += ftab[i-1];

        c1 = block[0];
        for (i = 0; i < last; i++) {
            c2 = block[i+1];
            j = (c1 << 8) + c2;
            c1 = c2;
            ftab[j]--;
            zptr[ftab[j]] = i;
        }
        j = (block[last] << 8) + block[0];
        ftab[j]--;
        zptr[ftab[j]] = last;

        /*--
            Now ftab contains the first loc of every small bucket.
            Calculate the running order, from smallest to largest
            big bucket.
        --*/

        for (i = 0; i <= 255; i++) runningOrder[i] = i;

        {
            Int32 vv;
            Int32 h = 1;
            do h = 3 * h + 1; while (h <= 256);
            do {
                h = h / 3;
                for (i = h; i <= 255; i++) {
                    vv = runningOrder[i];
                    j = i;
                    while ( BIGFREQ(runningOrder[j-h]) > BIGFREQ(vv) ) {
                        runningOrder[j] = runningOrder[j-h];
                        j = j - h;
                        if (j <= (h - 1)) goto zero;
                    }
                    zero:
                    runningOrder[j] = vv;
                }
            } while (h != 1);
        }

        /*--
            The main sorting loop.
        --*/

        for (i = 0; i <= 255; i++) {
            /*--
                Process big buckets, starting with the least full.
            --*/
            ss = runningOrder[i];

            /*--
                Complete the big bucket [ss] by quicksorting
                any unsorted small buckets [ss, j].  Hopefully
                previous pointer-scanning phases have already
                completed many of the small buckets [ss, j], so
                we don't have to sort them at all.
            --*/
            for (j = 0; j <= 255; j++) {
                sb = (ss << 8) + j;
                if ( ! (ftab[sb] & SETMASK) ) {
                    Int32 lo = ftab[sb]   & CLEARMASK;
                    Int32 hi = (ftab[sb+1] & CLEARMASK) - 1;
                    if (hi > lo) {
                        qSort3 ( lo, hi, 2 );
                        numQSorted += ( hi - lo + 1 );
                        if (workDone > workLimit && firstAttempt) return;
                    }
                    ftab[sb] |= SETMASK;
                }
            }

            /*--
                The ss big bucket is now done.  Record this fact,
                and update the quadrant descriptors.  Remember to
                update quadrants in the overshoot area too, if
                necessary.  The "if (i < 255)" test merely skips
                this updating for the last bucket processed, since
                updating for the last bucket is pointless.
            --*/
            bigDone[ss] = True;

            if (i < 255) {
                Int32 bbStart  = ftab[ss << 8] & CLEARMASK;
                Int32 bbSize   = (ftab[(ss+1) << 8] & CLEARMASK) - bbStart;
                Int32 shifts   = 0;

                while ((bbSize >> shifts) > 65534) shifts++;

                for (j = 0; j < bbSize; j++) {
                    Int32 a2update     = zptr[bbStart + j];
                    UInt16 qVal        = (UInt16)(j >> shifts);
                    quadrant[a2update] = qVal;
                    if (a2update < NUM_OVERSHOOT_BYTES)
                        quadrant[a2update + last + 1] = qVal;
                }

            }

            /*--
                Now scan this big bucket so as to synthesise the
                sorted order for small buckets [t, ss] for all t != ss.
            --*/
            for (j = 0; j <= 255; j++)
                copy[j] = ftab[(j << 8) + ss] & CLEARMASK;

            for (j = ftab[ss << 8] & CLEARMASK;
                j < (ftab[(ss+1) << 8] & CLEARMASK);
                j++) {
                c1 = block[zptr[j]-1];
                if ( ! bigDone[c1] ) {
                    zptr[copy[c1]] = zptr[j] == 0 ? last : zptr[j] - 1;
                    copy[c1] ++;
                }
            }

            for (j = 0; j <= 255; j++) ftab[(j << 8) + ss] |= SETMASK;
        }
    }
}

Int32 indexIntoF(Int32 indx, Int32 *cftab) {
    Int32 nb, na, mid;
    nb = 0;
    na = 256;
    do {
        mid = (nb + na) >> 1;
        if (indx >= cftab[mid]) nb = mid; else na = mid;
    }
    while (na - nb != 1);
    return nb;
}

#define GET_SMALL(cccc)                     \
                                            \
    cccc = indexIntoF ( tPos, cftab );      \
    tPos = GET_LL(tPos);

void undoReversibleTransformation_small(UChar* dstBuffer) {
    Int32  cftab[257], cftabAlso[257];
    Int32  i, j, tmp, tPos;
    UChar  ch;

    /*--
        We assume here that the global array unzftab will
        already be holding the frequency counts for
        ll8[0 .. last].
    --*/

    /*-- Set up cftab to facilitate generation of indexIntoF --*/
    cftab[0] = 0;
    for (i = 1; i <= 256; i++) cftab[i] = unzftab[i-1];
    for (i = 1; i <= 256; i++) cftab[i] += cftab[i-1];

    /*-- Make a copy of it, used in generation of T --*/
    for (i = 0; i <= 256; i++) cftabAlso[i] = cftab[i];

    /*-- compute the T vector --*/
    for (i = 0; i <= last; i++) {
        ch = (UChar)ll16[i];
        SET_LL(i, cftabAlso[ch]);
        cftabAlso[ch]++;
    }

    /*--
        Compute T^(-1) by pointer reversal on T.  This is rather
        subtle, in that, if the original block was two or more
        (in general, N) concatenated copies of the same thing,
        the T vector will consist of N cycles, each of length
        blocksize / N, and decoding will involve traversing one
        of these cycles N times.  Which particular cycle doesn't
        matter -- they are all equivalent.  The tricky part is to
        make sure that the pointer reversal creates a correct
        reversed cycle for us to traverse.  So, the code below
        simply reverses whatever cycle origPtr happens to fall into,
        without regard to the cycle length.  That gives one reversed
        cycle, which for normal blocks, is the entire block-size long.
        For repeated blocks, it will be interspersed with the other
        N-1 non-reversed cycles.  Providing that the F-subscripting
        phase which follows starts at origPtr, all then works ok.
    --*/
    i = origPtr;
    j = GET_LL(i);
    do {
        tmp = GET_LL(j);
        SET_LL(j, i);
        i = j;
        j = tmp;
    }
    while (i != origPtr);

    /*--
        We recreate the original by subscripting F through T^(-1).
        The run-length-decoder below requires characters incrementally,
        so tPos is set to a starting value, and is updated by
        the GET_SMALL macro.
    --*/
    tPos = origPtr;

    /*-------------------------------------------------*/
    /*--
        This is pretty much a verbatim copy of the
        run-length decoder present in the distribution
        bzip-0.21; it has to be here to avoid creating
        block[] as an intermediary structure.  As in 0.21,
        this code derives from some sent to me by
        Christian von Roques.

        It allows dst==NULL, so as to support the test (-t)
        option without slowing down the fast decompression
        code.
    --*/
    {
        Int32     i2, count, chPrev, ch2;
        UInt32    localCrc;

        count    = 0;
        i2       = 0;
        ch2      = 256;   /*-- not a char and not EOF --*/
        localCrc = getGlobalCRC();

        {
            RAND_DECLS;
            while ( i2 <= last ) {
                chPrev = ch2;
                GET_SMALL(ch2);
                if (blockRandomised) {
                    RAND_UPD_MASK;
                    ch2 ^= (UInt32)RAND_MASK;
                }
                i2++;
   
                dstBuffer[dstBufferPos++]=ch2;
                if(dstBufferPos>=dstBufferMax) {
                    dstBufferPos=0;
                }
                UPDATE_CRC ( localCrc, (UChar)ch2 );
   
                if (ch2 != chPrev) {
                    count = 1;
                }
                else {
                    count++;
                    if (count >= 4) {
                        Int32 j2;
                        UChar z;
                        GET_SMALL(z);
                        if (blockRandomised) {
                            RAND_UPD_MASK;
                            z ^= RAND_MASK;
                        }
                        for (j2 = 0;  j2 < (Int32)z;  j2++) {
                            dstBuffer[dstBufferPos++]=ch2;
                            if(dstBufferPos>=dstBufferMax) {
                                dstBufferPos=0;
                            }
                            UPDATE_CRC ( localCrc, (UChar)ch2 );
                        }
                        i2++;
                        count = 0;
                    }
                }
            }
        }

        setGlobalCRC ( localCrc );
    }
    /*-- end of the in-line run-length-decoder. --*/
}
#undef GET_SMALL

#define GET_FAST(cccc)                          \
                                                \
        cccc = ll8[tPos];                       \
        tPos = tt[tPos];

void undoReversibleTransformation_fast(UChar* dstBuffer) {
    Int32  cftab[257];
    Int32  i, tPos;
    UChar  ch;

    /*--
        We assume here that the global array unzftab will
        already be holding the frequency counts for
        ll8[0 .. last].
    --*/

    /*-- Set up cftab to facilitate generation of T^(-1) --*/
    cftab[0] = 0;
    for (i = 1; i <= 256; i++) cftab[i] = unzftab[i-1];
    for (i = 1; i <= 256; i++) cftab[i] += cftab[i-1];

    /*-- compute the T^(-1) vector --*/
    for (i = 0; i <= last; i++) {
        ch = (UChar)ll8[i];
        tt[cftab[ch]] = i;
        cftab[ch]++;
    }

    /*--
        We recreate the original by subscripting L through T^(-1).
        The run-length-decoder below requires characters incrementally,
        so tPos is set to a starting value, and is updated by
        the GET_FAST macro.
    --*/
    tPos = tt[origPtr];

    /*-------------------------------------------------*/
    /*--
        This is pretty much a verbatim copy of the
        run-length decoder present in the distribution
        bzip-0.21; it has to be here to avoid creating
        block[] as an intermediary structure.  As in 0.21,
        this code derives from some sent to me by
        Christian von Roques.
    --*/
    {
        Int32     i2, count, chPrev, ch2;
        UInt32    localCrc;

        count    = 0;
        i2       = 0;
        ch2      = 256;   /*-- not a char and not EOF --*/
        localCrc = getGlobalCRC();

        if (blockRandomised) {
            RAND_DECLS;
            while ( i2 <= last ) {
                chPrev = ch2;
                GET_FAST(ch2);
                RAND_UPD_MASK;
                ch2 ^= (UInt32)RAND_MASK;
                i2++;

                dstBuffer[dstBufferPos++]=ch2;
                if(dstBufferPos>=dstBufferMax) {
                    dstBufferPos=0;
                }
                UPDATE_CRC ( localCrc, (UChar)ch2 );
   
                if (ch2 != chPrev) {
                    count = 1;
                }
                else {
                    count++;
                    if (count >= 4) {
                        Int32 j2;
                        UChar z;
                        GET_FAST(z);
                        RAND_UPD_MASK;
                        z ^= RAND_MASK;
                        for (j2 = 0;  j2 < (Int32)z;  j2++) {
                            dstBuffer[dstBufferPos++]=ch2;
                            if(dstBufferPos>=dstBufferMax) {
                                dstBufferPos=0;
                            }
                            UPDATE_CRC ( localCrc, (UChar)ch2 );
                        }
                        i2++;
                        count = 0;
                    }
                }
            }
        }
        else {
            while( i2 <= last ) {
                chPrev = ch2;
                GET_FAST(ch2);
                i2++;

                dstBuffer[dstBufferPos++]=ch2;
                if(dstBufferPos>=dstBufferMax) {
                    dstBufferPos=0;
                }
                UPDATE_CRC ( localCrc, (UChar)ch2 );
   
                if (ch2 != chPrev) {
                    count = 1;
                }
                else {
                    count++;
                    if (count >= 4) {
                        Int32 j2;
                        UChar z;
                        GET_FAST(z);
                        for (j2 = 0;  j2 < (Int32)z;  j2++) {
                            dstBuffer[dstBufferPos++]=ch2;
                            if(dstBufferPos>=dstBufferMax) {
                                dstBufferPos=0;
                            }
                            UPDATE_CRC ( localCrc, (UChar)ch2 );
                        }
                        i2++;
                        count = 0;
                    }
                }
            }
        }   /*-- if (blockRandomised) --*/

        setGlobalCRC ( localCrc );
    }
    /*-- end of the in-line run-length-decoder. --*/
}
#undef GET_FAST

Bool uncompressStream(UChar *zStream, UChar *stream) {
    UChar       magic1, magic2, magic3, magic4;
    UChar       magic5, magic6;
    UInt32      storedBlockCRC, storedCombinedCRC;
    UInt32      computedBlockCRC, computedCombinedCRC;
    Int32       currBlockNo;
    IntNative   retVal;
    srcBufferPos=0;
    bsSetStream ( zStream, False );
    /*--
        A bad magic number is `recoverable from';
        return with False so the caller skips the file.
    --*/
    magic1 = bsGetUChar ();
    magic2 = bsGetUChar ();
    magic3 = bsGetUChar ();
    magic4 = bsGetUChar ();
    if (magic1 != 'B' ||
        magic2 != 'Z' ||
        magic3 != 'h' ||
        magic4 < '1'  ||
        magic4 > '9') {
        return False;
    }

    setDecompressStructureSizes ( magic4 - '0' );
    computedCombinedCRC = 0;

    currBlockNo = 0;

    while(True) {
        magic1 = bsGetUChar ();
        magic2 = bsGetUChar ();
        magic3 = bsGetUChar ();
        magic4 = bsGetUChar ();
        magic5 = bsGetUChar ();
        magic6 = bsGetUChar ();
        if (magic1 == 0x17 && magic2 == 0x72 &&
            magic3 == 0x45 && magic4 == 0x38 &&
            magic5 == 0x50 && magic6 == 0x90) break;

        //if (magic1 != 0x31 || magic2 != 0x41 ||
        //    magic3 != 0x59 || magic4 != 0x26 ||
        //    magic5 != 0x53 || magic6 != 0x59) badBlockHeader();

        storedBlockCRC = bsGetUInt32 ();

        if (bsR(1) == 1)
            blockRandomised = True; else
            blockRandomised = False;

        currBlockNo++;
        getAndMoveToFrontDecode ();
        initialiseCRC();
        if (smallMode)
            undoReversibleTransformation_small ( stream );
            else
            undoReversibleTransformation_fast  ( stream );
        computedBlockCRC = getFinalCRC();
        computedCombinedCRC = (computedCombinedCRC << 1) | (computedCombinedCRC >> 31);
        computedCombinedCRC ^= computedBlockCRC;
    };
    
    storedCombinedCRC  = bsGetUInt32();
    return True;
}

// Okay, it's time for my code ;-)
Bool initialize(void) {
    UInt32 temp;
    
    ftab          = NULL;
    ll4           = NULL;
    ll16          = NULL;
    ll8           = NULL;
    tt            = NULL;
    block         = NULL;
    zptr          = NULL;
    smallMode     = False;
    blockSize100k = 0;
    bsStream      = NULL;
    workFactor    = 30;

    srcBufferPos  = 0;
    dstBufferPos  = 0;

    srcBufferMax  = 0;
    dstBufferMax  = 0;

    selector = (UChar*) halloc(MAX_SELECTORS, sizeof(UChar));
    selectorMtf = (UChar*) halloc(MAX_SELECTORS, sizeof(UChar));
    mtfFreq = (Int32*) halloc(MAX_ALPHA_SIZE, sizeof(Int32));
    minLens = (Int32*) halloc(N_GROUPS, sizeof(Int32));

    if((selector==NULL) || (selectorMtf==NULL) || (mtfFreq==NULL) || (minLens==NULL)) {
        return False;
    }

    len = (UChar**) halloc(N_GROUPS, sizeof(UChar*));
    limit = (Int32**) halloc(N_GROUPS, sizeof(Int32*));
    base = (Int32**) halloc(N_GROUPS, sizeof(Int32*));
    perm = (Int32**) halloc(N_GROUPS, sizeof(Int32*));
    code = (Int32**) halloc(N_GROUPS, sizeof(Int32*));
    rfreq = (Int32**) halloc(N_GROUPS, sizeof(Int32*));

    if((len==NULL) || (limit==NULL) || (base==NULL) || (perm==NULL) || (code==NULL) || (rfreq==NULL)) {
        return False;
    }
    
    for(temp=0; temp<N_GROUPS; ++temp) {
        len[temp]=(UChar*) halloc(MAX_ALPHA_SIZE, sizeof(UChar));
        limit[temp]=(Int32*) halloc(MAX_ALPHA_SIZE, sizeof(Int32));
        base[temp]=(Int32*) halloc(MAX_ALPHA_SIZE, sizeof(Int32));
        perm[temp]=(Int32*) halloc(MAX_ALPHA_SIZE, sizeof(Int32));
        code[temp]=(Int32*) halloc(MAX_ALPHA_SIZE, sizeof(Int32));
        rfreq[temp]=(Int32*) halloc(MAX_ALPHA_SIZE, sizeof(Int32));
        if((len[temp]==NULL) || (limit[temp]==NULL) || (base[temp]==NULL) || (perm[temp]==NULL) || (code[temp]==NULL) || (rfreq[temp]==NULL)) {
            return False;
        }
    }
    
    return True;
}

void freeAll(void) {
    UInt32 temp;
    
    for(temp=0; temp<N_GROUPS; ++temp) {
        if(len[temp]!=NULL) {
            hfree(len[temp]);
        }
        if(limit[temp]!=NULL) {
            hfree(limit[temp]);
        }
        if(base[temp]!=NULL) {
            hfree(base[temp]);
        }
        if(perm[temp]!=NULL) {
            hfree(perm[temp]);
        }
        if(code[temp]!=NULL) {
            hfree(code[temp]);
        }
        if(rfreq[temp]!=NULL) {
            hfree(rfreq[temp]);
        }
    }
    
    if(len!=NULL) {
        hfree(len);
    }
    if(limit!=NULL) {
        hfree(limit);
    }
    if(base!=NULL) {
        hfree(base);
    }
    if(perm!=NULL) {
        hfree(perm);
    }
    if(code!=NULL) {
        hfree(code);
    }
    if(rfreq!=NULL) {
        hfree(rfreq);
    }

    if(selector!=NULL) {
        hfree(selector);
    }
    if(selectorMtf!=NULL) {
        hfree(selectorMtf);
    }
    if(mtfFreq!=NULL) {
        hfree(mtfFreq);
    }
    if(minLens!=NULL) {
        hfree(minLens);
    }
}

void setSourceBufferSize(UInt32 size) {
    srcBufferMax=size;
    return;
}

void setDestinationBufferSize(UInt32 size) {
    dstBufferMax=size;
    return;
}

UInt32 uncompressData(UChar *input, UInt32 inputSize, UChar *output, UInt32 outputSize) {
    if(initialize()) {
        srcBufferMax=inputSize;
        dstBufferMax=outputSize;
        uncompressStream(input,output);
    }
    freeAll();
    return dstBufferPos;
}

UInt32 uncompressDataInPlace(UChar *buffer, UInt32 inputSize, UInt32 outputSize) {
    UChar *tempBuffer=halloc(inputSize, sizeof(UChar));
    UInt32 temp;
    if(tempBuffer==NULL) {
        return 0;
    }
    else {
        for(temp=0; temp<inputSize; ++temp) {
            tempBuffer[temp]=buffer[temp];
        }
        temp=uncompressData(tempBuffer,inputSize,buffer,outputSize);
        hfree(tempBuffer);
        return temp;
    }
}

/* =========FOR INTERNAL USE ONLY. NO DISTRIBUTION PLEASE ========== */

/*********************************************************************
 Copyright 1999-2003, University of Cincinnati.  All rights reserved.
 By using this software the USER indicates that he or she has read,
 understood and will comply with the following:

 --- University of Cincinnati hereby grants USER nonexclusive permission
 to use, copy and/or modify this software for internal, noncommercial,
 research purposes only. Any distribution, including commercial sale
 or license, of this software, copies of the software, its associated
 documentation and/or modifications of either is strictly prohibited
 without the prior consent of University of Cincinnati.  Title to copyright
 to this software and its associated documentation shall at all times
 remain with University of Cincinnati.  Appropriate copyright notice shall
 be placed on all software copies, and a complete copy of this notice
 shall be included in all copies of the associated documentation.
 No right is  granted to use in advertising, publicity or otherwise
 any trademark,  service mark, or the name of University of Cincinnati.


 --- This software and any associated documentation is provided "as is"

 UNIVERSITY OF CINCINNATI MAKES NO REPRESENTATIONS OR WARRANTIES, EXPRESS
 OR IMPLIED, INCLUDING THOSE OF MERCHANTABILITY OR FITNESS FOR A
 PARTICULAR PURPOSE, OR THAT  USE OF THE SOFTWARE, MODIFICATIONS, OR
 ASSOCIATED DOCUMENTATION WILL NOT INFRINGE ANY PATENTS, COPYRIGHTS,
 TRADEMARKS OR OTHER INTELLECTUAL PROPERTY RIGHTS OF A THIRD PARTY.

 University of Cincinnati shall not be liable under any circumstances for
 any direct, indirect, special, incidental, or consequential damages
 with respect to any claim by USER or any third party on account of
 or arising from the use, or inability to use, this software or its
 associated documentation, even if University of Cincinnati has been advised
 of the possibility of those damages.
*********************************************************************/

// lemmainfo.h
// Started 8/16/01 - J. Ward

// This file and lemmainfo.cc have code used for putting lemma
// info into the Smurf data structures.
// Currently lemmas are stored as an array of signed integers.
// These are to be interpreted as a disjunction of literals.
// In each case, the lemma is a prime implicant of the constraint
// represented by the Smurf.

#ifndef LEMMAINFO_H
#define LEMMAINFO_H

#include "smurffactory.h"

#define LEMMA_SPACE_SIZE 50000  // initial size -- expandable in these inc.
// # of lemma blocks allocated to store all lemmas. This includes prime 
// implicants identified prior to calling the brancher, and also those 
// generated by the brancher.  Each lemma with n literals requires n + 2 
// ints to store in the current format.

// #define MAX_NUM_PRIME_IMPLICANTS 6561 // Max # of prime implicants we
#define MAX_NUM_PRIME_IMPLICANTS (MAX_VBLES_PER_SMURF*MAX_VBLES_PER_SMURF*MAX_VBLES_PER_SMURF)
// can handle in a single constraint.

extern long MAX_NUM_PATH_EQUIV_SETS;
//#define MAX_NUM_PATH_EQUIV_SETS 6561 // 3^MAX_VBLES_PER_SMURF
// Max # of paths from the root state to a descendant state that
// we can handle in a single Smurf.  We consider these paths
// as sets:  they are considered to be equivalent up to permutation
// of elements.
//assert(MAX_NUM_PATH_EQUIV_SETS == 3 << MAX_VBLES_PER_SMURF);

//typedef unsigned char LemmaCounterType;
typedef struct _LemmaInfoStruct {

  // Chaff Lemma related members
  int nWatchedVble[2]; //When lemma is created, 
                     //most recently contradicted literal in lemma
  //int nWatchedVble2; //When lemma is created, 
                     //second most recently contradicted literal in lemma  
  int nWatchedVblePolarity[2];
  //int nWatchedVblePolarity2;
  bool bPutInCache; // Indicates whether to put the lemma into the cache when
                    // it is removed from the unit lemma list.
  _LemmaInfoStruct *pNextLemma[2]; // Next lemma watching WatchedLiteral1.
                                // Also used in the unit lemma list
                                // and in the list of free LemmaInfoStructs.
  //LemmaInfoStruct *pNextLemma2; // Next lemma watching WatchedLiteral2.

  // The next two members are needed to delete the LemmaInfoStruct
  // from the watched1 and watched2 lists when the lemma is recycled.
  _LemmaInfoStruct *pPrevLemma[2]; // Prev lemma watching WatchedLiteral1.
  //LemmaInfoStruct *pPrevLemma2; // Prev lemma watching WatchedLiteral2.
 
  //int *pnLemma; // Zero terminated array of the literals in the lemma.
                // The sign of an integer indicates the polarity of the
                // literal.
  LemmaBlock *pLemma; // Pointer to first lemma block for the lemma.
  // The lemma blocks are arranged in a linked list.
  // They contain the literals which would constitute the lemma if it
  // were written as a disjunction.
  // The first integer in the first lemma block specifies the number of
  // literals in the lemma.
  LemmaBlock *pLemmaLastBlock; // Ptr to the last block in the linked list.
  int nNumBlocks; // Number of lemma blocks used to hold the lemma.

  // The next two members are store links into the Lemma Priority Queue.
  // They are not initialized until the lemma is moved into the lemma cache.
  // (Lemmas are recycled from the lemma cache only.)
  int cache;
  _LemmaInfoStruct *pLPQPrev;
  _LemmaInfoStruct *pLPQNext;

  // After the lemma has been placed in the lemma cache, the following
  // member is used to keep track of the number of entries on
  // the backtrack stack whose pLemma members point to the pLemma
  // for this lemma.  If this count is greater than zero, then
  // we do not want to recycle this lemma.
  int nBacktrackStackReferences;

  // how many times this lemma was moved in front of the lpq
  int nNumLemmaMoved;

  // how many times this lemma caused contradiction
  int nNumLemmaConflict;

  // NUM_LEMMA_INTO_CACHE
  int nLemmaNumber;

  // how many times this lemma caused inference
  int nNumLemmaInfs;

  // when was this lemma first time useful
  int nLemmaFirstUseful;

  // when was this lemma used the last time
  int nLemmaLastUsed;

} LemmaInfoStruct;


ITE_INLINE void
ComputeLemmasForSmurf(SmurfState *pState);

ITE_INLINE void DisplayAllBrancherLemmas();
ITE_INLINE void DisplayAllBrancherLemmasToFile(char *filename, int flag);
ITE_INLINE void DisplayLemma(LemmaBlock *pLemma);
ITE_INLINE void DisplayLemmaToFile(FILE *pFile, LemmaBlock *pLemma);
ITE_INLINE void DisplayLemmaStatus(LemmaBlock *pLemma);
ITE_INLINE void DisplayLemmaLevels(LemmaBlock *pLemma, int arrLevel[]);
ITE_INLINE void DisplayLemmaInfo(LemmaInfoStruct *pLemmaInfo);
ITE_INLINE void InitLemmaSpacePool(int at_least);
ITE_INLINE void FreeLemmaSpacePool();
ITE_INLINE void AllocateMoreLemmaSpace(int at_least);
ITE_INLINE void InitLemmaLookupSpace();
ITE_INLINE void ResetLemmaLookupSpace();
ITE_INLINE void FreeLemmaLookupSpace();
ITE_INLINE void InitLemmaHeurArrays(int nMaxVbleIndex);
ITE_INLINE void DeleteLemmaHeurArrays();
ITE_INLINE void ConstructLemmasForAND(SpecialFunc *pSpecialFunc);
ITE_INLINE void ConstructLemmasForXOR(SpecialFunc *pSpecialFunc);
ITE_INLINE void ConstructLemmasForMINMAX(SpecialFunc *pSpecialFunc);
ITE_INLINE void InitLemmaInfoArray();
ITE_INLINE void FreeLemmaInfoArray();
ITE_INLINE bool IsInLemmaList(LemmaInfoStruct *pLemmaInfo, LemmaInfoStruct *pList);

ITE_INLINE LemmaInfoStruct *
AddLemma(int nNumLiterals,   //Can be used in the brancher
	 int *arrLiterals, 
	 bool bFlag,
	 LemmaInfoStruct *pUnitLemmaList,
	 LemmaInfoStruct **pUnitLemmaListTail
	 );	 

ITE_INLINE void FreeLemma(LemmaInfoStruct *pLemmaInfo);
ITE_INLINE void FreeLemmas(int n);
ITE_INLINE void LPQEnqueue(LemmaInfoStruct *pLemmaInfo);
ITE_INLINE void MoveToFrontOfLPQ(LemmaInfoStruct *pLemmaInfo);
ITE_INLINE void CheckLengthOfLemmaList(int nVble, int nPos, int nWhichWatch,
		       int nNumBacktracks);
ITE_INLINE void EnterIntoLemmaSpace(int nNumElts, int arrLemmaLiterals[],
		    bool bRecycleLemmasAsNeeded, LemmaBlock *&pFirstBlock,
		    LemmaBlock *&pLastBlock, int &nNumBlocks);

// lemmawlits -- watched literals
ITE_INLINE void RemoveLemmaFromWatchedLits(LemmaInfoStruct *pLemmaInfo);
ITE_INLINE void AddLemmaIntoWatchedLits(LemmaInfoStruct *p);
ITE_INLINE void LemmaSetWatchedLits(LemmaInfoStruct *pLemmaInfo, int *arr, int num);

// lemmainfo -- lemmainfostruct 
ITE_INLINE void InitLemmaInfoArray();
ITE_INLINE void FreeLemmaInfoArray();
ITE_INLINE void FreeLemmaInfoStruct(LemmaInfoStruct *pLemmaInfo);
ITE_INLINE LemmaInfoStruct * AllocateLemmaInfoStruct();

// lemmaspace
ITE_INLINE void InitLemmaSpacePool(int at_least);
ITE_INLINE void AllocateMoreLemmaSpace(int at_least);
ITE_INLINE void FreeLemmaBlocks(LemmaInfoStruct *pLemmaInfo);
ITE_INLINE void FillLemmaWithReversedPolarities(LemmaBlock *pLemma);
ITE_INLINE void FreeLemmaSpacePool();
ITE_INLINE void EnterIntoLemmaSpace(int nNumElts,
      int arrLemmaLiterals[],
      bool bRecycleLemmasAsNeeded,
      LemmaBlock *&pFirstBlock,
      LemmaBlock *&pLastBlock,
      int &nNumBlocks);

#endif

/*
 * Copyright 2024 Andrea Miele
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "cpu/pred/corr_predictor.hh"

#include "base/bitfield.hh"
#include "base/intmath.hh"

namespace gem5
{

namespace branch_prediction
{

#ifdef GEM5_DEBUG
int
CorrPredictor::BPHistory::newCount = 0;
#endif

const unsigned CorrPredictor::nMax = 16; // max supported value of n
const unsigned CorrPredictor::mMax = 16; // max supported value of m
const unsigned CorrPredictor::nLocalPredictorsMax = (1 << 16); // max number of distinct local predictors 	
 
// Constructor
CorrPredictor::CorrPredictor (const CorrPredictorParams& params)
    : BPredUnit(params),
      nLocalPredictors(params.nLocalPredictors),
      localPredictors(params.nLocalPredictors, std::vector<SatCounter8>((1 << (params.m - 1)), SatCounter8(params.n))),
      globalHistory(params.numThreads, 0),
      m(params.m),
      n(params.n),
      localPredictorThreshold((1 << (params.n - 1)) - 1),
      globalHistoryMask((1 << (params.m - 1)) - 1),
      addressHashMask((1 << (static_cast<unsigned>(log2(params.nLocalPredictors)) - 1)) - 1)
{
    if (!isPowerOf2(nLocalPredictors)) {
        fatal("Number of local predictors must be a power of 2!\n");
    }
 
    if (nLocalPredictors > CorrPredictor::nLocalPredictorsMax) {
        fatal("nLocalPredictorsMax cannot be larger than %d!\n", CorrPredictor::nLocalPredictorsMax);
    }
    
    if (m > CorrPredictor::mMax) {
        fatal("m cannot be larger than %d!\n", CorrPredictor::mMax);
    }
 
    if (n > CorrPredictor::nMax) {
        fatal("n cannot be larger than %d!\n", CorrPredictor::nMax);
    }
} 		

// Look up prediction
bool CorrPredictor::lookup(ThreadID tid, Addr pc, void* &bp_history) {
    // Create BP history
    CorrPredictor::BPHistory *history = new BPHistory;

    unsigned globalHistoryValue = globalHistory[tid]; // global history value for current thread
    SatCounter8 localPredictorValue = localPredictors[(pc >> instShiftAmt) & addressHashMask][globalHistoryValue]; // localPredictor value for current pc address and global history value
    // Save values in BP history
    history->globalHistoryValue = globalHistoryValue;
    history->localHistoryValue = localPredictorValue;
    history->updateLocalPredictor = true;
    bp_history = (void *)history;
    return (localPredictorValue > localPredictorThreshold);
}

// Update global history    
void CorrPredictor::updateHistories(ThreadID tid, Addr pc, bool uncond, bool taken,
                         Addr target,  void * &bp_history) {
   assert(uncond || bp_history);
   
   if (uncond) { // unconditional branch
       CorrPredictor::BPHistory *history = new BPHistory;
       history->globalHistoryValue = globalHistory[tid]; 
       history->updateLocalPredictor = false;
       bp_history = (void*)history;
   }

   // update global history
   globalHistory[tid] = ((globalHistory[tid] << 1) | taken) & globalHistoryMask;
}

// Update predictors
void CorrPredictor::update(ThreadID tid, Addr pc, bool taken,
              void * &bp_history, bool squashed,
              const StaticInstPtr & inst, Addr target) {
    assert(bp_history);
    CorrPredictor::BPHistory *history = static_cast<BPHistory *>(bp_history);

    // Misprediction: restore previous state
    // Do not change local predictors
    if (squashed) {
        // Global history restore and update
        globalHistory[tid] = ((history->globalHistoryValue << 1) | taken) & globalHistoryMask;
        return;
    }
    
    if (history->updateLocalPredictor) {
        if (taken) {
            localPredictors[(pc >> instShiftAmt) & addressHashMask][history->globalHistoryValue]++;
        }
        else { // not taken
            localPredictors[(pc >> instShiftAmt) & addressHashMask][history->globalHistoryValue]--;
        }
    }
    // history not needed after this point
    delete history;
    bp_history = nullptr;
}
 
// Branch instructiong was squashed
void CorrPredictor::squash(ThreadID tid, void * &bp_history) {
    assert(bp_history);
    CorrPredictor::BPHistory* history = static_cast<BPHistory *>(bp_history);
    globalHistory[tid] = history->globalHistoryValue;

    // history not needed after this point
    delete history;
    bp_history = nullptr;

} 

} // namespace branch_prediction
} // namespace gem5

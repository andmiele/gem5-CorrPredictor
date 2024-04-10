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

#ifndef __CPU_PRED_CORR_PRED_HH__
#define __CPU_PRED_CORR_PRED_HH__

#include "base/sat_counter.hh"
#include "base/types.hh"
#include "cpu/pred/bpred_unit.hh"
#include "params/CorrPredictor.hh"

namespace gem5
{

namespace branch_prediction
{

/**
 * Implements an (n,m) correlating predictor. One n-bit global history counter selects
 * one of the 2^n m-bit local predictors
 */
class CorrPredictor : public BPredUnit
{
  public:
    /**
     * Default branch predictor constructor
     */
    CorrPredictor(const CorrPredictorParams &params);

    // Override base class methods
    bool lookup(ThreadID tid, Addr pc, void* &bp_history) override;
    void updateHistories(ThreadID tid, Addr pc, bool uncond, bool taken,
                         Addr target,  void * &bp_history) override;
    void update(ThreadID tid, Addr pc, bool taken,
                void * &bp_history, bool squashed,
                const StaticInstPtr & inst, Addr target) override;
    void squash(ThreadID tid, void * &bp_history) override;

  private:

    /**
     * The branch history information that is created upon predicting
     * a branch. It will be in  upon updating and squashing,
     * and the branch predictor can use this information to update or restore its
     * state
     */
    struct BPHistory
    {
#ifdef GEM5_DEBUG
        BPHistory()
        { newCount++; }
        ~BPHistory()
        { newCount--; }

        static int newCount;
#endif
        unsigned globalHistoryValue;
        unsigned localHistoryValue;
	bool updateLocalPredictor; // false if local predictors must not be updated (unconditional branch)
    };
    
    static const unsigned nMax; // max supported value of n
    static const unsigned mMax; // max supported value of m
    static const unsigned nLocalPredictorsMax; // max number of distinct local predictors 	
    unsigned nLocalPredictors; // number of distinct local predictors
    std::vector<std::vector<SatCounter8>> localPredictors; // array of nLocalPredictors arrays of 2^m local predictors
    std::vector<unsigned> globalHistory; // global history registers; one per hardware thread
    unsigned m; // global history size
    unsigned n; // local predictor size

    unsigned localPredictorThreshold; // saturation counter threshold for local banch prediction
    unsigned globalHistoryMask; // mask used to reduce globalHistory integer modulo 2^m after update
    unsigned addressHashMask; // mask used to reduce pc address modulo the number of local predictors
};


} // namespace branch_prediction
} // namespace gem5

#endif // __CPU_PRED_CORR_PRED_HH__

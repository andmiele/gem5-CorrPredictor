# (m, n) Correlating Branch Predictor for gem5 cycle-accurate CPU and system-level simulator

The value of the m-bit global history is used as an index to select one of 2^m local n-bit predictors (2^m local predictors per "address hash bucket").

To use this Branch Predictor you need to select a CPU model with branch prediction, like O3CPU, and set system.cpu.branchPred = CorrPredictor().
 
## Files (and directories they must be put in relative to the gem5 codebase root directory)

1. <code>src/cpu/pred/corr_predictor.cc</code>: Correlating Branch Predictor C++ class implementation
2. <code>src/cpu/pred/corr_predictor.hh</code>: Correlating Branch Predictor C++ class header file
3. <code>src/cpu/pred/BranchPredictor.py</code>: This gem5 Python script is modified to include the Correlating Branch Predictor class parameters
4. <code>src/cpu/pred/SConscript</code>: This gem5 Scons script is modified to include the Correlating Branch Predictor class in the build process

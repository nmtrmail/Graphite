#ifndef SIMPLE_PERFORMANCE_MODEL_H
#define SIMPLE_PERFORMANCE_MODEL_H

#include "performance_model.h"

class SimplePerformanceModel : public PerformanceModel
{
public:
   SimplePerformanceModel();
   ~SimplePerformanceModel();

   void outputSummary(std::ostream &os);

   UInt64 getInstructionCount() { return m_instruction_count; }
   UInt64 getCycleCount() { return m_cycle_count; }

private:
   void handleInstruction(Instruction *instruction);
   
   UInt64 m_instruction_count;
   UInt64 m_cycle_count;
};

#endif

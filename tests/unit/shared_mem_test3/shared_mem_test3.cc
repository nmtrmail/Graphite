#include "tile.h"
#include "mem_component.h"
#include "tile_manager.h"
#include "simulator.h"

#include "carbon_user.h"
#include "fixed_types.h"

using namespace std;

void* thread_func(void*);

int num_addresses = 2;
int num_threads = 2;
int num_iterations = 2;

carbon_barrier_t barrier;
IntPtr* address;

int main (int argc, char *argv[])
{
   printf("Starting (shared_mem_test3)\n");
   // Start simulator
   CarbonStartSim(argc, argv);
   // Enable Models
   Simulator::enablePerformanceModelsInCurrentProcess();

   carbon_thread_t tid_list[num_threads];

   // Init barrier
   CarbonBarrierInit(&barrier, num_threads);

   Core* core = Sim()->getTileManager()->getCurrentTile()->getCore();

   address = new IntPtr[num_addresses];

   for (int j = 0; j < num_addresses; j++)
   {
      int val = 0;
      address[j] = j << 18;
      core->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::WRITE, address[j], (Byte*) &val, sizeof(val), true);
   }

   for (int i = 0; i < num_threads; i++)
   {
      tid_list[i] = CarbonSpawnThread(thread_func, (void*) i);
   }

   for (int i = 0; i < num_threads; i++)
   {
      CarbonJoinThread(tid_list[i]);
   }
  
   for (int j = 0; j < num_addresses; j++)
   {
      int val;
      core->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, address[j], (Byte*) &val, sizeof(val), true);
      
      printf("val[%i] = %i\n", j, val);
      if (val != (num_threads * num_iterations))
      {
         printf("shared_mem_test3 (FAILURE)\n");
      }
   }

   delete [] address;

   printf("shared_mem_test3 (SUCCESS)\n");
   
   // Disable performance models
   Simulator::disablePerformanceModelsInCurrentProcess();
   // Shutdown simulator
   CarbonStopSim();

   return 0;
}

void* thread_func(void*)
{
   CarbonBarrierWait(&barrier);
   Core* core = Sim()->getTileManager()->getCurrentTile()->getCore();

   for (int i = 0; i < num_iterations; i++)
   {
      for (int j = 0; j < num_addresses; j++)
      {
         int val;
         core->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::LOCK, Core::READ_EX, address[j], (Byte*) &val, sizeof(val), true);
         
         val += 1;

         core->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::UNLOCK, Core::WRITE, address[j], (Byte*) &val, sizeof(val), true);
      }
   }
}

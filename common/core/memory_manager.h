#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <string>
#include <cassert>

#include "shmem_req_types.h"
#include "network.h"
#include "packet_type.h"
#include "core.h"
#include "ocache.h"
#include "address_home_lookup.h"
#include "cache_state.h"
#include "cond.h"
#include "lock.h"

class DramDirectory;

class MemoryManager
{
   public:
      
      enum sm_payload_t
      {
         REQUEST,
         UPDATE,
         ACK,
         PAYLOAD_NUM_STATES
      };

      /* one could make the argument to have one payload
       * with fields that may not be used....
       */
      struct RequestPayload
      {
         shmem_req_t request_type;
         IntPtr request_address;
         UInt32 request_num_bytes;

         RequestPayload()
               : request_type(READ),
               request_address(0),
               request_num_bytes(0)
         {}
      };

      struct UpdatePayload
      {
         CacheState::cstate_t update_new_cstate;
         IntPtr update_address;
         UInt32 data_size; //in bytes

         UpdatePayload()
               : update_new_cstate(CacheState::INVALID),
                 update_address(0),
                 data_size(0)
         {}
      };

      struct AckPayload
      {
         CacheState::cstate_t ack_new_cstate;
         IntPtr ack_address;
         UInt32 data_size; //this is used to tell us how much data to extract
         bool is_writeback; //when we invalidate/demote owners, we may need to do a writeback

         //if sent a downgrade message (E->S), but cache
         //no longer has the line, send a bit to tell dram directory
         //to remove it from the sharers' list
         bool remove_from_sharers;

         AckPayload()
               : ack_new_cstate(CacheState::INVALID),
               ack_address(0),
               data_size(0),
               is_writeback(false),
               remove_from_sharers(false)
         {}
      };

   private:
      SInt32 m_core_id;

      Network *m_network;
      OCache *m_ocache;
      DramDirectory *m_dram_dir;
      AddressHomeLookup *m_addr_home_lookup;

      Lock m_mmu_lock;
      ConditionVariable m_mmu_cond;
      volatile bool m_received_reply;
      volatile bool cache_locked;

      UInt32 m_cache_line_size;

      bool actionPermissable(CacheState cache_state, shmem_req_t shmem_req_t);

      // Locking and Unlocking Cache
      void lockCache();
      void unlockCache();
      bool isCacheLocked();

      void debugPrintReqPayload(MemoryManager::RequestPayload payload);

      // knobs
      static UInt32 m_knob_ahl_param;
      static UInt32 m_knob_dram_access_cost;
      static UInt32 m_knob_line_size;

   public:

      MemoryManager(SInt32 core_id, Network *network, OCache *ocache);
      virtual ~MemoryManager();

      DramDirectory* getDramDirectory() { return m_dram_dir; }

      //cache interfacing functions.
      void setCacheLineInfo(IntPtr ca_address, CacheState::cstate_t new_cstate);
      pair<bool, CacheTag*> getCacheLineInfo(IntPtr address);
      void accessCacheLineData(CacheBase::AccessType access_type, IntPtr ca_address, UInt32 offset, Byte* data_buffer, UInt32 data_size);
      void fillCacheLineData(IntPtr ca_address, Byte* fill_buffer);
      void invalidateCacheLine(IntPtr address);

      static void createUpdatePayloadBuffer(UpdatePayload* send_payload, Byte *data_buffer, Byte *payload_buffer, UInt32 payload_size);
      static void createAckPayloadBuffer(AckPayload* send_payload, Byte *data_buffer, Byte *payload_buffer, UInt32 payload_size);
      static void extractUpdatePayloadBuffer(NetPacket* packet, UpdatePayload* payload, Byte* data_buffer);
      static void extractAckPayloadBuffer(NetPacket* packet, AckPayload* payload, Byte* data_buffer);
      static void extractRequestPayloadBuffer(NetPacket* packet, RequestPayload* payload);

      static NetPacket makePacket(PacketType packet_type, Byte* payload_buffer, UInt32 payload_size, int sender_rank, int receiver_rank);
      static NetMatch makeNetMatch(PacketType packet_type, int sender_rank);

      //core traps all memory accesses here.
      bool initiateSharedMemReq(Core::lock_signal_t lock_signal, shmem_req_t shmem_req_type, IntPtr ca_address, UInt32 addr_offset, Byte* data_buffer, UInt32 buffer_size);

      //request from DRAM permission to use an address
      //writes requested data into the "fill_buffer", and writes what the new_cstate should be on the receiving end
      void requestPermission(shmem_req_t shmem_req_type, IntPtr address);

      //network interrupt calls this.
      //Any requests are serialized onto a stack,
      //and then forwarded to the DRAM for processing.
      void addMemRequest(NetPacket req_packet);

      //network interrupt calls this.
      //All UPDATE_UNEXPECTED packets are sent here.
      void processUnexpectedSharedMemUpdate(NetPacket update_packet);

      //network interrupt calls this.
      //the MMU then forwards write-back packet to the DRAM.
      void forwardWriteBackToDram(NetPacket wb_packet);

      void processAck(NetPacket update_packet);

      // Processes Shared Memory Response
      void processSharedMemResponse(NetPacket rep_packet);

      // Process Initial Shared Memory Request
      void processSharedMemInitialReq (NetPacket rep_packet);

      //debugging stuff
      static string sMemReqTypeToString(shmem_req_t type);
};

#include "dram_directory.h"

#endif

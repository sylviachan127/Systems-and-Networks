// Yuen Han Chan

#include <stdlib.h>
#include <stdio.h>
#include "tlb.h"
#include "pagetable.h"
#include "global.h" /* for tlb_size */
#include "statistics.h"

/*******************************************************************************
 * Looks up an address in the TLB. If no entry is found, attempts to access the
 * current page table via cpu_pagetable_lookup().
 *
 * @param vpn The virtual page number to lookup.
 * @param write If the access is a write, this is 1. Otherwise, it is 0.
 * @return The physical frame number of the page we are accessing.
 */
pfn_t tlb_lookup(vpn_t vpn, int write) {
   pfn_t pfn;

   /* 
    * FIX ME : Step 6
    */

   /* 
    * Search the TLB for the given VPN. Make sure to increment count_tlbhits if
    * it was a hit!
    */

//    tlbe_t* this_tlbe = NULL;
  
    for(int i =0; i<tlb_size; i++){
      if((tlb[i].vpn == vpn)&&(tlb[i].valid)){
        pfn = tlb[i].pfn;
        tlb[i].dirty = write;
        tlb[i].valid = 1;
        count_tlbhits++;
        return pfn;
      }
    }
    
   /* If it does not exist (it was not a hit), call the page table reader */
      pfn = pagetable_lookup(vpn, write);

   /* 
    * Replace an entry in the TLB if we missed. Pick invalid entries first,
    * then do a clock-sweep to find a victim.
    */

    int done = 0;

    for(int i = 0; i<tlb_size; i++){
      if(tlb[i].valid==0){
        tlb[i].vpn = vpn;
        tlb[i].pfn = pfn;
        tlb[i].valid = 1;
        tlb[i].dirty = write;
        tlb[i].used = 1;
        done = 1;
        break;
      }
    }

    if(!done){
     for(int i = 0; i<tlb_size; i++){
        if(tlb[i].used==0){
          tlb[i].vpn = vpn;
          tlb[i].pfn = pfn;
          tlb[i].valid = 1;
          tlb[i].dirty = write;
          tlb[i].used = 1;
          done = 1;
          break;
        }
      }
    }

    if(!done){
      tlb[0].vpn = vpn;
      tlb[0].pfn = pfn;
      tlb[0].valid = 1;
      tlb[0].dirty = write;
      tlb[0].used = 1;
    }

   /*
    * Perform TLB house keeping. This means marking the found TLB entry as
    * accessed and if we had a write, dirty. We also need to update the page
    * table in memory with the same data.
    *
    * We'll assume that this write is scheduled and the CPU doesn't actually
    * have to wait for it to finish (there wouldn't be much point to a TLB if
    * we didn't!).
    */

     


   return pfn;
}


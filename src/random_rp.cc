/**
 * Copyright (c) 2018-2020 Inria
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "mem/cache/replacement_policies/random_rp.hh"

#include <cassert>
#include <memory>

#include "base/random.hh"
#include "params/RandomRP.hh"

unsigned gem5::replacement_policy::Random::found_miss = 0;
unsigned gem5::replacement_policy::Random::found_access = 0;
unsigned gem5::replacement_policy::Random::roi_threshold = 1;
unsigned gem5::replacement_policy::Random::init_flag = 0;
float gem5::replacement_policy::Random::last_miss_rate = 0.0;

namespace gem5
{

namespace replacement_policy
{

Random::Random(const Params &p)
  : Base(p)
{
}

void
Random::invalidate(const std::shared_ptr<ReplacementData>& replacement_data)
{
    // Unprioritize replacement data victimization
    //std::static_pointer_cast<RandomReplData>(
    //    replacement_data)->valid = false;
    
    // Reset reference count
    std::static_pointer_cast<RandomReplData>(
        replacement_data)->refCount = 0;
    
    // Reset last touch timestamp
    std::static_pointer_cast<RandomReplData>(
        replacement_data)->lastTouchTick = Tick(0);
    
}

void
Random::touch(const std::shared_ptr<ReplacementData>& replacement_data) const
{
    // Update reference count
    std::static_pointer_cast<RandomReplData>(replacement_data)->refCount++;
    
    //if ref count > threshold, move to MRU, otherwise, move to LRU
    if(std::static_pointer_cast<RandomReplData>(replacement_data)->refCount > Random::roi_threshold){
        // Update last touch timestamp
        std::static_pointer_cast<RandomReplData>(
            replacement_data)->lastTouchTick = curTick();
    }else {
        std::static_pointer_cast<RandomReplData>(
            replacement_data)->lastTouchTick = 1;
    } 
    
    //increase access count
    Random::found_access++;

}

void
Random::reset(const std::shared_ptr<ReplacementData>& replacement_data) const
{
    float found_miss_rate = 0.0;    

    //called when inserting the entry

    // Reset reference count
    std::static_pointer_cast<RandomReplData>(replacement_data)->refCount = 1;
     
    // Make their timestamps as old as possible, so that they become LRU
    std::static_pointer_cast<RandomReplData>(
        replacement_data)->lastTouchTick = 1;
    
    //check init flag and do not tune the threshold at the first 10_000_000 accesses
    if(Random::init_flag != 513 && Random::found_access > 10000000){
        Random::found_access   = 1;
        Random::found_miss     = 0;
        Random::roi_threshold  = 1; //init to small number
        Random::init_flag      = 513;
        Random::last_miss_rate = 0.99;//init to 99% miss rate
        std::cout << "Init flag is set!" << std::endl;
    } else {
        Random::found_access++;
    }

    //check if we accumulates to 10_000_000 accesses
    if(Random::init_flag == 513 && Random::found_access > 10000000){
       found_miss_rate = static_cast<float>(Random::found_miss)/static_cast<float>(Random::found_access);
       std::cout << "There are "<<Random::found_access<<" cache accesses"<< std::endl;
       std::cout << "There are "<<Random::found_miss<< " cache misses"<< std::endl;
       
       std::cout <<"The miss rate now is " << found_miss_rate*10000 <<" out of 10000" << std::endl;
       std::cout <<"Last miss rate is " << Random::last_miss_rate*10000<<" out of 10000" << std::endl;

       //check miss rate and see whether to adjust threshold
       if(found_miss_rate < Random::last_miss_rate){
           //increase threshold if current miss rate is better, maximal is 22
           std::cout << "The miss rate is better, increasing the threshold" << std::endl;
           if(Random::roi_threshold < 22){
              Random::roi_threshold = Random::roi_threshold + 4;
           }
       }else {
           //decrease threshold if current miss rate is worse, minimal is 1
           std::cout << "The miss rate is worse, decreasing the threshold" << std::endl;
           if(Random::roi_threshold > 1){
              Random::roi_threshold = Random::roi_threshold - 4;
           }
       }
       std::cout << "The threshold now is " << Random::roi_threshold << std::endl;
       std::cout << std::endl;
       std::cout << std::endl;
       
       //update the static values
       Random::last_miss_rate = found_miss_rate;
       Random::found_access = 0;
       Random::found_miss   = 0;
    }

}

ReplaceableEntry*
Random::getVictim(const ReplacementCandidates& candidates) const
{
    Random::found_miss++;

    // There must be at least one replacement candidate
    assert(candidates.size() > 0);
    
    // Visit all candidates to find victim
    ReplaceableEntry* victim = candidates[0];
    for (const auto& candidate : candidates) {
        // Update victim entry if necessary
        if (std::static_pointer_cast<RandomReplData>(
                    candidate->replacementData)->lastTouchTick <
                std::static_pointer_cast<RandomReplData>(
                    victim->replacementData)->lastTouchTick) {
            victim = candidate;
        }
    }
  
    return victim;
}

std::shared_ptr<ReplacementData>
Random::instantiateEntry()
{
    return std::shared_ptr<ReplacementData>(new RandomReplData());
}

} // namespace replacement_policy
} // namespace gem5

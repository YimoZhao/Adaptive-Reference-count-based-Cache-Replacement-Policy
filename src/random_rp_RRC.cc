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
    
    //if ref count > 14, move to MRU, otherwise, move to LRU
    if(std::static_pointer_cast<RandomReplData>(replacement_data)->refCount > 14){
        // Update last touch timestamp
        std::static_pointer_cast<RandomReplData>(
            replacement_data)->lastTouchTick = curTick();
    }else {
        std::static_pointer_cast<RandomReplData>(
            replacement_data)->lastTouchTick = 1;
    } 
}

void
Random::reset(const std::shared_ptr<ReplacementData>& replacement_data) const
{
    // Unprioritize replacement data victimization
    //std::static_pointer_cast<RandomReplData>(
    //    replacement_data)->valid = true;
    
    //called when inserting the entry

    // Reset reference count
    std::static_pointer_cast<RandomReplData>(replacement_data)->refCount = 1;
     
    // Make their timestamps as old as possible, so that they become LRU
    std::static_pointer_cast<RandomReplData>(
        replacement_data)->lastTouchTick = 1;
}

ReplaceableEntry*
Random::getVictim(const ReplacementCandidates& candidates) const
{
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

#pragma once

#include "srsvm/memory.h"

bool srsvm_mmu_segment_contains(const srsvm_memory_segment *segment, const srsvm_ptr address, const srsvm_word size);
bool srsvm_mmu_segment_contains_literal(const srsvm_memory_segment *segment, const srsvm_ptr address, const srsvm_word size);

srsvm_memory_segment*  srsvm_mmu_locate(srsvm_memory_segment *root_segment, srsvm_ptr address);

bool srsvm_mmu_store(srsvm_memory_segment *root_segment, const srsvm_ptr address, const srsvm_word bytes, void* src);
bool srsvm_mmu_load(srsvm_memory_segment *root_segment, const srsvm_ptr address, const srsvm_word bytes, void* dest);

srsvm_memory_segment* srsvm_mmu_alloc_literal(srsvm_memory_segment *parent_segment, const srsvm_word literal_size, const srsvm_ptr suggested_base_address);
srsvm_memory_segment* srsvm_mmu_alloc_virtual(srsvm_memory_segment *parent_segment, const srsvm_word virtual_size, const srsvm_ptr base_address);

void srsvm_mmu_free(srsvm_memory_segment *segment);
void srsvm_mmu_free_force(srsvm_memory_segment *segment);

void srsvm_mmu_set_all_free(srsvm_memory_segment *segment);

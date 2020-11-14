#include <stdlib.h>
#include <string.h>

#include <limits.h>

#include "debug.h"
#include "memory.h"

bool srsvm_mmu_segment_contains(const srsvm_memory_segment *segment, const srsvm_ptr address, const srsvm_word size)
{
    return address >= segment->min_address && (address + size) < segment->max_address;
}

bool srsvm_mmu_segment_contains_literal(const srsvm_memory_segment *segment, const srsvm_ptr address, const srsvm_word size)
{
    if(segment->literal_sz == 0){
        return false;
    } else {
        return address >= segment->literal_start && (address + size) < (segment->literal_start + segment->literal_sz);
    }
}

srsvm_memory_segment* srsvm_mmu_locate(srsvm_memory_segment *root_segment, const srsvm_ptr address)
{
    dbg_printf("attempting to locate address " SWFX " in segment %p", address, root_segment);

    srsvm_memory_segment *segment = NULL;

    srsvm_lock_acquire(&root_segment->lock, 0);

    dbg_printf("  segment literal bounds: [" SWFX ", " SWFX ")", root_segment->literal_start, root_segment->literal_start + root_segment->literal_sz);
    
    if(srsvm_mmu_segment_contains_literal(root_segment, address, 0)){
        dbg_puts("  literal match");
        segment = root_segment;
    } else {
        dbg_printf("  segment virtual bounds: [" SWFX ", " SWFX ")", root_segment->min_address, root_segment->max_address);

        if(address < root_segment->min_address || address >= root_segment->max_address)
        {
            dbg_puts("  out of bounds");
            segment = NULL;
        } else {
            struct srsvm_memory_segment *child_segment;
            struct srsvm_memory_segment *locate_result;

            for(int segment_idx = 0; segment_idx < WORD_SIZE; segment_idx++){
                child_segment = root_segment->children[segment_idx];

                if(child_segment != NULL){
                    dbg_printf("  searching in child segmnet %p", child_segment);

                    if((locate_result = srsvm_mmu_locate(child_segment, address)) != NULL){
                        dbg_printf("  found address in child segment %p", child_segment);
                        segment = locate_result;
                        break;
                    }
                }
            }
        }
    }

    srsvm_lock_release(&root_segment->lock);

    if(segment == NULL){
        dbg_puts("address not found in an allocated segment");
    }

    return segment;
}

bool srsvm_mmu_store(srsvm_memory_segment *root_segment, const srsvm_ptr address, const srsvm_word bytes, void* src)
{
    dbg_printf("attemping to store " SWF " bytes to address " SWFX, bytes, address);

    srsvm_memory_segment *segment = srsvm_mmu_locate(root_segment, address);

    if(segment == NULL || !srsvm_mmu_segment_contains_literal(segment, address, bytes) || ! segment->writable || segment->locked){
        if(segment == NULL){
            dbg_puts("failed to locate segment");
        } else if(! segment->writable){
            dbg_puts("segment not writable");
        } else if(segment->locked){
            dbg_puts("segment locked");
        } else {
            dbg_puts("resolved segment does not contain address");
        }

        return false;
    }

    dbg_puts("segment resolved, storing...");

    srsvm_lock_acquire(&segment->lock, 0);

    void *cpy_dest = segment->literal_memory + (uintptr_t)(address - segment->literal_start);
    void *cpy_src = src; 

    if(sizeof(srsvm_word) > sizeof(size_t)){
        srsvm_word bytes_remaining = bytes;

        while(bytes_remaining > 0){
            size_t single_copy;

            if(bytes_remaining > SIZE_MAX){
                single_copy = SIZE_MAX;
            } else {
                single_copy = (size_t) bytes_remaining;
            }

            memcpy(cpy_dest, cpy_src, single_copy);

            cpy_dest += single_copy;
            cpy_src += single_copy;

            bytes_remaining -= single_copy;
        }
    } else {
        memcpy(cpy_dest, cpy_src, (size_t) bytes);
    }

    srsvm_lock_release(&segment->lock);
    
    dbg_puts("store successful");

    return true;
}

bool srsvm_mmu_load(srsvm_memory_segment *root_segment, const srsvm_ptr address, const size_t bytes, void* dest)
{
    srsvm_memory_segment *segment = srsvm_mmu_locate(root_segment, address);

    if(segment == NULL || !srsvm_mmu_segment_contains_literal(segment, address, bytes) || ! segment->readable){
        if(segment == NULL){
            dbg_puts("failed to locate segment");
        } else if(! segment->readable){
            dbg_puts("segment not writable");
        } else {
            dbg_puts("resolved segment does not contain address");
        }
        
        return false;
    }
    
    dbg_puts("segment resolved, loading...");

    srsvm_lock_acquire(&segment->lock, 0);

    void *cpy_dest = dest;
    void *cpy_src = segment->literal_memory + (uintptr_t)(address - segment->literal_start);

    if(sizeof(srsvm_word) > sizeof(size_t)){
        srsvm_word bytes_remaining = bytes;

        while(bytes_remaining > 0){
            size_t single_copy;

            if(bytes_remaining > SIZE_MAX){
                single_copy = SIZE_MAX;
            } else {
                single_copy = (size_t) bytes_remaining;
            }

            memcpy(cpy_dest, cpy_src, single_copy);

            cpy_dest += single_copy;
            cpy_src += single_copy;

            bytes_remaining -= single_copy;
        }
    } else {
        memcpy(cpy_dest, cpy_src, (size_t) bytes);
    }

    srsvm_lock_release(&segment->lock);
    
    dbg_puts("load successful");

    return true;
}

static void lock_all(srsvm_memory_segment *segment)
{
    if(segment->parent != NULL){
        lock_all(segment->parent);
    }

    srsvm_lock_acquire(&segment->lock, 0);
}

static void release_all(srsvm_memory_segment *segment)
{
    if(segment->parent != NULL){
        release_all(segment->parent);
    }

    srsvm_lock_release(&segment->lock);
}

static bool has_room(srsvm_memory_segment *parent, const srsvm_word bytes, const srsvm_ptr base_address, srsvm_memory_segment *prev_parent, bool child_slot)
{
    dbg_printf("looking for room for " SWF " bytes in segment %p", bytes, parent);

    if(base_address > 0){
        dbg_printf("  requested base address: " SWFX, base_address);
        
        if(base_address < parent->min_address || base_address + bytes > parent->max_address){
            if(parent->parent != NULL){
                if(! has_room(parent->parent, bytes, base_address, parent, false)){
                    dbg_printf("parent segment %p does not have room", parent->parent);

                    return false;
                }
            }
        } else if (base_address >= parent->literal_start && base_address + bytes < parent->literal_start + parent->literal_sz){
            dbg_puts("literal intersect 1");

            return false;
        } else if(base_address + bytes >= parent->literal_start + parent->literal_sz && base_address < parent->literal_start + parent->literal_sz){
            dbg_puts("literal intersect 2");

            return false;
        } else {
            dbg_puts("  requested region lies in this segment");
        }

        dbg_puts("  found room");
    }

    if(! child_slot){
        dbg_puts("  looking at child slots...");
        for(int child_slot = 0; child_slot < WORD_SIZE; child_slot++){
            if(parent->children[child_slot] != NULL){
                if(prev_parent != NULL && prev_parent == parent->children[child_slot]){
                    continue;
                } else {
                    if(base_address > 0){
                        dbg_printf("  checking region intersection for child segment %p", parent->children[child_slot]);
                        
                        if(base_address >= parent->children[child_slot]->min_address && base_address + bytes < parent->children[child_slot]->max_address && !has_room(parent->children[child_slot], bytes, base_address, NULL, true)){
                            dbg_puts("intersection found 1");

                            return false;
                        } else if(base_address + bytes >= parent->children[child_slot]->min_address && base_address < parent->children[child_slot]->max_address && !has_room(parent->children[child_slot], bytes, base_address, NULL, true)){
                            dbg_puts("intersection found 2");

                            return false;
                        }
                    } 
                }
            } else break;
        }
    }

    dbg_puts("sufficient room located");

    return true;
}

static int find_free_slot(srsvm_memory_segment *parent)
{
    for(int slot = 0; slot < WORD_SIZE; slot++){
        if(parent->children[slot] == NULL){
            return slot;
        }
    }

    return -1;
}

int compare_segment_bounds(const void* seg_a, const void* seg_b)
{
    const srsvm_memory_segment *a = *(const srsvm_memory_segment**)seg_a;
    const srsvm_memory_segment *b = *(const srsvm_memory_segment**)seg_b;

    if(a == NULL && b != NULL){
        return 1;
    } else if(a != NULL && b == NULL){
        return -1;
    } else if(a == NULL && b == NULL){
        return 0;
    } else {
        if(a->min_address < b->min_address){
            return -1;
        } else if(a->min_address > b->min_address){
            return 1;
        } else {
            return 0;
        }
    }
}

static void update_seg_bounds(srsvm_memory_segment *child, srsvm_memory_segment *parent)
{
    if(parent != NULL){
        if(parent->max_address < child->max_address){
            parent->max_address = child->max_address;
        }

        if(parent->min_address > child->min_address){
            parent->min_address = child->min_address;
        }

        update_seg_bounds(parent, parent->parent);
    }

    qsort(&child->children, WORD_SIZE, sizeof(srsvm_memory_segment*), compare_segment_bounds);
}

static bool insert_segment(srsvm_memory_segment *parent, srsvm_memory_segment *child, const srsvm_ptr suggested_base_address, const bool force_virtual)
{   
    dbg_printf("attempting to insert child segment %p into parent segment %p", child, parent);

    dbg_printf("child virtual size: " SWF, child->sz);
    dbg_printf("child literal size: " SWF, child->literal_sz);

    bool success = false;

    int child_slot;

    if(suggested_base_address > 0){
        dbg_printf("  requested base address: " SWFX, suggested_base_address);
    }

    if(! has_room(parent, child->sz, suggested_base_address, NULL, false)){
        dbg_puts("insufficient room for requested base address");

        if(!force_virtual){
            if(suggested_base_address > 0 && has_room(parent, child->sz, 0, NULL, false)){
                dbg_puts("  sufficient room for different base address");

                if((child_slot = find_free_slot(parent)) != -1){
                    dbg_printf("  inserting child into parent slot %d", child_slot);
                    
                    parent->children[child_slot] = child;
                    child->parent = parent;

                    if(child_slot == 0){
                        child->min_address = parent->literal_start + parent->literal_sz;
                        child->literal_start = parent->literal_start + parent->literal_sz;

                        child->max_address = child->literal_start + child->sz;

                        child->level = parent->level+1;

                        update_seg_bounds(child, child->parent);

                        success = true;
                    } else {
                        srsvm_memory_segment *previous_child = parent->children[child_slot-1];

                        child->min_address = previous_child->max_address;
                        child->literal_start = previous_child->max_address;

                        child->max_address = child->literal_start + child->sz;

                        child->level = parent->level+1;

                        update_seg_bounds(child, child->parent);

                        success = true;
                    }
                } else {
                    dbg_puts("no free slot found in parent, looking for space in a child segment...");

                    for(child_slot = 0; child_slot < WORD_SIZE; child_slot++){
                        if(insert_segment(parent->children[child_slot], child, 0, force_virtual)){
                            dbg_printf("found space in child segment %p", parent->children[child_slot]);

                            success = true;
                            break;
                        }
                    }
                }
            }
        } else {
            dbg_puts("bailing since a virtual segment is being forced");
        }
    } else {
        dbg_puts("  sufficient room found in parent");

        if((child_slot = find_free_slot(parent)) != -1){
            dbg_printf("  inserting child into parent slot %d", child_slot);
           
            parent->children[child_slot] = child;
            child->parent = parent;

            child->min_address = suggested_base_address;
            child->literal_start = suggested_base_address;

            child->max_address = suggested_base_address + child->sz;

            child->level = parent->level+1;

            update_seg_bounds(child, child->parent);

            dbg_puts("base address granted");

            success = true;
        } else {
            dbg_puts("no free slot found in parent, looking for space in a child segment...");
            
            for(child_slot = 0; child_slot < WORD_SIZE; child_slot++){
                if(insert_segment(parent->children[child_slot], child, suggested_base_address, force_virtual)){ 
                    dbg_printf("found space in child segment %p", parent->children[child_slot]);
                    dbg_puts("base address granted");
                    
                    success = true;
                    break;
                }
            }

            if(! success){
                for(child_slot = 0; child_slot < WORD_SIZE; child_slot++){
                    if(insert_segment(parent->children[child_slot], child, 0, force_virtual)){
                        dbg_printf("found space in child segment %p", parent->children[child_slot]);

                        success = true;
                        break;
                    }
                }
            }
        }
    }

    return success;
}

srsvm_word alloc_size(const srsvm_word requested_bytes)
{
    /* https://stackoverflow.com/a/35485579/1633848 */

    srsvm_word actual_bytes = requested_bytes;

    actual_bytes--;

    actual_bytes |= actual_bytes >> 1;
    actual_bytes |= actual_bytes >> 2;
    actual_bytes |= actual_bytes >> 4;
    actual_bytes |= actual_bytes >> 8;

#if WORD_SIZE == 32
    actual_bytes |= actual_bytes >> 16;
#elif WORD_SIZE == 64
    actual_bytes |= actual_bytes >> 16;
    actual_bytes |= actual_bytes >> 32;
#elif WORD_SIZE == 128
    actual_bytes |= actual_bytes >> 16;
    actual_bytes |= actual_bytes >> 32;
    actual_bytes |= actual_bytes >> 64;
#endif

    actual_bytes++;

    if(actual_bytes < requested_bytes){
        // overflow?
        return requested_bytes;
    } else {
        return actual_bytes;
    }
}

static srsvm_memory_segment* srsvm_mmu_alloc(srsvm_memory_segment *parent_segment, const srsvm_word literal_size, const srsvm_word virtual_size, const srsvm_ptr suggested_base_address, const bool force_virtual)
{
    dbg_printf("allocating memory segment, literal size: " SWF ", virtual_size: " SWF ", requested base address: " SWFX, literal_size, virtual_size, suggested_base_address);

    srsvm_memory_segment *segment = NULL;

    if(literal_size == 0 && virtual_size == 0){
        dbg_puts("empty segment requested, bailing");

        goto error_cleanup;
    } else {
        segment = malloc(sizeof(srsvm_memory_segment));

        if(segment == NULL){
            dbg_printf("malloc failed: %s", strerror(errno));
        }
    }

    if(segment != NULL){
        segment->literal_memory = NULL;

        if(! srsvm_lock_initialize(&segment->lock)){
            goto error_cleanup;
        } else if(literal_size > 0 && (segment->literal_memory = malloc(literal_size * sizeof(char))) == NULL){
            goto error_cleanup;
        } else {
            segment->literal_sz = literal_size;

            if(literal_size > 0){
                segment->literal_start = suggested_base_address;
            } else {
                segment->literal_start = SRSVM_NULL_PTR;
            }

            if(virtual_size == 0){
                segment->sz = alloc_size(literal_size);
            } else {
                segment->sz = virtual_size;
            }

            segment->readable = true;
            segment->writable = true;
            segment->executable = true;

            segment->locked = false;

            segment->free_flag = false;

            for(int child_idx = 0; child_idx < WORD_SIZE; child_idx++){
                segment->children[child_idx] = NULL;
            }

            if(parent_segment != NULL){
                dbg_printf("allocated child segment %p", segment);

                dbg_printf("attempting to insert segment in parent segment %p...", parent_segment);

                lock_all(parent_segment);

                if(! insert_segment(parent_segment, segment, suggested_base_address, force_virtual)){
                    dbg_puts("failed to insert segment");

                    release_all(parent_segment);

                    goto error_cleanup;
                } else {
                    dbg_puts("segment inserted");
                }

                release_all(parent_segment);
            } else {
                dbg_printf("allocated root segment: %p", segment);

                segment->parent = NULL;
                segment->level = 0;

                segment->min_address = suggested_base_address;
                segment->max_address = suggested_base_address + segment->sz;
            }
        }
    }

    return segment;

error_cleanup:
    if(segment != NULL){
        srsvm_lock_destroy(&segment->lock);
        if(segment->literal_memory != NULL) free(segment->literal_memory);
        free(segment);
    }

    return NULL;
}

srsvm_memory_segment *srsvm_mmu_alloc_literal(srsvm_memory_segment *parent_segment, const srsvm_word literal_size, const srsvm_ptr suggested_base_address)
{
    return srsvm_mmu_alloc(parent_segment, literal_size, 0, suggested_base_address,  false);
}

srsvm_memory_segment *srsvm_mmu_alloc_virtual(srsvm_memory_segment *parent_segment, const srsvm_word virtual_size, const srsvm_ptr base_address)
{
    return srsvm_mmu_alloc(parent_segment, 0, virtual_size, base_address, true); 
}

static bool children_freed(srsvm_memory_segment *segment)
{
    bool all_freed = true;

    for(int child_slot = 0; child_slot < WORD_SIZE; child_slot++){
        if(segment->children[child_slot] != NULL){
            if(! segment->children[child_slot]->free_flag){
                all_freed = false;
            } else if(! children_freed(segment->children[child_slot]))
            {
                all_freed = false;
            }
        } else break;
    }

    return all_freed;
}

bool free_tree(srsvm_memory_segment *segment)
{
    if(! segment->free_flag){
        return false;
    }

    bool can_free = true;

    for(int child_slot = 0; child_slot < WORD_SIZE; child_slot++){
        if(segment->children[child_slot] != NULL){
            if(segment->children[child_slot]->free_flag && children_freed(segment->children[child_slot])){
                if(free_tree(segment->children[child_slot])){
                    segment->children[child_slot] = NULL;
                } else {
                    can_free = false;
                }
            }
        } else break;
    }

    if(can_free && children_freed(segment)){
        if(segment->literal_memory != NULL){
            free(segment->literal_memory);
            segment->literal_memory = NULL;
        }

        srsvm_lock_release(&segment->lock);
        srsvm_lock_destroy(&segment->lock);

        free(segment);

        return true;
    } else {
        return false;
    }
}

void srsvm_mmu_free(srsvm_memory_segment *segment)
{
    segment->free_flag = true;

    lock_all(segment);

    free_tree(segment);

    release_all(segment);
}

void srsvm_mmu_free_force(srsvm_memory_segment *segment)
{
    for(int child_slot = 0; child_slot < WORD_SIZE; child_slot++){
        if(segment->children[child_slot] != NULL){
            srsvm_mmu_free_force(segment->children[child_slot]);
        } else break;
    }

    srsvm_lock_release(&segment->lock);
    srsvm_lock_destroy(&segment->lock);

    free(segment);
}

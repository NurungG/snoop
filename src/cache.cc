#include "cache.h"

#include <iostream>
using namespace std;

static int num_of_bits(int num) {
    int ret = 0;
    while ((num >>= 1)) {
        ++ret;
    }
    return ret;
}

pcc::pcc(int _cap, int _ways, int _block_sz) : cap(_cap), n_way(_ways), block_sz(_block_sz) {
    cap <<= 10; // scale up

    n_entry = cap / block_sz;
    n_set   = n_entry / n_way;

    b_offset = num_of_bits(block_sz);
    b_index  = num_of_bits(cap / n_way / block_sz);
    b_tag    = 64 - b_offset - b_offset;

    offset_mask = UINT64_MAX >> (64 - b_offset);
    tag_mask    = UINT64_MAX << (64 - b_tag);
    index_mask  = ( UINT64_MAX ^ offset_mask ) & ( UINT64_MAX ^ tag_mask);

    // Allocate cache
    way = new struct cache_way [n_way];
    set = new struct cache_set [n_set];

    for (int i = 0; i < n_way; i++) {
        way[i].tbl = new struct cache_entry [n_set];
    }
    for (int i = 0; i < n_set; i++) {
        set[i].ptr = new struct cache_entry * [n_way];
        set[i].lru = lru_init(n_way);

        // Initialize cache entries
        for (int j = 0; j < n_way; j++) {
            set[i].ptr[j] = &way[j].tbl[i];
            set[i].ptr[j]->is_valid = false;
            set[i].ptr[j]->lru_ptr =
                    lru_push(set[i].lru, (void *)set[i].ptr[j]);
        }
    }
}

pcc::~pcc() {
    for (int i = 0; i < n_way; i++) {
        delete [] way[i].tbl;
    }
    for (int i = 0; i < n_set; i++) {
        lru_free(set[i].lru);
    }

    delete [] way;
    delete [] set;
}

static inline uint64_t get_index(pcc *cache, uint64_t addr) {
    return ( ( addr & cache->index_mask ) >> cache->b_offset );
}

static inline uint64_t get_tag(pcc *cache, uint64_t addr) {
    return ( ( addr & cache->tag_mask ) >> ( 64 - cache->b_tag ) );
}

static void check_cache(pcc *cache, struct cache_entry **target, struct cache_set *set, uint64_t tag, bool *is_hit, bool *is_full) {
    struct cache_entry *iter;

    // Check cache hit or miss
    for (int i = cache->n_way-1; i >= 0; i--) {
        iter = set->ptr[i];

        if (!iter->is_valid) {
            *is_full = false;
            *target  = iter;
            continue;
        }

        if (tag == iter->tag) {
            *is_hit = true;
            *target = iter;
            break;
        }
    }
}

static void evict_victim(pcc *cache, struct cache_entry **target, struct cache_set *set) {
    *target = (cache_entry *)lru_get_victim(set->lru);
    if ((*target)->is_dirty) {
        ++cache->dirty_evict;

        // Write-back
        // target->data => memory
    } else {
        ++cache->clean_evict;
    }
}

static void read_data(struct cache_entry **target, uint64_t tag) {
    (*target)->tag      = tag;
    (*target)->is_valid = true;
    (*target)->is_dirty = false;

    // (*target)->data <= memory
}

int pcc::read(uint64_t addr) {
    struct cache_set *set;
    uint64_t tag;

    struct cache_entry *target;

    bool is_hit  = false;
    bool is_full = true;

    set = &set[get_index(this, addr)];
    tag = get_tag(this, addr);

    ++read_cnt;

    check_cache(this, &target, set, tag, &is_hit, &is_full);

    if (!is_hit) {
        ++read_miss;
        if (is_full) {
            evict_victim(this, &target, set);
        }
        read_data(&target, tag);
    }

    // Update
    lru_update(set->lru, target->lru_ptr);

    return 0;
}

int pcc::write(uint64_t addr) {
    struct cache_set *set;
    uint64_t tag;

    struct cache_entry *target;

    bool is_hit  = false;
    bool is_full = true;

    set = &set[get_index(this, addr)];
    tag = get_tag(this, addr);

    ++write_cnt;

    check_cache(this, &target, set, tag, &is_hit, &is_full);

    if (!is_hit) {
        ++write_miss;
        if (is_full) {
            evict_victim(this, &target, set);
        }
        read_data(&target, tag);
    }

    // Update
    lru_update(set->lru, target->lru_ptr);

    // Mark as dirty
    target->is_dirty = true;

    return 0;
}

int pcc::mark_shared(uint64_t addr) {
    struct cache_set *set;
    uint64_t tag;

    struct cache_entry *target;

    bool is_hit  = false;
    bool is_full = true;

    set = &set[get_index(this, addr)];
    tag = get_tag(this, addr);

    check_cache(this, &target, set, tag, &is_hit, &is_full);

    if (!is_hit) return 1;

    target->is_shared = true;

    return 0;
}

int pcc::mark_invalid(uint64_t addr) {
    struct cache_set *set;
    uint64_t tag;

    struct cache_entry *target;

    bool is_hit  = false;
    bool is_full = true;

    set = &set[get_index(this, addr)];
    tag = get_tag(this, addr);

    check_cache(this, &target, set, tag, &is_hit, &is_full);

    if (!is_hit) return 1;

    target->is_invalid = true;
    lru_update_inverse(set->lru, target->lru_ptr);
    
    return 0;
}
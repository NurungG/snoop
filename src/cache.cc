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

ppc::ppc(int _cap, int _ways, int _block_sz) : cap(_cap), n_way(_ways), block_sz(_block_sz) {
    this->cap <<= 10; // scale up

    this->n_entry = this->cap / this->block_sz;
    this->n_set   = this->n_entry / this->n_way;

    this->b_offset = num_of_bits(this->block_sz);
    this->b_index  = num_of_bits(this->cap / this->n_way / this->block_sz);
    this->b_tag    = 64 - this->b_offset - this->b_offset;

    this->offset_mask = UINT64_MAX >> (64 - this->b_offset);
    this->tag_mask    = UINT64_MAX << (64 - this->b_tag);
    this->index_mask  = ( UINT64_MAX ^ this->offset_mask ) & ( UINT64_MAX ^ this->tag_mask);

    // Allocate cache
    this->way = new struct cache_way [this->n_way];
    this->set = new struct cache_set [this->n_set];

    for (int i = 0; i < this->n_way; i++) {
        this->way[i].tbl = new struct cache_entry [this->n_set];
    }
    for (int i = 0; i < this->n_set; i++) {
        this->set[i].ptr = new struct cache_entry * [this->n_way];
        this->set[i].lru = lru_init(this->n_way);

        // Initialize cache entries
        for (int j = 0; j < this->n_way; j++) {
            this->set[i].ptr[j] = &this->way[j].tbl[i];
            this->set[i].ptr[j]->is_valid = false;
            this->set[i].ptr[j]->lru_ptr =
                    lru_push(this->set[i].lru, (void *)this->set[i].ptr[j]);
        }
    }
}

ppc::~ppc() {
    for (int i = 0; i < this->n_way; i++) {
        delete [] this->way[i].tbl;
    }
    for (int i = 0; i < this->n_set; i++) {
        lru_free(this->set[i].lru);
    }

    delete [] this->way;
    delete [] this->set;
}

static inline uint64_t get_index(ppc *cache, uint64_t addr) {
    return ( ( addr & cache->index_mask ) >> cache->b_offset );
}

static inline uint64_t get_tag(ppc *cache, uint64_t addr) {
    return ( ( addr & cache->tag_mask ) >> ( 64 - cache->b_tag ) );
}

static void check_cache(ppc *cache, struct cache_entry **target, struct cache_set *set, uint64_t tag, bool *is_hit, bool *is_full) {
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

static void evict_victim(ppc *cache, struct cache_entry **target, struct cache_set *set) {
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

int ppc::read(uint64_t addr) {
    struct cache_set *set;
    uint64_t tag;

    struct cache_entry *target;

    bool is_hit  = false;
    bool is_full = true;

    set = &this->set[get_index(this, addr)];
    tag = get_tag(this, addr);

    ++this->read_cnt;

    check_cache(this, &target, set, tag, &is_hit, &is_full);

    if (!is_hit) {
        ++this->read_miss;
        if (is_full) {
            evict_victim(this, &target, set);
        }
        read_data(&target, tag);
    }

    // Update
    lru_update(set->lru, target->lru_ptr);

    return 0;
}

int ppc::write(uint64_t addr) {
    struct cache_set *set;
    uint64_t tag;

    struct cache_entry *target;

    bool is_hit  = false;
    bool is_full = true;

    set = &this->set[get_index(this, addr)];
    tag = get_tag(this, addr);

    ++this->write_cnt;

    check_cache(this, &target, set, tag, &is_hit, &is_full);

    if (!is_hit) {
        ++this->write_miss;
        if (is_full) {
            evict_victim(this, &target, set);
        }
        read_data(&target, tag);
    }

    // Update
    lru_update(set->lru, target->lru_ptr);

    // Mark as dirty
    target->is_dirty = true;

    return 1;
}

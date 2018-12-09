#ifndef __CACHE_H__
#define __CACHE_H__

#include <cstdint>

#include "lru.h"


struct cache_entry {
    uint64_t tag;
    bool is_valid;
    bool is_dirty;

    //char data[BLOCK_SIZE];

    struct lru_node *lru_ptr;
};

struct cache_way {
    struct cache_entry *tbl;
};

struct cache_set {
    struct cache_entry **ptr;
    struct lru_list *lru;
};

/** Per Core Cache */
class ppc {
public:
    /** Cache management core */
    struct cache_way *way;
    struct cache_set *set;

    /** Configuration variables */
    int cap;
    int n_way;
    int block_sz;

    int n_entry;
    int n_set;

    int b_offset;
    int b_index;
    int b_tag;

    uint64_t offset_mask;
    uint64_t index_mask;
    uint64_t tag_mask;


    /** Statistic variables */
    uint64_t total_cnt;
    uint64_t read_cnt;
    uint64_t write_cnt;

    uint64_t read_miss;
    uint64_t write_miss;

    uint64_t clean_evict;
    uint64_t dirty_evict;

    uint64_t checksum;

    ppc(int, int, int);
    ~ppc();
    int read(uint64_t addr);
    int write(uint64_t addr);
};

#endif

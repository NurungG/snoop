#ifndef __SNOOP_H__
#define __SNOOP_H__

#include "cache.h"
#include <queue>

struct bus_arg {
    int bus_user;
    int bus_type;
    uint64_t bus_addr;
}

class msi {
public:
    pcc *core;
    queue<pair<int, int> bus_q;
    bool *is_busy;

    int n_core;
    int cap;
    int n_way;
    int block_sz;

    int bus_wait_time;
    int bus_wait_max;

    int bus_user;
    int bus_type;
    uint64_t bus_addr;

    msi(int, int, int, int);
    ~msi();
    int read(int, uint64_t);
    int write(int, uint64_t);
    int check_bus();
};

class mesi {
public:

};

#endif

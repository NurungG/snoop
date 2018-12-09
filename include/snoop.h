#ifndef __SNOOP_H__
#define __SNOOP_H__

#include "cache.h"
#include <queue>

class msi {
public:
    ppc *core;
    queue<int> bus_q;

    int n_core;
    int cap;
    int n_way;
    int block_sz;

    int bus_wait_time;

    int bus_user;

    msi(int, int, int, int);
    ~msi();
    int read(int, uint64_t);
    int write(int, uint64_t);
};

class mesi {
public:

}

#endif

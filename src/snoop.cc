#include "snoop.h"

msi::msi(int _nc, int _cap, int _way, int _blk) : n_core(_nc), cap(_cap), n_way(_way), block_sz(_blk) {
    core = new pcc [n_core];
    is_busy = new bool [n_core];
    for (int i = 0; i < n_core; i++) {
        core[i] = pcc(cap, n_way, block_sz);
        is_busy[i] = false;
    }

    bus_wait_max = (n_core - 1) * 2;
}

msi::~msi() {
    for (int i = 0; i < n_core; i++) {
        core[i].~pcc();
    }
    delete [] core;
    delete [] is_busy;
}

int msi::read(int core_idx, uint64_t addr) {
    int rc;

    rc = core[core_idx].read(addr);
    if (rc) {
        if (bus_user == -1) {
            bus_user = core_idx;
            bus_type = 0;
            bus_addr = addr;
        } else {
            bus_q.push((struct bus_arg){ core_idx, 0, addr });
        }
        is_busy[core_idx] = true;
    }

    return 0;
}

int msi::write(int core_idx, uint64_t addr) {
    int rc;

    rc = core[core_idx].write(addr);
    if (rc) {
        if (bus_user == -1) {
            bus_user = core_idx;
            bus_type = 1;
            bus_addr = addr;
        } else {
            bus_q.push((struct bus_arg){ core_idx, 1, addr });
        }
        is_busy[core_idx] = true;
    }

    return 0;
}

int msi::check_bus() {
    if (bus_user != -1) {
        bus_wait_time++;
        if (bus_wait_time == bus_wait_max) {
            is_busy[bus_user] = false;
            if (bus_type == 0) { // read
                for (int i = 0; i < n_core; i++) {
                    if (i != bus_user) {
                        core[i].mark_shared(addr);
                    }
                }
            } else if (bus_type == 1) { // write
                for (int i = 0; i < n_core; i++) {
                    if (i != bus_user) {
                        core[i].mark_invalid(addr);
                    }
                }
            }
            
            if (!bus_q.empty()) {
                bus_user = bus_q.front().bus_user;
                bus_type = bus_q.front().bus_type;
                bus_addr = bus_q.front().bus_addr;
                bus_q.pop();
            } else {
                bus_user = -1;
            }
            bus_wait_time = 0;
        }
    }
}

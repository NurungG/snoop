#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "snoop.h"
using namespace std;

#define DEBUG 1

struct snoop_ctx {
    int cores;
    int prtcl;
    int cap;
    int ways;

    std::ifstream *core;
};

static int parse_arg(int argc, char *argv[], struct snoop_ctx *ctx) {
    if (argc < 5) return 1;

    string input;
    for (int i = 1; i < argc; i++) {
        input += argv[i];
        input += ' ';
    }
    istringstream iss(input);
    if (!(iss >> ctx->cores >> ctx->prtcl >> ctx->cap >> ctx->ways)) {
        return 1;
    }

#if DEBUG
    cout << ctx->cores << endl
        << ctx->prtcl << endl
        << ctx->cap << endl
        << ctx->ways << endl;
#endif
    return 0;
}

static int open_trace(struct snoop_ctx *ctx) {
    ifstream *ifs = new ifstream[ctx->cores];

    for (int i = 0; i < ctx->cores; i++) {
        stringstream ss;
        ss << "trace/core_" << i << "_" << ctx->cores << ".out";
        string file = ss.str();
#if DEBUG
        cout << file << " is openning" << endl;
#endif
        ifs[i].open(file, ifstream::in);

        if (!ifs[i].is_open()) return 1;
    }

    ctx->core = ifs;

    return 0;
}

static int do_simulate(struct snoop_ctx *ctx) {
    if (ctx->prtcl == 0) {
        msi simulator(ctx->cores, ctx->cap, ctx->ways, 64);

        for (int i = 0; i < simulator.n_core; i++) {
            if (!simulator.is_busy && ctx->core[i] >> op >> hex >> addr >> dec) {
                if (op == 'R') simulator.read(i, addr);
                else if (op == 'W') simulator.write(i, addr);
            }
        }
    } else if (ctx->prtcl == 1) {
        //mesi simualtor(ctx->cores, ctx->cap, ctx->ways, 64);
    } else {

    }
    return 0;
}

static int clean_up(struct snoop_ctx *ctx) {
    for (int i = 0; i < ctx->cores; i++) {
#if DEBUG
        cout << "core " << i << " trace file is closing" << endl;
#endif
        ctx->core[i].close();
    }
    delete [] ctx->core;

    return 0;
}

int main(int argc, char *argv[]) {
    int rc;
    struct snoop_ctx ctx;

    rc = parse_arg(argc, argv, &ctx);
    if (rc) {
        cerr << "[ERROR] Bad Input Format" << endl
             << "Usage:   " << argv[0]
             << " <cores> <protocol> <capacity> <associativity>" << endl
             << "Example: " << argv[0] << " 4 0 128 8\n" << endl;
        return 1;
    }

    rc = open_trace(&ctx);
    if (rc) {
        cerr << "[ERROR] Cannot open trace files" << endl;
        return 1;
    }

    rc = do_simulate(&ctx);
    if (rc) {
        cerr << "[ERROR] Simulation failed" << endl;
        return 1;
    }

    rc = clean_up(&ctx);
    if (rc) {
        cerr << "[ERROR] Fail to clean up" << endl;
        return 1;
    }

    return 0;
}

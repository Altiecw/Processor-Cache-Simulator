#ifndef CACHESIMULATOR_H
#define CACHESIMULATOR_H

#include "Cache.h"

class CacheSimulator
{
  public:
    int Rep;
    int Inc;
    Cache l1, l2;
    bool l1Only = true;
    std::string Trace;

    CacheSimulator(int blocksize, int l1size, int l1assoc, int l2size, int l2assoc, int rep, int inc);

    static void EvaluateTrace(std::string name);

    static void GenerateTrace(std::string name, int size, int seed, float refChance = 0.5f, float writeChance = 0.5f);

    void Run(std::string trace);

    void Output(bool showsets, bool print = false, std::string file = "Results.txt");
};

#endif

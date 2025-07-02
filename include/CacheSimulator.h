#ifndef CACHESIMULATOR_H
#define CACHESIMULATOR_H

#include "Cache.h"

class CacheSimulator
{
  private:
    std::vector<Cache*> Caches;
  public:
    int Rep;
    int Inc;
    std::string Trace;


    CacheSimulator(int blocksize, std::vector<int> cacheData, int rep, int inc);

    static void EvaluateTrace(std::string name);

    static void GenerateTrace(std::string name, int size, int seed, float refChance = 0.5f, float writeChance = 0.5f);

    void Run(std::string trace);

    void Output(bool showsets, bool print = false, std::string file = "Results.txt");
};

#endif

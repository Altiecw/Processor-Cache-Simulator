#ifndef CACHE_H
#define CACHE_H

#include <bitset>
#include <queue>
#include <string>
#include <tuple>
#include <vector>

class Cache
{
  public:
    int Size;
    int Blocksize;
    int Assoc;
    int Level;
    int Rep;
    int Inc;
    int s_Tag;
    int s_Index;
    int s_Offset;
    int backInvals = 0;
    int reads = 0;
    int read_misses = 0;
    int writes = 0;
    int write_misses = 0;
    int write_backs = 0;
    bool l1Only;
    std::vector<int> LRUTickers;
    std::vector<std::queue<int>> Fifo;
    Cache *Higher;
    Cache *Lower;
    std::vector<std::vector<std::string>> Table;
    std::vector<std::vector<int>> LRUage;
    std::vector<std::vector<bool>> Dirty;
    std::vector<std::vector<std::string>> OPT;

    Cache()
    {
    }

    Cache(int size, int blocksize, int assoc, int level, int rep, int inc);

    std::tuple<std::string, int, int> Decode(std::string address);

    std::string Read(std::string address);

    std::string Write(std::string address);

    void Evict(std::string address);

    void Back_Evict(std::string address);

    float MissRate()
    {
        return ((float)read_misses + (float)write_misses) / ((float)reads + (float)writes);
    }

    int Replace(int Index);

    void PreCompile(std::vector<std::string> trace);

    void SetLower(Cache *lower)
    {
        Lower = lower;
    }

    void SetHigher(Cache *higher)
    {
        Higher = higher;
    }

    std::string ToTag(int index, int slot);

    int Sets()
    {
        return Size / (Assoc * Blocksize);
    }

    void Update(std::string address, int change);
};

#endif

#include "Cache.h"
#include <cmath>
#include <sstream>

Cache::Cache(int size, int blocksize, int assoc, int level, int rep, int inc)
{
    Size = size;
    if (std::ceil(log2(blocksize)) == floor(log2(blocksize)))
    {
        Blocksize = blocksize;
    }
    else
    {
        // throw invalid_argument("Blocksize must be a power of 2");
    }
    Assoc = assoc;
    Level = level;
    Rep = rep;
    Inc = inc;

    int sets = Sets();
    s_Index = log2(sets);
    s_Offset = log2(blocksize);
    s_Tag = 32 - s_Index - s_Offset;

    Table = std::vector<std::vector<std::string>>(Sets());
    Dirty = std::vector<std::vector<bool>>(Sets());
    LRUage = std::vector<std::vector<int>>(Sets());
    LRUTickers = std::vector<int>(Sets());
    Fifo = std::vector<std::queue<int>>(Sets());
    OPT = std::vector<std::vector<std::string>>(Sets());

    for (int i = 0; i < Sets(); i++)
    {
        std::vector<std::string> col(Assoc);
        Table[i] = col;
        std::vector<bool> col1(Assoc);
        Dirty[i] = col1;
        std::vector<int> col2(Assoc);
        LRUage[i] = col2;
        std::queue<int> col3;
        Fifo[i] = col3;
        std::vector<std::string> col4;
        OPT[i] = col4;
    }
}

std::tuple<std::string, int, int> Cache::Decode(std::string address)
{
    if (address.length() > 8)
    {
        // Insert enough 0s
        int fill = 8 - address.length();
        for (int i = 0; i < fill; i++)
        {
            address.insert(0, "0");
        }
    }

    // Convert hex to binary, use to find tag, index, and offset
    std::string AddBin;
    std::string r_Tag;
    int r_Index;
    int r_Offset;
    unsigned bits;
    std::stringstream s;
    s << std::hex << address;
    s >> bits;
    std::bitset<32> b(bits);
    AddBin = b.to_string();

    // Index is first element of remaining hex, offset is last
    r_Tag = AddBin.substr(0, s_Tag);
    if (s_Index > 0)
    {
        r_Index = stoi(AddBin.substr(s_Tag, s_Index), 0, 2);
    }
    else
    {
        r_Index = 0;
    }
    r_Offset = stoi(AddBin.substr(s_Tag + s_Index), 0, 2);

    return {r_Tag, r_Index, r_Offset};
}

std::string Cache::Read(std::string address)
{
    reads += 1;
    // Convert Input
    std::tuple<std::string, int, int> Decoded = Decode(address);
    std::string Tag = std::get<0>(Decoded);
    int Index = std::get<1>(Decoded);
    int Offset = std::get<2>(Decoded);

    // See if tag is at index
    bool Hit = false;
    for (int i = 0; i < Assoc; i++)
    {
        Decoded = Decode(Table[Index][i]);
        if (std::get<0>(Decoded) == Tag)
        {
            // Item found, return it
            Hit = true;
            LRUage[Index][i] = LRUTickers[Index];
            LRUTickers[Index] += 1;
            if (Rep == 2)
            {
                OPT[Index].erase(OPT[Index].begin());
            }
            return address;
        }
    }

    // There was no hit; Time to retrieve
    read_misses += 1;
    // First, find which block needs to be replaced
    // Check if there's an open slot
    int slot = -1;
    for (int i = 0; i < Assoc; i++)
    {
        if (Table[Index][i].empty())
        {
            slot = i;
            break;
        }
    }

    if (slot < 0)
    {
        // Find slot via replacement function
        slot = Replace(Index);
    }

    std::string gone = Table[Index][slot];
    Evict(Table[Index][slot]);
    Update(gone, 0);

    // Get block from lower cache
    std::string block;
    if (Level == 1)
    {
        if (!l1Only)
        {
            block = (*Lower).Read(address);
        }
        else
        {
            block = address;
        }
    }
    else if (Level == 2)
    {
        // Do nothing (getting from main memory)
        block = address;
    }
    // Block was put into cache, update FIFO queue
    Fifo[Index].push(slot);
    if (Rep == 2)
    {
        OPT[Index].erase(OPT[Index].begin());
    }
    LRUage[Index][slot] = LRUTickers[Index];
    LRUTickers[Index] += 1;
    Table[Index][slot] = block;

    return block;
}

std::string Cache::Write(std::string address)
{
    /// Write to Cache at address
    // Convert Input
    writes += 1;
    std::tuple<std::string, int, int> Decoded = Decode(address);
    std::string Tag = std::get<0>(Decoded);
    int Index = std::get<1>(Decoded);
    int Offset = std::get<2>(Decoded);

    // Check if block is there
    for (int i = 0; i < Assoc; i++)
    {
        Decoded = Decode(Table[Index][i]);
        if (std::get<0>(Decoded) == Tag)
        {
            // Item found, return it and mark it as dirty
            LRUage[Index][i] = LRUTickers[Index];
            LRUTickers[Index] += 1;
            Dirty[Index][i] = true;
            if (Rep == 2)
            {
                OPT[Index].erase(OPT[Index].begin());
            }
            return address;
        }
    }
    // There was no hit; Time to retrieve
    write_misses += 1;
    // First, find which block needs to be replaced
    // Check if there's an open slot
    int slot = -1;
    for (int i = 0; i < Assoc; i++)
    {
        if (Table[Index][i].empty())
        {
            slot = i;
            break;
        }
    }

    if (slot < 0)
    {
        // Find slot via replacement function
        slot = Replace(Index);
    }

    std::string gone = Table[Index][slot];
    Evict(Table[Index][slot]);
    Update(gone, 0);

    // Get block from lower cache
    std::string block;
    if (Level == 1)
    {
        if (!l1Only)
        {
            block = (*Lower).Read(address);
        }
        else
        {
            // Do nothing (getting from main memory)
            block = address;
        }
    }
    else if (Level == 2)
    {
        // Do nothing (getting from main memory)
        block = address;
    }

    // Put address into slot; Mark it as Dirty
    Fifo[Index].push(slot);
    if (Rep == 2)
    {
        OPT[Index].erase(OPT[Index].begin());
    }
    LRUage[Index][slot] = LRUTickers[Index];
    LRUTickers[Index] += 1;
    Table[Index][slot] = block;
    Dirty[Index][slot] = true;
    return block;
}

void Cache::Evict(std::string address)
{
    // Remove Address from Cache, if in cache
    std::tuple<std::string, int, int> Decoded = Decode(address);
    std::string Tag = std::get<0>(Decoded);
    int Index = std::get<1>(Decoded);
    int Offset = std::get<2>(Decoded);

    // Check if block is there
    for (int i = 0; i < Assoc; i++)
    {
        Decoded = Decode(Table[Index][i]);
        if (std::get<0>(Decoded) == Tag)
        {
            // Item found, evict it
            // if block is written to, write it to the next level
            if (Dirty[Index][i])
            {
                // DO NOTHING (writing to main memory)
                write_backs += 1;
                if (Level < 2 && !l1Only)
                {
                    (*Lower).Write(Table[Index][i]);
                }
                Dirty[Index][i] = false;
            }

            Table[Index][i] = "";
            return;
        }
    }

    // Address wasn't found; not in table;
    return;
}

void Cache::Back_Evict(std::string address)
{
    if (address == "")
    {
        return;
    }

    // Remove Address from Cache, if in cache
    std::tuple<std::string, int, int> Decoded = Decode(address);
    std::string Tag = std::get<0>(Decoded);
    int Index = std::get<1>(Decoded);
    int Offset = std::get<2>(Decoded);

    // Check if block is there
    for (int i = 0; i < Assoc; i++)
    {
        Decoded = Decode(Table[Index][i]);
        if (std::get<0>(Decoded) == Tag)
        {
            // Item found, evict it
            // if block is written to, write it to the next level
            if (Dirty[Index][i])
            {
                // DO NOTHING (writing to main memory)
                write_backs += 1;
                Dirty[Index][i] = false;
            }

            Table[Index][i] = "";
            return;
        }
    }

    // Address wasn't found; not in table;
    return;
}

int Cache::Replace(int Index)
{
    /// Returns what block should be replaced
    // Check which Replacement policy is there
    if (Rep == 0)
    {
        // LRU
        int smallest = 0;
        for (int i = 0; i < Assoc; i++)
        {
            if (LRUage[Index][i] < LRUage[Index][smallest])
            {
                smallest = i;
            }
        }

        // Reduce Ticker and all ages in set
        for (int i = 0; i < Assoc; i++)
        {
            LRUage[Index][i] -= smallest;
        }
        LRUTickers[Index] -= smallest;

        return smallest;
    }
    else if (Rep == 1)
    {
        // Fifo
        int returned = Fifo[Index].front();
        Fifo[Index].pop();
        return returned;
    }
    else if (Rep == 2)
    {
        // Optimal
        // Look into the future addresses in this set, find the one furthest in the vector
        std::vector<std::string>::iterator it;
        int largest = 0;
        int largest_i = 0;
        for (int i = 0; i < Assoc; i++)
        {
            std::tuple<std::string, int, int> Decoded = Decode(Table[Index][i]);
            it = std::find(OPT[Index].begin(), OPT[Index].end(), std::get<0>(Decoded));
            if (it == OPT[Index].end())
            {
                return i;
            }

            int place = it - OPT[Index].begin();
            if (place > largest)
            {
                largest = place;
                largest_i = i;
            }
        }

        return largest_i;
    }
}

void Cache::PreCompile(std::vector<std::string> trace)
{
    /// Pre-compile trace for OPT
    for (int i = 0; i < trace.size(); i++)
    {
        std::tuple<std::string, int, int> Decoded = Decode(trace[i]);
        std::string Tag = std::get<0>(Decoded);
        int Index = std::get<1>(Decoded);
        int Offset = std::get<2>(Decoded);

        OPT[Index].push_back(Tag);
    }
}

std::string Cache::ToTag(int index, int slot)
{
    // Return Tag as hexadecimal
    if (Table[index][slot] == "")
    {
        return "";
    }
    std::tuple<std::string, int, int> Decoded = Decode(Table[index][slot]);
    std::string Tag = std::get<0>(Decoded);
    int Index = std::get<1>(Decoded);
    int Offset = std::get<2>(Decoded);

    std::bitset<32> b(Tag);
    unsigned n = b.to_ulong();
    std::stringstream s;
    s << std::hex << n;
    return s.str();
}

void Cache::Update(std::string address, int change)
{
    // Affect either higher or lower cache depending on Replacement policy
    // Change: 0 = deleting address, 1 = inserting address
    if (Inc == 0)
    {
        // Non-Inclusive; Do nothing since reads in l1 automatically require l2 to have same address
    }
    else if (Inc == 1)
    {
        // Inclusive
        if (change == 0)
        {
            // if at l2, remove address from l1 as well
            if (Level == 2)
            {
                (*Higher).Back_Evict(address);
                (*Higher).Update(address, 0);
                backInvals += 1;
            }
        }
    }
}

#include "CacheSimulator.h"
#include <vector>
#include <random>

CacheSimulator::CacheSimulator(int blocksize, int l1size, int l1assoc, int l2size, int l2assoc, int rep, int inc)
{
    l1 = Cache(l1size, blocksize, l1assoc, 1, rep, inc);
    if (l2size > 0)
    {
        l2 = Cache(l2size, blocksize, l2assoc, 2, rep, inc);
        l1.SetLower(&l2);
        l2.SetHigher(&l1);
        l1Only = false;
    }
    l1.l1Only = l1Only;

    Rep = rep;
    Inc = inc;
}

void CacheSimulator::EvaluateTrace(std::string name) 
{
    std::fstream source("traces/" + name);
    if (!source.is_open()) {
        return;
    }

    std::vector<std::string> uniques;
    std::string line;
    int count = 0;
    int count_r = 0;
    int count_w = 0;
    while (getline(source, line))
    {
        if (std::find(uniques.begin(), uniques.end(), line.substr(2, line.size() - 2)) == uniques.end()) uniques.push_back(line.substr(2, line.size() - 2));
        if (line.substr(0, 1) == "w") {
            ++count_w;
        }
        else {
            ++count_r;
        }
        ++count;
    }
    std::cout << "Trace: " + name + '\n';
    std::cout << "Size: " << count << " lines\n";
    std::cout << "Unique Lines: " << uniques.size() << " (" << (double)uniques.size() / (double)count * 100.0 << "%)\n";
    std::cout << "Reads: " << count_r << " (" << (double)count_r / (double)count * 100.0 << "%)\n";
    std::cout << "Writes: " << count_w << " (" << (double)count_w / (double)count * 100.0 << "%)\n";
}

void CacheSimulator::GenerateTrace(std::string name, int size, int seed, float refChance, float writeChance)
{
    std::ofstream file("traces/" + name);

    if (!file.is_open()) {
        return;
    }

    std::vector<unsigned long int> priors;
    std::random_device device;
    std::default_random_engine generator(seed);
    for (int i = 1; i <= size; ++i) {
        const char rw = generator() % 100 > writeChance * 100.0f ? 'r' : 'w';
        unsigned long int address;
        if ((float)priors.size() / (float)(i) < 1.0 - refChance || priors.size() == 0) {
            if (generator() % 100 > refChance * 100.0f || priors.size() == 0) {
                address = generator();
                priors.push_back(address);
            }
            else {
                address = priors[rand() % priors.size()];
            }
        }
        else {
            address = priors[rand() % priors.size()];
        }
        file << rw << ' ' << std::hex << address << '\n';
    }
    file.close();
    std::cout << "Created new trace file at traces/" + name + '\n';
}

void CacheSimulator::Run(std::string trace)
{
    // Load file to get commands from
    Trace = trace;
    std::fstream source;
    source.open("traces/" + trace);
    if (!source.is_open())
    {
        throw std::invalid_argument(
            "Couldn't find trace files. Make sure .exe is in same directory as the folder 'traces'.");
    }

    // Do OPT Pre-processing if needed
    if (Rep == 2)
    {
        std::vector<std::string> trace;
        std::string s;
        while (getline(source, s))
        {
            trace.push_back(s.substr(2));
        }

        l1.PreCompile(trace);
        if (!l1Only)
        {
            l2.PreCompile(trace);
        }
    }
    source.close();
    source.open("traces/" + trace);

    // Go through trace line by line
    std::string line;
    int count = 1;
    while (getline(source, line))
    {
        // cout << "Reading Line " + to_string(count) << endl;
        //  Check if this is either a read or a write; Give l1 Read or write command
        if (line.substr(0, 1) == "r")
        {
            std::string Report = l1.Read(line.substr(2));
        }
        else if (line.substr(0, 1) == "w")
        {
            std::string Report = l1.Write(line.substr(2));
        }
        count += 1;
    }

    source.close();
}

void CacheSimulator::Output(bool showsets)
{
    std::cout << "===== Simulator configuration =====" << std::endl;
    std::cout << "BLOCKSIZE:             " + std::to_string(l1.Blocksize) << std::endl;
    std::cout << "L1_SIZE:               " + std::to_string(l1.Size) << std::endl;
    std::cout << "L1_ASSOC:              " + std::to_string(l1.Assoc) << std::endl;
    std::cout << "L2_SIZE:               " + std::to_string(l2.Size) << std::endl;
    std::cout << "L2_ASSOC:              " + std::to_string(l2.Assoc) << std::endl;
    if (Rep == 0)
    {
        std::cout << "REPLACEMENT POLICY:    LRU" << std::endl;
    }
    else if (Rep == 1)
    {
        std::cout << "REPLACEMENT POLICY:    FIFO" << std::endl;
    }
    else if (Rep == 2)
    {
        std::cout << "REPLACEMENT POLICY:    Optimal" << std::endl;
    }
    if (Inc == 0)
    {
        std::cout << "INCLUSION PROPERTY:    non-inclusive" << std::endl;
    }
    else if (Inc == 1)
    {
        std::cout << "INCLUSION PROPERTY:    inclusive" << std::endl;
    }
    std::cout << "trace_file:            " + Trace << std::endl;
    if (showsets)
    {
        std::cout << "===== L1 contents =====" << std::endl;
        for (int i = 0; i < l1.Sets(); i++)
        {
            std::string line = "Set\t" + std::to_string(i) + ":\t";
            for (int j = 0; j < l1.Assoc; j++)
            {
                if (l1.Dirty[i][j])
                {
                    line += l1.ToTag(i, j) + " D   ";
                }
                else
                {
                    line += l1.ToTag(i, j) + "     ";
                }
            }
            std::cout << line << std::endl;
        }
        if (!l1Only)
        {
            std::cout << "===== L2 contents =====" << std::endl;
            for (int i = 0; i < l2.Sets(); i++)
            {
                std::string line = "Set\t" + std::to_string(i) + ":\t";
                for (int j = 0; j < l2.Assoc; j++)
                {
                    if (l2.Dirty[i][j])
                    {
                        line += l2.ToTag(i, j) + " D   ";
                    }
                    else
                    {
                        line += l2.ToTag(i, j) + "     ";
                    }
                }
                std::cout << line << std::endl;
            }
        }
    }
    std::cout << "===== Simulation results(raw) =====" << std::endl;
    std::cout << "a. number of L1 reads:        " + std::to_string(l1.reads) << std::endl;
    std::cout << "b. number of L1 read misses:  " + std::to_string(l1.read_misses) << std::endl;
    std::cout << "c. number of L1 writes:       " + std::to_string(l1.writes) << std::endl;
    std::cout << "d. number of L1 write misses: " + std::to_string(l1.write_misses) << std::endl;
    std::cout << "e. L1 miss rate:              " + std::to_string(l1.MissRate()) << std::endl;
    std::cout << "f. number of L1 writebacks:   " + std::to_string(l1.write_backs) << std::endl;
    std::cout << "g. number of L2 reads:        " + std::to_string(l2.reads) << std::endl;
    std::cout << "h. number of L2 read misses:  " + std::to_string(l2.read_misses) << std::endl;
    std::cout << "i. number of L2 writes:       " + std::to_string(l2.writes) << std::endl;
    std::cout << "j. number of L2 write misses: " + std::to_string(l2.write_misses) << std::endl;
    if (!l1Only)
    {
        std::cout << "k. L2 miss rate:              " + std::to_string((float)l2.read_misses / (float)l2.reads)
                  << std::endl;
    }
    else
    {
        std::cout << "k. L2 miss rate:              0" << std::endl;
    }
    std::cout << "l. number of L2 writebacks:   " + std::to_string(l2.write_backs) << std::endl;
    if (!l1Only)
    {
        if (Inc == 0)
        {
            std::cout << "m. total memory traffic:      " +
                             std::to_string(l2.read_misses + l2.write_misses + l2.write_backs)
                      << std::endl;
        }
        else if (Inc == 1)
        {
            std::cout << "m. total memory traffic:      " +
                             std::to_string(l2.read_misses + l2.write_misses + l2.write_backs)
                      << std::endl;
        }
        // cout << "Back Invals:                  " + to_string(l2.backInvals) << endl;
    }
    else
    {
        std::cout << "m. total memory traffic:      " +
                         std::to_string(l1.read_misses + l1.write_misses + l1.write_backs)
                  << std::endl;
    }
}

#include "CacheSimulator.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <unordered_map>
#include <vector>


std::chrono::time_point<std::chrono::system_clock> begin, end;

static unsigned int TriangularDistribution(const int a, const int b, const int c, std::default_random_engine* gen) {
    int x = (*gen)();

    if (x < c) {
        x = (x - a) / (c - a);
    }
    else if (x > c) {
        x = (b - x) / (b - c + 1);
    }

    return x * 2 / (b - a);
}

CacheSimulator::CacheSimulator(int blocksize, std::vector<int> cacheData, int rep, int inc) {
    
    for (int i = 0; i < cacheData.size(); i += 2) {
        Caches.push_back(new Cache(cacheData[i], blocksize, cacheData[i+1], i / 2 + 1, rep, inc));
    }

    for (int i = 0; i < Caches.size(); ++i) {
        if (i > 0) Caches[i]->SetHigher(Caches[i - 1]);
        if (i < Caches.size() - 1) Caches[i]->SetLower(Caches[i + 1]);
    }

    Caches[0]->l1Only = Caches.size() == 1;

    Rep = rep;
    Inc = inc;
}

void CacheSimulator::EvaluateTrace(std::string name) 
{
    std::fstream source("traces/" + name);
    if (!source.is_open()) {
        return;
    }

    std::vector<std::string> addresses;
    std::string line;
    int count = 0;
    int progress = 0;
    int count_r = 0;
    int count_w = 0;
    long int distance_sum = 0;
    std::vector<long int> distances;
    std::unordered_map<long int, int> distance_frequency = { {0,0} };
    long int distances_mode = 0;
    long int distances_median;

    int MaxDistance = 0;
    double MeanDistance;


    while (getline(source, line)) {
        ++count;
    }

    source.clear();
    source.seekg(0);

    while (getline(source, line))
    {
        auto last = std::find(addresses.rbegin(), addresses.rend(), line.substr(2, line.size() - 2));
        if (last != addresses.rend()) {
            // Item is in addresses
            distances.push_back(-(addresses.rbegin() - last - 1));
            distance_sum += distances.back();
            if (distances.back() > MaxDistance) MaxDistance = distances.back();
            ++distance_frequency[distances.back()];
            if (distance_frequency[distances.back()] > distance_frequency[0]) distances_mode = distances.back();
        }
        addresses.push_back(line.substr(2, line.size() - 2));
        if (line.substr(0, 1) == "w") {
            ++count_w;
        }
        else {
            ++count_r;
        }
        ++progress;

        //std::cout << "Progress: " << (float)progress / (float)count * 100 << "%\t\r" << std::flush;
        printf("Progress: %f%%\r", (float)progress / (float)count * 100);
    }

    

    source.close();
    std::sort(distances.begin(), distances.end());
    std::sort(addresses.begin(), addresses.end());
    int uniques = std::unique(addresses.begin(), addresses.end()) - addresses.begin();
    
    distances_median = (distances.size() % 2 == 1) ? distances[distances.size() / 2] : (distances[(distances.size() - 1) / 2] + distances[distances.size() / 2]) / 2.0;

    std::cout << "Trace: " + name + '\n';
    std::cout << "Size: " << count << " lines\n";
    std::cout << "Unique Lines: " << uniques << " (" << (double)uniques / (double)count * 100.0 << "%)\n";
    std::cout << "Distances Between Identical Addresses:" << '\n';
    std::cout << "-Max: " << MaxDistance << '\n';
    std::cout << "-Mode: " << distance_frequency[distances_mode] << '\n';
    std::cout << "-Mean: " << (float)distance_sum / (float)(count-uniques) << '\n';
    std::cout << "-Median: " << distances_median << '\n';
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
    std::default_random_engine generator;
    generator = std::default_random_engine(seed);
    for (int i = 1; i <= size; ++i) {
        const char rw = generator() % 100 > writeChance * 100.0f ? 'r' : 'w';
        unsigned long int address;
        if ((float)priors.size() / (float)(i) < 1.0 - refChance || priors.size() == 0) {
            if (generator() % 100 > refChance * 100.0f || priors.size() == 0) {
                address = generator();
                priors.push_back(address);
            }
            else {
                address = priors[generator() % priors.size()];
            }
        }
        else {
            address = priors[generator() % priors.size()];
        }
        file << rw << ' ' << std::hex << address << '\n';
    }
    file.close();
    std::cout << "Created new trace file at traces/" + name + '\n';
}

void CacheSimulator::Run(std::string trace)
{
    Trace = trace;
    std::fstream source;
    source.open("traces/" + trace);
    if (!source.is_open())
    {
        throw std::invalid_argument(
            "Couldn't find trace files. Make sure .exe is in same directory as the folder 'traces'.");
    }

    begin = std::chrono::system_clock::now();

    // Do OPT Pre-processing if needed
    if (Rep == 2)
    {
        std::vector<std::string> trace;
        std::string s;
        while (getline(source, s))
        {
            trace.push_back(s.substr(2));
        }

        for (Cache* cache : Caches) {
            (*cache).PreCompile(trace);
        }
    }
    source.close();
    source.open("traces/" + trace);

    // Go through trace line by line
    std::string line;
    int count = 1;
    while (getline(source, line))
    {
        if (line.substr(0, 1) == "r")
        {
            //std::string Report = l1.Read(line.substr(2));
            Caches[0]->Read(line.substr(2));
        }
        else if (line.substr(0, 1) == "w")
        {
            //std::string Report = l1.Write(line.substr(2));
            Caches[0]->Write(line.substr(2));
        }
        ++count;
    }

    end = std::chrono::system_clock::now();

    source.close();
}

void CacheSimulator::Output(bool showsets, bool print, std::string filename)
{
    std::cout << "===== Simulator configuration =====\n";
    std::cout << "BLOCKSIZE:\t\t" << (*Caches[0]).Blocksize << '\n';
    for (Cache* cache : Caches) {
        std::cout << "L"<< (*cache).Level << "_SIZE:\t\t" << (*cache).Size << '\n';
        std::cout << "L" << (*cache).Level << "_ASSOC:\t\t" << (*cache).Assoc << '\n';
    }

    const std::string repPolicy = [&]() {
        if (Rep == 0)
        {
            return "LRU";
        }
        else if (Rep == 1)
        {
            return "FIFO";
        }
        else if (Rep == 2)
        {
            return "Optimal";
        }
        else
        {
            return "ERROR";
        }
        }();
    std::cout << "REPLACEMENT POLICY:\t" + repPolicy << '\n';

    const std::string incPolicy = [&]() {
        if (Inc == 0)
        {
            return "Non-inclusive";
        }
        else if (Inc == 1)
        {
            return "Inclusive";
        }
        else
        {
            return "ERROR";
        }
        }();
    std::cout << "INCLUSION PROPERTY:\t" + incPolicy << '\n';

    std::cout << "Trace file:\t\t" + Trace << '\n';

    time_t begin_t = std::chrono::system_clock::to_time_t(begin);
    time_t end_t = std::chrono::system_clock::to_time_t(end);

    char str_begin[26];
    ctime_s(str_begin, sizeof(str_begin), &begin_t);
    std::cout << "Begun:\t\t\t" << str_begin;

    char str_end[26];
    ctime_s(str_end, sizeof(str_end), &end_t);
    std::cout << "End:\t\t\t" << str_end;

    std::cout << "Duration:\t\t" << (end - begin) / std::chrono::milliseconds(1) / 1000.0 << " seconds\n";

    if (showsets)
    {
        for (Cache* cache : Caches) {
            std::cout << "===== L" << (*cache).Level << " contents =====\n";
            for (int i = 0; i < (*cache).Sets(); ++i)
            {
                std::string line = "Set\t" + std::to_string(i) + ":\t";
                for (int j = 0; j < (*cache).Assoc; ++j)
                {
                    if ((*cache).Dirty[i][j])
                    {
                        line += (*cache).ToTag(i, j) + " D\t";
                    }
                    else
                    {
                        line += (*cache).ToTag(i, j) + "\t\t";
                    }
                }
                std::cout << line << '\n';
            }
        }
    }

    std::cout << "===== Simulation results(raw) =====\n";
    for (Cache* cache : Caches) {
        std::cout << 'L' << (*cache).Level << " Cache:\n";
        std::cout << "\tReads:\t\t\t" << (*cache).reads << '\n';
        std::cout << "\tRead misses:\t\t" << (*cache).read_misses << '\n';
        std::cout << "\tWrites:\t\t\t" << (*cache).writes << '\n';
        std::cout << "\tWrite misses:\t\t" << (*cache).write_misses << '\n';
        std::cout << "\tMiss rate:\t\t" << (*cache).MissRate() * 100 << "%\n";
        std::cout << "\tWritebacks:\t\t" << (*cache).write_backs << '\n';
        std::cout << "\tTotal memory traffic:\t" << (*cache).read_misses + (*cache).write_misses + (*cache).write_backs << std::endl;
    }

    if (print) {
        std::ofstream file(filename);

        if (file.is_open()) {
            file << "===== Simulator configuration =====" << '\n';
            file << "BLOCKSIZE:\t\t" << (*Caches[0]).Blocksize << '\n';
            for (Cache* cache : Caches) {
                file << "L" << (*cache).Level << "_SIZE:\t\t" << (*cache).Size << '\n';
                file << "L" << (*cache).Level << "_ASSOC:\t\t" << (*cache).Assoc << '\n';
            }
            file << "REPLACEMENT POLICY:\t" + repPolicy << '\n';
            file << "INCLUSION PROPERTY:\t" + incPolicy << '\n';

            file << "Trace file:\t\t" + Trace << '\n';
            file << "Begun:\t\t\t" << str_begin;
            file << "End:\t\t\t" << str_end;
            file << "Duration:\t\t" << (end - begin) / std::chrono::milliseconds(1) / 1000.0 << " seconds\n";

            if (showsets)
            {
                for (Cache* cache : Caches) {
                    file << "===== L" << (*cache).Level << " contents =====\n";
                    for (int i = 0; i < (*cache).Sets(); ++i)
                    {
                        std::string line = "Set\t" + std::to_string(i) + ":\t";
                        for (int j = 0; j < (*cache).Assoc; ++j)
                        {
                            if ((*cache).Dirty[i][j])
                            {
                                line += (*cache).ToTag(i, j) + " D\t";
                            }
                            else
                            {
                                line += (*cache).ToTag(i, j) + "\t\t";
                            }
                        }
                        file << line << '\n';
                    }
                }
            }

            for (Cache* cache : Caches) {
                file << 'L' << (*cache).Level << " Cache:\n";
                file << "\tReads:\t\t\t" << (*cache).reads << '\n';
                file << "\tRead misses:\t\t" << (*cache).read_misses << '\n';
                file << "\tWrites:\t\t\t" << (*cache).writes << '\n';
                file << "\tWrite misses:\t\t" << (*cache).write_misses << '\n';
                file << "\tMiss rate:\t\t" << (*cache).MissRate() * 100 << "%\n";
                file << "\tWritebacks:\t\t" << (*cache).write_backs << '\n';
                file << "\tTotal memory traffic:\t" << (*cache).read_misses + (*cache).write_misses + (*cache).write_backs << std::endl;
            }

            file.close();
            std::cout << "Exported Results to " + filename + '\n';
        }
    }
}

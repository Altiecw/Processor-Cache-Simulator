#include "CacheSimulator.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

int main()
{
    std::vector<std::string> inputs;
    bool looping = true;
    while (looping)
    {
        std::string input;
        std::getline(std::cin, input);
        std::stringstream stream(input);
        while (getline(stream, input, ' ')) {
            inputs.push_back(input);
        }

        if (inputs[0] == "h" || inputs[0] == "H")
        {
            std::cout << "---COMMANDS----" << '\n';
            std::cout << "sim_cache: Launches the simulation with the following parameters. Put spaces between variables:" << '\n';
            std::cout << "\t<int: blocksize> <int: l1 cache size> <int: l1 association> <int: l2 cache size> <int: l2 assocaition> <int: Replacement Policy Code> <int: Inclusion Property Code> <string: trace file> <string: output file>" << '\n';
            std::cout << "\tAll sizes and associations must be powers of 2." << '\n';
            std::cout << "\tSetting l2 cache variables to 0 disables using the l2 cache in simulation." << '\n';
            std::cout << "\tOutput file will only be created if given a filename." << '\n';
            std::cout << "gen: Generates a file to mimick a trace file:" << '\n';
            std::cout << "\t<string: filename> <int: size> <float: % references to prior addresses> <float: % writes>" << '\n';
            std::cout << "end: Stops the program." << std::endl;
        }
        else if (inputs[0] == "end")
        {
            looping = false;
        }
        else if (inputs[0] == "gen")
        {
            CacheSimulator::GenerateTrace(inputs[1], stoi(inputs[2]), std::time({}), stof(inputs[3]), stof(inputs[4]));
        }
        else if (inputs[0] == "eval")
        {
            CacheSimulator::EvaluateTrace(inputs[1]);
        }
        else if (inputs[0] == "")
        {
            std::cout << "Please put in a command" << std::endl;
        }
        else if (inputs[0] == "test")
        {
            // RAMDOM ASSOC AND SIZE TEST
            int assoc = 1;
            while (assoc <= 8)
            {
                for (int size = 10; size < 21; size++)
                {
                    std::cout << "SIZE: " << pow(2, size) << " ASSOC: " + assoc << std::endl;
                    CacheSimulator sim(32, pow(2, size), assoc, 0, 0, 0, 0);
                    sim.Run("gcc_trace.txt");
                    sim.Output(false);
                }
                assoc *= 2;
            }
            for (int size = 10; size < 21; size++)
            {
                std::cout << "SIZE: " << pow(2, size) << " ASSOC: " << pow(2, size) / 32 << std::endl;
                CacheSimulator sim(32, pow(2, size), pow(2, size) / 32, 0, 0, 0, 0);
                sim.Run("gcc_trace.txt");
                sim.Output(false);
            }
        }
        else if (inputs[0] == "benchmark")
        {
            double total_time = 0;
            int increment = 0;
            for (int size = 10; size < 16; size++)
            {
                clock_t start = clock();
                CacheSimulator sim(32, pow(2, size), 1, pow(2, size + 1), 2, 0, 0);
                sim.Run("gcc_trace.txt");
                sim.Output(false);
                clock_t end = clock();
                total_time += ((double)(end - start)) / CLOCKS_PER_SEC;
                std::cout << "This was completed in " << ((double)(end - start)) / CLOCKS_PER_SEC << " seconds." << std::endl;
                increment++;
            }
            std::cout << "Total time: " << total_time << " seconds. Average time: " << total_time / increment << " seconds per test." << std::endl;
        }
        else if (inputs[0] == "sim_cache")
        {
            // Try and interpret sim cache input; Don't do simulation build if inputs
            int blocksize = stoi(inputs[1]);
            int l1size = stoi(inputs[2]);
            int l1assoc = stoi(inputs[3]);
            int l2size = stoi(inputs[4]);
            int l2assoc = stoi(inputs[5]);
            int rep = stoi(inputs[6]);
            int inc = stoi(inputs[7]);

            if (blocksize <= 0 || l1size <= 0 || l1assoc <= 0 || l2size < 0 || l2assoc < 0 || rep < 0 || inc < 0)
            {
                std::cout << "All inputs must be a positive integer" << std::endl;
                continue;
            }

            CacheSimulator sim(blocksize, l1size, l1assoc, l2size, l2assoc, rep, inc);
            sim.Run(inputs[8]);

            if (inputs.size() < 10) {
                sim.Output(true, false);
            }
            else {
                sim.Output(true, true, inputs[9]);
            }
        }

        inputs.clear();
    }
}

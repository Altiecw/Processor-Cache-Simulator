#include "CacheSimulator.h"

#include <fstream>
#include <fstream>
#include <stack>
#include <string>
#include <stdexcept>
#include <sstream>
#include <tuple>
#include <vector>
#include <iostream>
using namespace std;



int main()
{
    //std::cout << "Welcome to the Machine Problem 1 Simulation!" << endl;
    bool looping = true;
    while (looping) {
        // Get and interpret input
        string input = "";
        //cout << "Please enter a command to initiate the simulation. Press H for help:" << endl;
        getline(cin, input);
        std::istringstream stream(input);
        string command, s_blocksize, s_l1size, s_l1assoc, s_l2size, s_l2assoc, s_rep, s_inc, trace;
        stream >> command >> s_blocksize >> s_l1size >> s_l1assoc >> s_l2size >> s_l2assoc >> s_rep >> s_inc >> trace;

        if (input == "h" || input == "H") {
            // Help Screen
            cout << "---COMMANDS----" << endl;
            cout << "sim_cache: Launches the simulation with the following parameters. Put spaces between variables:" << endl;
            cout << "\t<BLOCKSIZE> <L1_SIZE> <L1_ASSOC> <L2_SIZE> <L2_ASSOC> <REPLACEMENT_POLICY> <INCLUSION_PROPERTY> <trace_file>" << endl;
            cout << "/end: Stops the program." << endl;
        }
        else if (input == "/end") {
            looping = false;
        }
        else if (input == "") {
            cout << "Please put in a command" << endl;
        }
        else if (input == "test 1") {
            // RAMDOM ASSOC AND SIZE TEST
            int assoc = 1;
            while (assoc <= 8) {
                for (int size = 10; size < 21; size++) {
                    cout << "SIZE: " + to_string(pow(2, size)) + " ASSOC: " + to_string(assoc) << endl;
                    CacheSimulator sim(32, pow(2, size), assoc, 0, 0, 0, 0);
                    sim.Run("gcc_trace.txt");
                    sim.Output(false);
                }
                assoc *= 2;
            }
            for (int size = 10; size < 21; size++) {
                cout << "SIZE: " + to_string(pow(2, size)) + " ASSOC: " + to_string(pow(2, size) / 32) << endl;
                CacheSimulator sim(32, pow(2, size), pow(2, size) / 32, 0, 0, 0, 0);
                sim.Run("gcc_trace.txt");
                sim.Output(false);
            }
        }
        else if (input.substr(0, 9) == "sim_cache") {
            // Try and interpret sim cache input; Don't do simulation build if inputs 
            int blocksize = stoi(s_blocksize);
            int l1size = stoi(s_l1size);
            int l1assoc = stoi(s_l1assoc);
            int l2size = stoi(s_l2size);
            int l2assoc = stoi(s_l2assoc);
            int rep = stoi(s_rep);
            int inc = stoi(s_inc);

            if (blocksize <= 0 || l1size <= 0 || l1assoc <= 0 || l2size < 0 || l2assoc < 0 || rep < 0 || inc < 0) {
                cout << "All inputs must be a positive integer" << endl;
                continue;
            }

            // Build simulator
            CacheSimulator sim(blocksize, l1size, l1assoc, l2size, l2assoc, rep, inc);
            sim.Run(trace);
            sim.Output(true);

        }

    }
    //cout << "Thank you for using this program." << endl;
}



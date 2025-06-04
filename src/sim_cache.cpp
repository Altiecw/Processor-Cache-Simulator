// MachineProblem_1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <bitset>
#include <fstream>
#include <iostream>
#include <queue>
#include <stack>
#include <string>
#include <stdexcept>
#include <sstream>
#include <tuple>
#include <vector>
using namespace std;

class Cache {
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
    vector<int> LRUTickers;
    vector<queue<int>> Fifo;
    Cache* Higher;
    Cache* Lower;
    vector<vector<string>> Table;
    vector<vector<int>> LRUage;
    vector<vector<bool>> Dirty;
    vector<vector<string>> OPT;

    Cache(){ }

    Cache(int size, int blocksize, int assoc, int level, int rep, int inc) {
        Size = size;
        if (ceil(log2(blocksize)) == floor(log2(blocksize))) {
            Blocksize = blocksize;
        }
        else {
            throw invalid_argument("Blocksize must be a power of 2");
        }
        Assoc = assoc;
        Level = level;
        Rep = rep;
        Inc = inc;

        int sets = Sets();
        s_Index = log2(sets);
        s_Offset = log2(blocksize);
        s_Tag = 32 - s_Index - s_Offset;

        Table = vector<vector<string>>(Sets());
        Dirty = vector<vector<bool>>(Sets());
        LRUage = vector<vector<int>>(Sets());
        LRUTickers = vector<int>(Sets());
        Fifo = vector<queue<int>>(Sets());
        OPT = vector<vector<string>>(Sets());

        for (int i = 0; i < Sets(); i++)
        {
            vector<string> col(Assoc);
            Table[i] = col;
            vector<bool> col1(Assoc);
            Dirty[i] = col1;
            vector<int> col2(Assoc);
            LRUage[i] = col2;
            queue<int> col3;
            Fifo[i] = col3;
            vector<string> col4;
            OPT[i] = col4;
        }
    }

    tuple<string, int, int> Decode(string address) {
        if (address.length() > 8) {
            // Insert enough 0s
            int fill = 8 - address.length();
            for (int i = 0; i < fill; i++) {
                address.insert(0, "0");
            }
        }

        // Convert hex to binary, use to find tag, index, and offset
        string AddBin;
        string r_Tag;
        int r_Index;
        int r_Offset;
        unsigned bits;
        stringstream s;
        s << hex << address;
        s >> bits;
        bitset<32> b(bits);
        AddBin = b.to_string();

        //Index is first element of remaining hex, offset is last
        r_Tag = AddBin.substr(0, s_Tag);
        if (s_Index > 0) {
            r_Index = stoi(AddBin.substr(s_Tag, s_Index), 0, 2);
        }
        else {
            r_Index = 0;
        }
        r_Offset = stoi(AddBin.substr(s_Tag + s_Index), 0, 2);

        return { r_Tag, r_Index, r_Offset };
    }

    string Read(string address) {
        reads += 1;
        // Convert Input
        tuple<string, int, int> Decoded = Decode(address);
        string Tag = get<0>(Decoded);
        int Index = get<1>(Decoded);
        int Offset = get<2>(Decoded);

        // See if tag is at index
        bool Hit = false;
        for (int i = 0; i < Assoc; i++) {
            Decoded = Decode(Table[Index][i]);
            if (get<0>(Decoded) == Tag) {
                // Item found, return it
                Hit = true;
                LRUage[Index][i] = LRUTickers[Index];
                LRUTickers[Index] += 1;
                if (Rep == 2) {
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
        for (int i = 0; i < Assoc; i++) {
            if (Table[Index][i].empty()) {
                slot = i;
                break;
            }
        }

        if (slot < 0) {
            // Find slot via replacement function
            slot = Replace(Index);
            
        }

        string gone = Table[Index][slot];
        Evict(Table[Index][slot]);
        Update(gone, 0);

        // Get block from lower cache
        string block;
        if (Level == 1) {
            if (!l1Only) {
                block = (*Lower).Read(address);
            }
            else {
                block = address;
            }

        }
        else if (Level == 2) {
            // Do nothing (getting from main memory)
            block = address;
        }
        // Block was put into cache, update FIFO queue
        Fifo[Index].push(slot);
        if (Rep == 2) {
            OPT[Index].erase(OPT[Index].begin());
        }
        LRUage[Index][slot] = LRUTickers[Index];
        LRUTickers[Index] += 1;
        Table[Index][slot] = block;

        return block;
    }

    string Write(string address) {
        /// Write to Cache at address
        // Convert Input
        writes += 1;
        tuple<string, int, int> Decoded = Decode(address);
        string Tag = get<0>(Decoded);
        int Index = get<1>(Decoded);
        int Offset = get<2>(Decoded);

        //Check if block is there
        for (int i = 0; i < Assoc; i++) {
            Decoded = Decode(Table[Index][i]);
            if (get<0>(Decoded) == Tag) {
                // Item found, return it and mark it as dirty
                LRUage[Index][i] = LRUTickers[Index];
                LRUTickers[Index] += 1;
                Dirty[Index][i] = true;
                if (Rep == 2) {
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
        for (int i = 0; i < Assoc; i++) {
            if (Table[Index][i].empty()) {
                slot = i;
                break;
            }
        }

        if (slot < 0) {
            // Find slot via replacement function
            slot = Replace(Index);
            
        }

        string gone = Table[Index][slot];
        Evict(Table[Index][slot]);
        Update(gone, 0);

        // Get block from lower cache
        string block;
        if (Level == 1) {
            if (!l1Only) {
                block = (*Lower).Read(address);
            }
            else {
                // Do nothing (getting from main memory)
                block = address;
            }
        }
        else if (Level == 2) {
            // Do nothing (getting from main memory)
            block = address;
        }

        //Put address into slot; Mark it as Dirty
        Fifo[Index].push(slot);
        if (Rep == 2) {
            OPT[Index].erase(OPT[Index].begin());
        }
        LRUage[Index][slot] = LRUTickers[Index];
        LRUTickers[Index] += 1;
        Table[Index][slot] = block;
        Dirty[Index][slot] = true;
        return block;
    }

    void Evict(string address) {
        // Remove Address from Cache, if in cache
        tuple<string, int, int> Decoded = Decode(address);
        string Tag = get<0>(Decoded);
        int Index = get<1>(Decoded);
        int Offset = get<2>(Decoded);

        // Check if block is there
        for (int i = 0; i < Assoc; i++) {
            Decoded = Decode(Table[Index][i]);
            if (get<0>(Decoded) == Tag) {
                // Item found, evict it
                // if block is written to, write it to the next level
                if (Dirty[Index][i]) {
                    // DO NOTHING (writing to main memory)
                    write_backs += 1;
                    if (Level < 2 && !l1Only) {
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

    void Back_Evict(string address) {
        if (address == "") {
            return;
        }

        // Remove Address from Cache, if in cache
        tuple<string, int, int> Decoded = Decode(address);
        string Tag = get<0>(Decoded);
        int Index = get<1>(Decoded);
        int Offset = get<2>(Decoded);

        // Check if block is there
        for (int i = 0; i < Assoc; i++) {
            Decoded = Decode(Table[Index][i]);
            if (get<0>(Decoded) == Tag) {
                // Item found, evict it
                // if block is written to, write it to the next level
                if (Dirty[Index][i]) {
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

    float MissRate() {
        return ((float)read_misses + (float)write_misses) / ((float)reads + (float)writes);
    }

    int Replace(int Index) {
        /// Returns what block should be replaced
        // Check which Replacement policy is there
        if (Rep == 0) {
            // LRU
            int smallest = 0;
            for (int i = 0; i < Assoc; i++) {
                if (LRUage[Index][i] < LRUage[Index][smallest]) {
                    smallest = i;
                }
            }

            //Reduce Ticker and all ages in set
            for (int i = 0; i < Assoc; i++) {
                LRUage[Index][i] -= smallest;
            }
            LRUTickers[Index] -= smallest;

            return smallest;
        }
        else if (Rep == 1) {
            // Fifo
            int returned = Fifo[Index].front();
            Fifo[Index].pop();
            return returned;
        }
        else if (Rep == 2) {
            // Optimal
            // Look into the future addresses in this set, find the one furthest in the vector
            vector<string>::iterator it;
            int largest = 0;
            int largest_i = 0;
            for (int i = 0; i < Assoc; i++) {
                tuple<string, int, int> Decoded = Decode(Table[Index][i]);
                it = std::find(OPT[Index].begin(), OPT[Index].end(), get<0>(Decoded));
                if (it == OPT[Index].end()) {
                    return i;
                }

                int place = it - OPT[Index].begin();
                if (place > largest) {
                    largest = place;
                    largest_i = i;
                }
            }

            return largest_i;
            
        }
    }

    void PreCompile(vector<string> trace) {
        /// Pre-compile trace for OPT
        for (int i = 0; i < trace.size(); i++) {
            tuple<string, int, int> Decoded = Decode(trace[i]);
            string Tag = get<0>(Decoded);
            int Index = get<1>(Decoded);
            int Offset = get<2>(Decoded);

            

            OPT[Index].push_back(Tag);
        }
    }

    void SetLower(Cache * lower) {
        Lower = lower;
    }

    void SetHigher(Cache* higher) {
        Higher = higher;
    }

    string ToTag(int index, int slot) {
        // Return Tag as hexadecimal
        if (Table[index][slot] == "") {
            return "";
        }
        tuple<string, int, int> Decoded = Decode(Table[index][slot]);
        string Tag = get<0>(Decoded);
        int Index = get<1>(Decoded);
        int Offset = get<2>(Decoded);
    
        bitset<32> b(Tag);
        unsigned n = b.to_ulong();
        stringstream s;
        s << hex << n;
        return s.str();
    }

    int Sets() {
        return Size / (Assoc * Blocksize);
    }

    void Update(string address, int change) {
        // Affect either higher or lower cache depending on Replacement policy
        // Change: 0 = deleting address, 1 = inserting address
        if (Inc == 0) {
            // Non-Inclusive; Do nothing since reads in l1 automatically require l2 to have same address
        }
        else if (Inc == 1) {
            // Inclusive
            if (change == 0) {
                // if at l2, remove address from l1 as well
                if (Level == 2) {
                    (*Higher).Back_Evict(address);
                    (*Higher).Update(address, 0);
                    backInvals += 1;
                }
            }
        }
    }
};

class CacheSimulator {
public:
    int Rep;
    int Inc;
    Cache l1, l2;
    bool l1Only = true;
    int s_Tag;
    int s_Index;
    int s_Offset;
    string Trace;
    CacheSimulator(int blocksize, int l1size, int l1assoc, int l2size, int l2assoc, int rep, int inc) {
        l1 = Cache(l1size, blocksize, l1assoc, 1, rep, inc);
        if (l2size > 0) {
            l2 = Cache(l2size, blocksize, l2assoc, 2, rep, inc);
            l1.SetLower(&l2);
            l2.SetHigher(&l1);
            l1Only = false;
        }
        l1.l1Only = l1Only;

        Rep = rep;
        Inc = inc;
    }

    void Run(string trace) {
        // Load file to get commands from
        Trace = trace;
        fstream source;
        source.open("traces/" + trace);
        if (!source.is_open()) {
            throw invalid_argument("Couldn't find trace files. Make sure .exe is in same directory as the folder 'traces'.");
        }

        // Do OPT Pre-processing if needed
        if (Rep == 2) {
            vector<string> trace;
            string s;
            while (getline(source, s))
            {
                trace.push_back(s.substr(2));
            }

            l1.PreCompile(trace);
            if (!l1Only) {
                l2.PreCompile(trace);
            }
        }
        source.close();
        source.open("traces/" + trace);

        //Go through trace line by line
        string line;
        int count = 1;
        while (getline(source, line))
        {
            //cout << "Reading Line " + to_string(count) << endl;
            // Check if this is either a read or a write; Give l1 Read or write command
            if (line.substr(0,1) == "r") {
                string Report = l1.Read(line.substr(2));
            }
            else if (line.substr(0, 1) == "w") {
                string Report = l1.Write(line.substr(2));
            }
            count += 1;
        }

        source.close();
    }

    void Output(bool showsets) {
        cout << "===== Simulator configuration =====" << endl;
        cout << "BLOCKSIZE:             " + to_string(l1.Blocksize) << endl;
        cout << "L1_SIZE:               " + to_string(l1.Size) << endl;
        cout << "L1_ASSOC:              " + to_string(l1.Assoc) << endl;
        cout << "L2_SIZE:               " + to_string(l2.Size) << endl;
        cout << "L2_ASSOC:              " + to_string(l2.Assoc) << endl;
        if (Rep == 0) {
            cout << "REPLACEMENT POLICY:    LRU" << endl;
        }
        else if (Rep == 1) {
            cout << "REPLACEMENT POLICY:    FIFO" << endl;
        }
        else if (Rep == 2) {
            cout << "REPLACEMENT POLICY:    Optimal" << endl;
        }
        if (Inc == 0) {
            cout << "INCLUSION PROPERTY:    non-inclusive" << endl;
        }
        else if (Inc == 1) {
            cout << "INCLUSION PROPERTY:    inclusive" << endl;
        }
        cout << "trace_file:            " + Trace << endl;
        if (showsets) {
            cout << "===== L1 contents =====" << endl;
            for (int i = 0; i < l1.Sets(); i++) {
                string line = "Set\t" + to_string(i) + ":\t";
                for (int j = 0; j < l1.Assoc; j++) {
                    if (l1.Dirty[i][j]) {
                        line += l1.ToTag(i, j) + " D   ";
                    }
                    else {
                        line += l1.ToTag(i, j) + "     ";
                    }
                }
                cout << line << endl;
            }
            if (!l1Only) {
                cout << "===== L2 contents =====" << endl;
                for (int i = 0; i < l2.Sets(); i++) {
                    string line = "Set\t" + to_string(i) + ":\t";
                    for (int j = 0; j < l2.Assoc; j++) {
                        if (l2.Dirty[i][j]) {
                            line += l2.ToTag(i, j) + " D   ";
                        }
                        else {
                            line += l2.ToTag(i, j) + "     ";
                        }
                    }
                    cout << line << endl;
                }
            }
        }
        cout << "===== Simulation results(raw) =====" << endl;
        cout << "a. number of L1 reads:        " + to_string(l1.reads) << endl;
        cout << "b. number of L1 read misses:  " + to_string(l1.read_misses) << endl;
        cout << "c. number of L1 writes:       " + to_string(l1.writes) << endl;
        cout << "d. number of L1 write misses: " + to_string(l1.write_misses) << endl;
        cout << "e. L1 miss rate:              " + to_string(l1.MissRate()) << endl;
        cout << "f. number of L1 writebacks:   " + to_string(l1.write_backs) << endl;
        cout << "g. number of L2 reads:        " + to_string(l2.reads) << endl;
        cout << "h. number of L2 read misses:  " + to_string(l2.read_misses) << endl;
        cout << "i. number of L2 writes:       " + to_string(l2.writes) << endl;
        cout << "j. number of L2 write misses: " + to_string(l2.write_misses) << endl;
        if (!l1Only) {
            cout << "k. L2 miss rate:              " + to_string((float)l2.read_misses / (float)l2.reads) << endl;
        }
        else {
            cout << "k. L2 miss rate:              0" << endl;
        }
        cout << "l. number of L2 writebacks:   " + to_string(l2.write_backs) << endl;
        if (!l1Only) {
            if (Inc == 0) {
                cout << "m. total memory traffic:      " + to_string(l2.read_misses + l2.write_misses + l2.write_backs) << endl;
            }
            else if (Inc == 1) {
                cout << "m. total memory traffic:      " + to_string(l2.read_misses + l2.write_misses + l2.write_backs) << endl;
            }
            //cout << "Back Invals:                  " + to_string(l2.backInvals) << endl;
        }
        else {
            cout << "m. total memory traffic:      " + to_string(l1.read_misses + l1.write_misses + l1.write_backs) << endl;
        }
    }
};

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
                    CacheSimulator sim(32, pow(2,size), assoc, 0, 0, 0, 0);
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
        else if (input.substr(0,9) == "sim_cache") {
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



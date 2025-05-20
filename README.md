This is a simulator of a L1 and optional L2 cache in C++.

It reads through a trace fine containing tens of thousands of read and write commands for addresses standing in for blocks of data. The processor reads and writes based on the commands and whether or not the addresses are present within the caches.

The simulator can take different cache sizes, block sizes, associations, and replacement and inlcusion policies.

To run a simulation, run the .exe built from the code and then type "h" to get the parametres, then type the properties for each.

For example, "sim_cache 32 1024 4 8192 8 0 1 gcc_trace.txt" runs a simulation where block sizes are 32 bytes, the L1 cache has a size of 1024 bytes with 4 assocaitivity, the L2 cache had a size of 8192 bytes with 8 associativity, there is a least recently used replacement policy indicated by the code 0, a inclusive policy indicated by the 1, and using the gcc_trace file.

There are three available replacement policies: Least recently used (0), first in first out (1), and optimal (2).

There is the non-inclusive policy (0), and the inclusive policy (1).

At the end of a simulation (which may take around 10 seconds depending on your computer). The program will print out the configuration, then the final state of the caches, along with statistics on the read, writes, misses, and writebacks for each cache.

Additionally, there is a test when inputting "test 1" that runs tests with vary sizes and associativity with the L1 cache only with a least recently used replacement policy and a non-inclusive policy.

This was for an assignment in a computer architecture course I was taking in 2022 when I was getting my masters from UCF. This was one of the first major C++ programs I made, after having followed some OpenGL tutorials for classes in the past 2 years and some C++ lessons before. It felt impressive how quickly I adapted to using C++, it played an important part as future courses and my first software development job used C++.

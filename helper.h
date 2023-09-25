#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdfloat>
#include <utility>
#include <vector>

// I'm not sure why std::float64_t didn't compile, maybe C++23 isn't out yet :shrug:
using float64_t = double;

// This is 1G
const size_t FILE_SIZE = 1 << 30; // This is 1G
const float64_t MICROSECONDS_PER_SECOND = 1'000'000;

// For part 1
const size_t PAGE_SIZE = sysconf(_SC_PAGE_SIZE);
const char BYTE_TO_WRITE = '?';

// For part 2
const size_t IO_SIZE = 4096;

// Common helper functions

inline void handle_error(const char* msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// Return a permutation of writing positions
std::vector<size_t> generate_write_positions(size_t part_size, bool random) {
    std::vector<size_t> positions;
    for (size_t i = 0; i < FILE_SIZE; i += part_size) {
        positions.emplace_back(i);
    }
    // I really don't think anyone should be writing their own permutation shuffling ...
    // Also rand() is bad, read https://codeforces.com/blog/entry/61587
    if (random) {
        std::mt19937_64 rng(std::chrono::steady_clock::now().time_since_epoch().count());
        std::shuffle(positions.begin(), positions.end(), rng);
    }
    return positions;
}

// Return <mean, standard deviation>
std::pair<float64_t, float64_t> get_stats(std::vector<float64_t> samples) {
    size_t n = samples.size();
    float64_t mu = std::accumulate(samples.begin(), samples.end(), static_cast<float64_t>(0)) / n;
    float64_t variance = std::accumulate(samples.begin(), samples.end(), static_cast<float64_t>(0),
                                         [&mu](float64_t accum, float64_t elem) {
                                             return accum + (elem - mu) * (elem - mu);
                                         }) / n;
    return std::make_pair(mu, std::sqrt(variance));
}

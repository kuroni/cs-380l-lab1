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

const size_t FILE_SIZE = 1 << 30;
const size_t PAGE_SIZE = sysconf(_SC_PAGE_SIZE);
const char BYTE_TO_WRITE = '?';
const float64_t MICROSECONDS_PER_SECOND = 1'000'000;

inline void handle_error(const char* msg) {
    std::cerr << msg << ": " << std::strerror(errno) << '\n';
    exit(EXIT_FAILURE);
}

std::vector<size_t> generate_write_positions(bool random) {
    std::vector<size_t> positions;
    for (size_t i = 0; i < FILE_SIZE; i += PAGE_SIZE) {
        positions.emplace_back(i);
    }
    // i really don't think anyone should be writing their own permutation shuffling ...
    // read https://codeforces.com/blog/entry/61587
    if (random) {
        std::mt19937_64 rng(std::chrono::steady_clock::now().time_since_epoch().count());
        std::shuffle(positions.begin(), positions.end(), rng);
    }
    return positions;
}

// return time taken to run this function in microseconds
int64_t write_random_bytes(const char* file_name, int mmap_flags, bool debug, std::vector<size_t> page_positions) {
    auto start_time = std::chrono::steady_clock::now();

    int fd = -1;

    if ((fd = open(file_name, O_RDWR, 0)) == -1) {
        handle_error("open");
    }

    // this shouldn't affect runtime too much but i made it into a separate block just in case
    // checking if file size is actually 1G
    if (debug) {
        struct stat sb;
        if (fstat(fd, &sb) == -1) {
            handle_error("fstat");
        }
        assert(sb.st_size == FILE_SIZE);
    }

    char* addr = (char*)mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, mmap_flags, fd, 0);
    if (addr == MAP_FAILED) {
        handle_error("mmap");
    }

    for (size_t position : page_positions) {
        addr[position] = BYTE_TO_WRITE;
    }

    // clean up: sync map, unmap, and close file
    if (msync(addr, FILE_SIZE, MS_SYNC) == -1) {
        handle_error("msync");
    }
    if (munmap(addr, FILE_SIZE)) {
        handle_error("munmap");
    }
    if (close(fd) == -1) {
        handle_error("close");
    }

    auto end_time = std::chrono::steady_clock::now();
    int64_t elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    return elapsed_time;
}

// return pair(mean, standard deviation)
std::pair<float64_t, float64_t> get_stats(std::vector<float64_t> samples) {
    size_t n = samples.size();
    float64_t mu = std::accumulate(samples.begin(), samples.end(), static_cast<float64_t>(0)) / n;
    float64_t variance = std::accumulate(samples.begin(), samples.end(), static_cast<float64_t>(0),
                                         [&mu](float64_t accum, float64_t elem) {
                                             return accum + (elem - mu) * (elem - mu);
                                         }) / n;
    return std::make_pair(mu, std::sqrt(variance));
}

int main(int argc, char** argv) {
    const char* file_name = (const char*)argv[1];
    int iterations = atoi(argv[2]);

    std::vector<float64_t> samples;
    for (int it = 0; it < iterations; it++) {
        std::vector<size_t> page_positions = generate_write_positions(true);
        // apparently we don't need MAP_FILE, as it's ignored? that's neat
        int64_t elapsed_time = write_random_bytes(file_name, MAP_ANONYMOUS | MAP_SHARED, true, page_positions);
        samples.push_back(elapsed_time / MICROSECONDS_PER_SECOND);
    }
    auto [mu, sd] = get_stats(samples);

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Mean:     " << mu << '\n';
    std::cout << "SD:       " << sd << '\n';
    for (size_t it = 0; it < iterations; it++) {
        std::cout << "Sample " << it << ": " << samples[it] << '\n';
    }

    return EXIT_SUCCESS;
}

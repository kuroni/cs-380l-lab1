#include <iostream>
#include <cstring>
#include <cassert>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>

#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

const size_t FILE_SIZE = 1 << 30;
const size_t PAGE_SIZE = sysconf(_SC_PAGE_SIZE);
const char BYTE_TO_WRITE = '?';

inline void handle_error(const char *msg) {
    std::cerr << msg << ": " << std::strerror(errno) << '\n';
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
    const char* filename = (const char*)argv[1];
    int fd = -1;
    struct stat sb;

    if ((fd = open(filename, O_RDWR, 0)) == -1) {
        handle_error("open");
    }
    if (fstat(fd, &sb) == -1) {
        handle_error("fstat");
    }
    assert(sb.st_size == FILE_SIZE);

    char* addr = (char*)mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        handle_error("mmap");
    }

    std::vector<size_t> positions;
    for (size_t i = 0; i < FILE_SIZE; i += PAGE_SIZE) {
        positions.emplace_back(i);
    }
    std::mt19937_64 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::shuffle(positions.begin(), positions.end(), rng);

    for (size_t position : positions) {
        addr[position] = BYTE_TO_WRITE;
    }

    if (msync(addr, FILE_SIZE, MS_SYNC) == -1) {
        handle_error("msync");
    }
}

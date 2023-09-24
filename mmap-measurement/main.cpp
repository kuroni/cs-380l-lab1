#include <iostream>
#include <cstring>
#include <cassert>

#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>

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
    assert(sb.st_size == (1 << 30));

    char* addr = (char*)mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_FILE | MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
        handle_error("mmap");
    }
}

#include "helper.h"

// return time taken to run this function in microseconds
int64_t experiment(const char* file_name, int mmap_flags, std::vector<size_t> page_positions, bool debug = false) {
    auto start_time = std::chrono::steady_clock::now();

    int fd = -1;
    if ((fd = open(file_name, O_RDWR)) == -1) {
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
    if (munmap(addr, FILE_SIZE) == -1) {
        handle_error("munmap");
    }
    if (close(fd) == -1) {
        handle_error("close");
    }

    auto end_time = std::chrono::steady_clock::now();
    int64_t elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    return elapsed_time;
}

int main(int argc, char** argv) {
    int iterations = 1;
    bool file_backed = true, shared = true;
    std::string file_name; // I won't meddle with char* :|

    // handing options here
    // courtersy of https://man7.org/linux/man-pages/man3/getopt.3.html
    while (true) {
        int option_index = 0;
        static struct option long_options[] = {
            {"file", required_argument, 0, 0},
            {"iter", required_argument, 0, 0},
            {"anon", no_argument, 0, 0},
            {"priv", no_argument, 0, 0},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "", long_options, &option_index);
        if (c == -1) {
            break;
        } else if (c == 0) {
            switch (option_index) {
                case 0: // file
                    file_name = optarg; break;
                case 1: // iterations
                    iterations = atoi(optarg); break;
                case 2: // anonymous
                    file_backed = false; break;
                case 3: // private
                    shared = false; break;
            }
        }
    }

    // apparently we don't need MAP_FILE, as it's ignored?
    int mmap_flag = (file_backed ? MAP_FILE : MAP_ANONYMOUS) | (shared ? MAP_SHARED : MAP_PRIVATE);

    std::vector<float64_t> samples;
    for (int it = 0; it < iterations; it++) {
        // we always randomize position for this assignment
        std::vector<size_t> page_positions = generate_write_positions(PAGE_SIZE, true);
        int64_t elapsed_time = experiment(file_name.c_str(), mmap_flag, page_positions);
        std::clog << "Iteration " << it << ": " << elapsed_time << "μs\n";
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

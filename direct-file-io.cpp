#include "helper.h"

// rewriting the file IO a little to use my own convetions
void do_file_io(int fd, char *buf, std::vector<size_t>& offset_array, int opt_read) {
    int ret = 0;
    for (int64_t offset : offset_array) {
        if ((ret == lseek(fd, offset, SEEK_SET)) == -1) {
            handle_error("lseek");
        }
        if (opt_read) {
            ret = read(fd, buf, IO_SIZE);
        } else {
            ret = write(fd, buf, IO_SIZE);
        }
        if (ret == -1) {
            handle_error("read/write");
        }
    }
}

// return time taken to run this function in microseconds
int64_t experiment(const char* file_name, std::vector<size_t> write_positions, bool is_read, bool debug = false) {
    auto start_time = std::chrono::steady_clock::now();

    int fd = -1;
    if ((fd = open(file_name, O_RDWR | O_DIRECT)) == -1) {
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

    // allocate buf
    char* buf = (char*)memalign(IO_SIZE, IO_SIZE);
    if (buf == NULL) {
        handle_error("memalign");
    }

    do_file_io(fd, buf, write_positions, is_read);

    if (close(fd) == -1) {
        handle_error("close");
    }

    auto end_time = std::chrono::steady_clock::now();
    int64_t elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    return elapsed_time;
}

int main(int argc, char** argv) {
    // bind process to one cpu; see https://stackoverflow.com/questions/8326427/how-to-force-a-c-program-to-run-on-a-particular-core
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(0, &set);
    sched_setaffinity(0, sizeof(cpu_set_t), &set);

    int iterations = 1;
    bool is_read = true;
    bool is_random_position = false;
    std::string file_name; // I won't meddle with char* :|

    // handing options here
    // courtersy of https://man7.org/linux/man-pages/man3/getopt.3.html
    while (true) {
        int option_index = 0;
        static struct option long_options[] = {
            {"file", required_argument, 0, 0},
            {"iter", required_argument, 0, 0},
            {"rand", no_argument, 0, 0},
            {"write", no_argument, 0, 0},
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
                case 2: // randomizing positions
                    is_random_position = true; break;
                case 3: // write
                    is_read = false; break;
            }
        }
    }

    load_file_to_cache(file_name.c_str()); // load file into cache

    std::vector<float64_t> samples;
    for (int it = 0; it < iterations; it++) {
        std::vector<size_t> write_positions = generate_write_positions(IO_SIZE, is_random_position);
        int64_t elapsed_time = experiment(file_name.c_str(), write_positions, is_read);
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

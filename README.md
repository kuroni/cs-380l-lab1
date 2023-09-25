# CS 380L Fall 2023 - Lab 1
You know what this is

## Running

Simply do `make <file>.cpp`

## Options

For `mmap-measurement`, these options are available:
- `--file=<file_name>`: points to the 1G file for the experiment.
- `--iter=<iterations>`: number of iterations to run.
- `--anon`: have this argument to use the `MAP_ANONYMOUS` flag; without this argument, `MAP_FILE` is used instead.
- `--priv`: have this argument to use the `MAP_PRIVATE` flag; without this argument, `MAP_SHARED` is used instead.

For `direct-file-io`, these options are available:
- `--file=<file_name>`: points to the 1G file for the experiment.
- `--iter=<iterations>`: number of iterations to run.
- `--rand`: have this argument to enable random r/w positions; without this argument, the r/w positions are sequential.
- `--write`: have this argument to write to the file; without this argument, the program reads from the file.

# TCP proxy-server

## How to start:

    cmake CMakeLists.txt
    make
    ./server (or ./server "config_path")

### Description:

Ip-addresses and ports client/server and logfile-path can be changed in config.txt (default placed in root directory).
Path to configuration-file can be given as second argument.
Logging of input query. Each new query in logfile starts from comment wich declare length.
Package type: Postgres.

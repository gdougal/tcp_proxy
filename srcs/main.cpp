#include "Server.hpp"

int main(int argc, char** argv) {
	Server	serv(Config("./config.txt"));
	serv.run_server();
	return 0;
}

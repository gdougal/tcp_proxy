#include "Server.hpp"

int main(int argc, char** argv) {
	std::string path;
	if (argc == 2) {
		path = (argv[1]);
	}
	else {
		path = "./config.txt";
	}
	try {
		Server	serv((Config)(path));
		serv.run_server();
	}
	catch (...) {}
	return 0;
}

//
// Created by Gilberto Dougal on 5/10/21.
//

#ifndef PROXY_SERVER_SERVER_HPP
#define PROXY_SERVER_SERVER_HPP

#include "Bridge.hpp"
#include "Config.hpp"
#include <iostream>
#include <list>
#include <netinet/in.h>
#include <string>
#include <sys/fcntl.h>
#include <unistd.h>


class Server {
public:
	Server(const Config &cfg);
	virtual ~Server();
	[[noreturn]] void run_server();

private:
	void	manage_client_fd();
	void	create_client();
	void	close_fd_error(std::string except);
	void	create_listen_socket(const Config &cfg);
	void	create_db_mask(const Config &cfg);

	int																	listen_fd_;
	int																	max_fd_;
	fd_set															read_fds_;
	fd_set															write_fds_;
	sockaddr_in													client_mask_;
	std::list<std::shared_ptr<Bridge> >	bridges_;
	int																	logfile_;
};

#endif //PROXY_SERVER_SERVER_HPP

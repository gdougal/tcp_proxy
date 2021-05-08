#include <iostream>
#include "Config.hpp"
#include <iostream>
#include <vector>
#include <netinet/in.h>
#include <string>


#include <fstream>
#include <sys/fcntl.h>
#include <unistd.h>
#include <cstring>
#include <csignal>

//#if __APPLE__
#include <dns_util.h>
//#elif __linux__
//#include <arpa/inet.h>
#include <list>

enum state {
	READ_FROM_CLIENT,
	SEND_TO_DB,
	READ_FROM_DB,
	SEND_TO_CLIENT,
	FINNAL
};

class Bridge {
	int fd_client_proxy_;
	int fd_proxy_db_;
	uint8_t cur_state_;
public:
	Bridge(sockaddr_in &info, int client_to_proxy) :
		fd_client_proxy_(client_to_proxy) {
		if((fd_proxy_db_ = connect(fd_client_proxy_, (sockaddr *)&info, sizeof(info))) == -1) {
			std::cerr << "Client cant`t accept client" << std::endl;
		}
		cur_state_ = state::READ_FROM_CLIENT;
	}
	//read_from_clien {log} -> send_to_db
	/*[close if (read(clent) < 0)]*/
	//read_from_db -> send_to_client

	int			getFdClientProxy()	const { return fd_client_proxy_; }
	int			getFdProxyDb()			const { return fd_proxy_db_; }
	uint8_t	getCurState()			const { return cur_state_; }
};


class Server {
public:
	Server(const Config &cfg) {
		create_listen_socket(cfg);
		create_db_mask(cfg);
	}

	[[noreturn]] void run_server() {
		while (true) {
			manage_client_fd();
			select(max_fd_, &read_fds_, &write_fds_, nullptr, nullptr);
			create_client();

		}
	}
private:

	void manage_client_fd() {
		FD_ZERO(&read_fds_);
		FD_ZERO(&write_fds_);
		for (auto &item: bridges_) {
			switch (item->getCurState()) {
				case state::READ_FROM_CLIENT:
					FD_SET(item->getFdClientProxy(), &read_fds_);
//						break;
				case state::SEND_TO_CLIENT:
					FD_SET(item->getFdClientProxy(), &write_fds_);
//						break;
				case state::READ_FROM_DB:
					FD_SET(item->getFdProxyDb(), &read_fds_);
//						break;
				case state::SEND_TO_DB:
					FD_SET(item->getFdProxyDb(), &write_fds_);
//						break;
			}
		}
	}

	void	create_client() {
		int		new_client_fd;
		if (FD_ISSET(listen_fd_, &read_fds_)) {
			if ((new_client_fd = accept(listen_fd_, nullptr, nullptr)) < 0) {
				std::cerr << "Cant`t accept client" << std::endl;
				return;
			}
			bridges_.emplace_back(new Bridge(client_mask_, new_client_fd));
		}
	}

	void close_fd_error(std::string except) {
		if (close(listen_fd_) < 0)
			throw std::runtime_error(except.append(" and error close"));
		throw std::runtime_error(except);
	}

	void create_listen_socket(const Config &cfg) {
		struct sockaddr_in servaddr{};

		if ((listen_fd_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
			throw std::runtime_error("Create socket error");
		}
		std::memset(&servaddr, 0, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = cfg.getSection("SERVER").getAddrVal("port");
		servaddr.sin_port = cfg.getSection("SERVER").getAddrVal("ip");
		int yes = 1;

		if (setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			close_fd_error("error setsockopt");
		}
		if ((bind(listen_fd_, (struct sockaddr *) &servaddr, sizeof servaddr)) < 0) {
			close_fd_error("bind error");
		}
		if (fcntl(listen_fd_, F_SETFL, O_NONBLOCK) < 0) {
			close_fd_error("fcntl error");
		}
		if (listen(listen_fd_, 15) < 0) {
			close_fd_error("listen error!");
		}
	}

	void create_db_mask(const Config &cfg) {
		std::memset(&client_mask_, 0, sizeof(client_mask_));
		client_mask_.sin_family = AF_INET;
		client_mask_.sin_addr.s_addr = cfg.getSection("DB").getAddrVal("ip");
		client_mask_.sin_port = cfg.getSection("DB").getAddrVal("port");
	}


	int																	listen_fd_;
	int																	max_fd_;
	fd_set															read_fds_;
	fd_set															write_fds_;
	sockaddr_in													client_mask_;
	std::list<std::shared_ptr<Bridge> >	bridges_;
};


int main(int argc, char** argv) {
	Server	serv(Config("./config.txt"));
	return 0;
}

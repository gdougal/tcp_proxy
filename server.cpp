#include <iostream>
#include <fstream>
#include "Config.hpp"
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
#include <array>
#define	MAX_DATA_SIZE 508

enum state {
	READ_FROM_CLIENT,
	SEND_TO_DB,
	READ_FROM_DB,
	SEND_TO_CLIENT,
	FINALL
};

class Bridge {
	int																			fd_client_proxy_;
	int																			fd_proxy_db_;
	uint8_t																	cur_state_;
	std::vector<uint8_t>										buf;
	ssize_t																	package_size_ = 0;

public:
	Bridge(sockaddr_in &info, int client_to_proxy) :
		fd_client_proxy_(client_to_proxy) {
		fd_proxy_db_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if((connect(fd_proxy_db_, (sockaddr *)&info, sizeof(info))) == -1) {
			std::cerr << "Client cant`t accept client" << std::endl;
		}
		cur_state_ = state::READ_FROM_CLIENT;
		buf.reserve(MAX_DATA_SIZE);
	}

	void	logger() {
		std::cout << "Logger" << std::endl;
		std::cout << buf.data() << std::endl;
	}

	void	send_to_db() {
		if(send(fd_proxy_db_, buf.data(), package_size_, 0) > 0) {
			cur_state_ = state::READ_FROM_DB;
		}
	}

	void	read_from_client() {
		uint8_t	_buf[MAX_DATA_SIZE];
		std::cout << "Start_read_from_client" << std::endl;
		package_size_ = recv(fd_client_proxy_, _buf, MAX_DATA_SIZE - 1, 0);
		if (package_size_ <= 0) {
			std::cerr << "Cant`t recv from client" << std::endl;
			cur_state_ = state::FINALL;
		}
		else {
			buf = std::vector(_buf, _buf + package_size_);
			std::cout << "After_read_from_client" << std::endl;
			cur_state_ = state::SEND_TO_DB;
			logger();
			send_to_db();
		}
	}

	void	send_to_client() {
		std::cout << "Send_to_client" << std::endl;
		if(send(fd_proxy_db_, buf.data(), package_size_, 0) > 0) {
			cur_state_ = state::FINALL;
		}
	};

	void	read_from_db() {
		buf.clear();
		uint8_t	_buf[MAX_DATA_SIZE];
		std::cout << "Read_form_DB" << std::endl;
		package_size_ = recv(fd_proxy_db_, _buf, MAX_DATA_SIZE - 1, 0);
		if (package_size_ <= 0) {
			std::cerr << "Cant`t recv from db" << std::endl;
			cur_state_ = state::FINALL;
		}
		else {
			buf = std::vector(_buf, _buf + package_size_);
			std::cout << "After_form_DB" << std::endl;
			cur_state_ = state::SEND_TO_CLIENT;
			send_to_client();
		}
	}

	int			getFdClientProxy()	const { return fd_client_proxy_; }
	int			getFdProxyDb()			const { return fd_proxy_db_; }
	uint8_t	getCurState()			const { return cur_state_; }
};


class Server {
//	std::ifstream	file;
public:
	Server(const Config &cfg) {
		create_listen_socket(cfg);
		create_db_mask(cfg);
	}

	[[noreturn]] void run_server() {
		while (true) {
			manage_client_fd();
			select(max_fd_ + 1, &read_fds_, &write_fds_, nullptr, nullptr);
			create_client();

			for (auto it = bridges_.begin(); it != bridges_.end() ; ) {
				if (FD_ISSET((*it)->getFdClientProxy(), &read_fds_) && (*it)->getCurState() == state::READ_FROM_CLIENT) {
					(*it)->read_from_client();
				}
				else if (FD_ISSET((*it)->getFdProxyDb(), &read_fds_) && (*it)->getCurState() == state::READ_FROM_DB) {
					(*it)->read_from_db();
				}
				if ((*it)->getCurState() == state::FINALL) {
					it = bridges_.erase(it);
				}
				else {
					++it;
				}
			}

		}
	}
private:

	void manage_client_fd() {
		std::cout << "Manage_start" << std::endl;
		FD_ZERO(&read_fds_);
		FD_ZERO(&write_fds_);
		FD_SET(listen_fd_, &read_fds_);
		max_fd_ = listen_fd_;
		for (auto &item: bridges_) {
			switch (item->getCurState()) {
				case state::READ_FROM_CLIENT:
					FD_SET(item->getFdClientProxy(), &read_fds_);
					max_fd_ = std::max(listen_fd_, item->getFdClientProxy());
					std::cout << "1" << std::endl;
//						break;
//				case state::SEND_TO_CLIENT:
//					FD_SET(item->getFdClientProxy(), &write_fds_);
						break;
				case state::READ_FROM_DB:
					FD_SET(item->getFdProxyDb(), &read_fds_);
					max_fd_ = std::max(listen_fd_, item->getFdProxyDb());
					std::cout << "2" << std::endl;
//						break;
//				case state::SEND_TO_DB:
//					FD_SET(item->getFdProxyDb(), &write_fds_);
						break;
			}
			std::cout << "3" << std::endl;
		}
		std::cout << "Mange_end" << std::endl;
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
		servaddr.sin_addr.s_addr = cfg.getSection("SERVER").getAddrVal("ip");
		servaddr.sin_port = cfg.getSection("SERVER").getPortVal("port");
		int yes = 1;

		if (setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			close_fd_error("error setsockopt");
		}
		if ((bind(listen_fd_, (struct sockaddr *) &servaddr, sizeof servaddr)) == -1) {
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
		client_mask_.sin_port = cfg.getSection("DB").getPortVal("port");
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
	serv.run_server();
	return 0;
}

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
#define	MAX_DATA_SIZE 10000

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
	char																		buf_[MAX_DATA_SIZE];
	ssize_t																	package_size_ = 0;

public:
	Bridge(sockaddr_in &info, int client_to_proxy) :
		fd_client_proxy_(client_to_proxy),
		package_size_(0) {
		fd_proxy_db_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if((connect(fd_proxy_db_, (sockaddr *)&info, sizeof(info))) == -1) {
			std::cerr << "Client cant`t accept client" << std::endl;
		}
//		fcntl(fd_proxy_db_, F_SETFL, O_NONBLOCK);
//		fcntl(fd_client_proxy_, F_SETFL, O_NONBLOCK);
		cur_state_ = state::READ_FROM_CLIENT;
		clear_buf();
	}

	virtual ~Bridge() {
		close(fd_proxy_db_);
		close(fd_client_proxy_);
	}

	void	logger() {
		std::cout << "Logger" << std::endl;
		for (int i = 0; i < package_size_; ++i) {
			std::cout << buf_[i];
		}
		std::cout << std::endl;
	}

	void	send_to_db() {
		std::cout << "Sending to DB" << std::endl;
		logger();
		if(send(fd_proxy_db_, buf_, package_size_, 0) < 0) {
			cur_state_ = state::FINALL;
			return;
		}
		cur_state_ = state::READ_FROM_DB;
		clear_buf();
	}

	void	read_from_client() {
		std::cout << "Start_read_from_client" << std::endl;
		clear_buf();
		package_size_ = recv(fd_client_proxy_, buf_, MAX_DATA_SIZE - 1, 0);
		if (package_size_ <= 0) {
			std::cerr << "Cant`t recv from client" << std::endl;
			cur_state_ = state::FINALL;
		}
		else {
			std::cout << "After_read_from_client" << std::endl;
			cur_state_ = state::SEND_TO_DB;
			std::cout << std::endl;
		}
	}

	void	send_to_client() {
		std::cout << "Send_to_client" << std::endl;
		logger();
		if (send(fd_client_proxy_, buf_, package_size_, 0) < 0) {
			cur_state_ = state::FINALL;
			return ;
		}
		cur_state_ = state::READ_FROM_CLIENT;
		clear_buf();
	};

	void	read_from_db() {
		std::cout << "Read_form_DB" << std::endl;
		package_size_ = recv(fd_proxy_db_, buf_, MAX_DATA_SIZE - 1, 0);
		if (package_size_ <= 0) {
			std::cerr << "Cant`t recv from db" << std::endl;
			cur_state_ = state::FINALL;
			return ;
		}
		cur_state_ = state::SEND_TO_CLIENT;
	}

	int			getFdClientProxy()	const { return fd_client_proxy_; }
	int			getFdProxyDb()			const { return fd_proxy_db_; }
	uint8_t	getCurState()				const { return cur_state_; }

private:
	void	clear_buf() {
		std::memset(buf_, '\0', package_size_);
		package_size_ = 0;
	}
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
			std::cout << "START CYCLE" << std::endl;
			manage_client_fd();
			select(max_fd_ + 1, &read_fds_, &write_fds_, nullptr, nullptr);
			std::cout << "AFTER SELECT" << std::endl;
			create_client();
			for (auto it = bridges_.begin(); it != bridges_.end() ; ) {
				std::cout << "ITS FOR BABY" << std::endl;
				if (FD_ISSET((*it)->getFdClientProxy(), &read_fds_) && (*it)->getCurState() == state::READ_FROM_CLIENT) {
					(*it)->read_from_client();
				}
				else if (FD_ISSET((*it)->getFdProxyDb(), &write_fds_) && (*it)->getCurState() == state::SEND_TO_DB) {
					(*it)->send_to_db();
				}
				else if (FD_ISSET((*it)->getFdProxyDb(), &read_fds_) && (*it)->getCurState() == state::READ_FROM_DB) {
					(*it)->read_from_db();
				}
				else if (FD_ISSET((*it)->getFdClientProxy(), &write_fds_) && (*it)->getCurState() == state::SEND_TO_CLIENT) {
					(*it)->send_to_client();
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
					break;
				case state::SEND_TO_CLIENT:
					FD_SET(item->getFdClientProxy(), &write_fds_);
					max_fd_ = std::max(listen_fd_, item->getFdClientProxy());
					std::cout << "2" << std::endl;
					break;
				case state::READ_FROM_DB:
					FD_SET(item->getFdProxyDb(), &read_fds_);
					max_fd_ = std::max(listen_fd_, item->getFdProxyDb());
					std::cout << "3" << std::endl;
					break;
				case state::SEND_TO_DB:
					FD_SET(item->getFdProxyDb(), &write_fds_);
					max_fd_ = std::max(listen_fd_, item->getFdProxyDb());
					std::cout << "4" << std::endl;
					break;
			}
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
			fcntl(new_client_fd, F_SETFL, O_NONBLOCK);
			bridges_.emplace_back(new Bridge(client_mask_, new_client_fd));
			std::cout << "Client was accepted successful!" << std::endl;
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

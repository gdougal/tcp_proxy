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
	enum state															cur_state_;
	char*																		buf_ = new char[MAX_DATA_SIZE];
	ssize_t																	package_size_ = 0;
	int																			outfile_;
	std::function<void()>										handler;
public:
	Bridge(sockaddr_in &info, int client_to_proxy, int file) :
		fd_client_proxy_(client_to_proxy),
		package_size_(0),
		outfile_(file) {
		fd_proxy_db_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if((connect(fd_proxy_db_, (sockaddr *)&info, sizeof(info))) == -1) {
			std::cerr << "Client cant`t accept client" << std::endl;
		}
		cur_state_ = state::READ_FROM_CLIENT;
		clear_buf();
		handler = std::bind(&Bridge::read_from_client, this);
	}

	virtual ~Bridge() {
		close(fd_proxy_db_);
		close(fd_client_proxy_);
		delete[] buf_;
	}

	void		caller(bool a) {
		if (a == true)
			handler();
	}

	int					getFdClientProxy()	const { return fd_client_proxy_; }
	int					getFdProxyDb()			const { return fd_proxy_db_; }
	enum state	getCurState()				const { return cur_state_; }

private:

	void	send_to_db() {
		logger();
		if(send(fd_proxy_db_, buf_, package_size_, 0) < 0) {
			cur_state_ = state::FINALL;
			return;
		}
		cur_state_ = state::READ_FROM_DB;
		handler = std::bind(&Bridge::read_from_db, this);
		clear_buf();
	}

	void	read_from_client() {
		clear_buf();
		package_size_ = recv(fd_client_proxy_, buf_, MAX_DATA_SIZE - 1, 0);
		if (package_size_ <= 0) {
			std::cerr << "Cant`t recv from client" << std::endl;
			cur_state_ = state::FINALL;
		}
		else {
			handler = std::bind(&Bridge::send_to_db, this);
			cur_state_ = state::SEND_TO_DB;
		}
	}

	void	send_to_client() {
		if (send(fd_client_proxy_, buf_, package_size_, 0) < 0) {
			cur_state_ = state::FINALL;
			return ;
		}
		handler = std::bind(&Bridge::read_from_client, this);
		cur_state_ = state::READ_FROM_CLIENT;
		clear_buf();
	};

	void	read_from_db() {
		package_size_ = recv(fd_proxy_db_, buf_, MAX_DATA_SIZE - 1, 0);
		if (package_size_ <= 0) {
			std::cerr << "Cant`t recv from db" << std::endl;
			cur_state_ = state::FINALL;
			return ;
		}
		handler = std::bind(&Bridge::send_to_client, this);
		cur_state_ = state::SEND_TO_CLIENT;
	}

	void	clear_buf() {
		std::memset(buf_, '\0', package_size_);
		package_size_ = 0;
	}
	void	logger() {
		uint len = (uint(buf_[1]) << 24) | (uint(buf_[2]) << 16) | (uint(buf_[3]) << 8) | (uint(buf_[4])) - 5;
		if (buf_[0] == 'Q' && len < package_size_) {
			std::string	meta("-- QUERY SIZE: ");
			meta += std::to_string(len) + "\n";
			write(outfile_, meta.c_str(), meta.size());
			write(outfile_, &buf_[5], len);
			write(outfile_, "\n", 1);
		}
	}
};


class Server {
public:
	Server(const Config &cfg) {
		create_listen_socket(cfg);
		create_db_mask(cfg);
		logfile_ = open(cfg.getSection("LOGS_PATH").getStringVal("file").c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0644);
	}

	virtual ~Server() {
		close(listen_fd_);
	}

	[[noreturn]] void run_server() {
		bool is_any_set;
		while (true) {
			manage_client_fd();
			select(max_fd_ + 1, &read_fds_, &write_fds_, nullptr, nullptr);
			create_client();
			auto it = bridges_.begin();
			while (it != bridges_.end()) {

				is_any_set =			FD_ISSET((*it)->getFdClientProxy(), &read_fds_)	||
													FD_ISSET((*it)->getFdProxyDb(), &write_fds_)			||
													FD_ISSET((*it)->getFdProxyDb(), &read_fds_)			||
													FD_ISSET((*it)->getFdClientProxy(), &write_fds_);

				(*it)->caller(is_any_set);
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


	void	manage_client_fd() {
		FD_ZERO(&read_fds_);
		FD_ZERO(&write_fds_);
		FD_SET(listen_fd_, &read_fds_);
		max_fd_ = listen_fd_;
		for (auto &item: bridges_) {
			switch (item->getCurState()) {
				case state::FINALL:
					return;
				case state::READ_FROM_CLIENT:
					FD_SET(item->getFdClientProxy(), &read_fds_);
//					max_fd_ = std::max(listen_fd_, item->getFdClientProxy());
					break;
				case state::SEND_TO_CLIENT:
					FD_SET(item->getFdClientProxy(), &write_fds_);
//					max_fd_ = std::max(listen_fd_, item->getFdClientProxy());
					break;
				case state::READ_FROM_DB:
					FD_SET(item->getFdProxyDb(), &read_fds_);
//					max_fd_ = std::max(listen_fd_, item->getFdProxyDb());
					break;
				case state::SEND_TO_DB:
					FD_SET(item->getFdProxyDb(), &write_fds_);
//					max_fd_ = std::max(listen_fd_, item->getFdProxyDb());
					break;
			}
			max_fd_ = std::max(
							*std::max_element(write_fds_.fds_bits, write_fds_.fds_bits + bridges_.size()),
							*std::max_element(read_fds_.fds_bits, read_fds_.fds_bits + bridges_.size())
							);
		}
	}

	void	create_client() {
		int		new_client_fd;
		if (FD_ISSET(listen_fd_, &read_fds_)) {
			if ((new_client_fd = accept(listen_fd_, nullptr, nullptr)) < 0) {
				std::cerr << "Cant`t accept client" << std::endl;
				return;
			}
			fcntl(new_client_fd, F_SETFL, O_NONBLOCK);
			bridges_.emplace_back(new Bridge(client_mask_, new_client_fd, logfile_));
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
	int																	logfile_;
};



int main(int argc, char** argv) {
	Server	serv(Config("./config.txt"));
	serv.run_server();
	return 0;
}

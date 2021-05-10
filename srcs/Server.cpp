//
// Created by Gilberto Dougal on 5/10/21.
//

#include "Server.hpp"

Server::Server(const Config &cfg) {
	create_listen_socket(cfg);
	create_db_mask(cfg);
	logfile_ = open(cfg.getSection("LOGS_PATH").getStringVal("file").c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0644);
	if (logfile_ < 0)
		throw std::runtime_error("Can't create file");
}

Server::~Server() {
	close(listen_fd_);
}

void Server::run_server() {
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


void Server::manage_client_fd() {
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
				break;
			case state::SEND_TO_CLIENT:
				FD_SET(item->getFdClientProxy(), &write_fds_);
				break;
			case state::READ_FROM_DB:
				FD_SET(item->getFdProxyDb(), &read_fds_);
				break;
			case state::SEND_TO_DB:
				FD_SET(item->getFdProxyDb(), &write_fds_);
				break;
		}
	}
	max_fd_ = std::max(
					*std::max_element(write_fds_.fds_bits, write_fds_.fds_bits + bridges_.size()),
					*std::max_element(read_fds_.fds_bits, read_fds_.fds_bits + bridges_.size())
	);
}

void Server::create_client() {
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

void Server::close_fd_error(std::string except) {
	if (close(listen_fd_) < 0)
		throw std::runtime_error(except.append(" and error close"));
	throw std::runtime_error(except);
}

void Server::create_listen_socket(const Config &cfg) {
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

void Server::create_db_mask(const Config &cfg) {
	std::memset(&client_mask_, 0, sizeof(client_mask_));
	client_mask_.sin_family = AF_INET;
	client_mask_.sin_addr.s_addr = cfg.getSection("DB").getAddrVal("ip");
	client_mask_.sin_port = cfg.getSection("DB").getPortVal("port");
}

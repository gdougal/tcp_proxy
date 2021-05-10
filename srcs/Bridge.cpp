//
// Created by Gilberto Dougal on 5/10/21.
//

#include "Bridge.hpp"

Bridge::Bridge(sockaddr_in &info, int client_to_proxy, int file)  :
				fd_client_proxy_(client_to_proxy),
				tmp_((new char[PORTION_SIZE])),
				outfile_(file) {
	fd_proxy_db_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if((connect(fd_proxy_db_, (sockaddr *)&info, sizeof(info))) == -1) {
		std::cerr << "Client cant`t accept client" << std::endl;
	}
	cur_state_ = state::READ_FROM_CLIENT;
	handler_ = std::bind(&Bridge::read_from_client, this);
}

Bridge::~Bridge() {
	close(fd_proxy_db_);
	close(fd_client_proxy_);
}

void Bridge::caller(bool a) {
	if (a == true)
		handler_();
}

int Bridge::getFdClientProxy() const { return fd_client_proxy_; };
int Bridge::getFdProxyDb() const { return fd_proxy_db_; };
enum state Bridge::getCurState() const { return cur_state_; };

void Bridge::send_to_db() {
	logger();
	if(send(fd_proxy_db_, bufer_.data(), bufer_.size(), 0) < 0) {
		cur_state_ = state::FINALL;
		return;
	}
	cur_state_ = state::READ_FROM_DB;
	handler_ = std::bind(&Bridge::read_from_db, this);
	bufer_.clear();
}

void Bridge::read_from_client() {
	int	cur_read;
	cur_read = recv(fd_client_proxy_, tmp_.get(), PORTION_SIZE, 0);
	if (cur_read <= 0) {
		cur_state_ = state::FINALL;
	}
	bufer_.insert(bufer_.end(), tmp_.get(), tmp_.get() + cur_read);
	if (!package_len() || bufer_.size() == package_len()) {
		handler_ = std::bind(&Bridge::send_to_db, this);
		cur_state_ = state::SEND_TO_DB;
	}
}

void Bridge::send_to_client() {
	if (send(fd_client_proxy_, bufer_.data(), bufer_.size(), 0) < 0) {
		cur_state_ = state::FINALL;
		return ;
	}
	handler_ = std::bind(&Bridge::read_from_client, this);
	cur_state_ = state::READ_FROM_CLIENT;
	bufer_.clear();
};

void Bridge::read_from_db() {
	int	cur_read = PORTION_SIZE;
	while (cur_read == PORTION_SIZE) {
		cur_read = recv(fd_proxy_db_, tmp_.get(), PORTION_SIZE, 0);
		if (cur_read <= 0) {
			cur_state_ = state::FINALL;
			return;
		}
		bufer_.insert(bufer_.end(), tmp_.get(), tmp_.get() + cur_read);
	}
	handler_ = std::bind(&Bridge::send_to_client, this);
	cur_state_ = state::SEND_TO_CLIENT;
}

uint Bridge::package_len() {
	constexpr uint	pg_package_ = 1;
	uint				len = 0;
	if (bufer_.size() > 1 && std::isalpha(bufer_[0])) {
		len = (uint(bufer_[1]) << 24) | (uint(bufer_[2]) << 16) | (uint(bufer_[3]) << 8) | (uint8_t(bufer_[4])) + pg_package_;
		return len;
	}
	return 0;
}

void Bridge::logger() {
	constexpr uint	not_logging_bytes = 6;
	if (bufer_.size() && bufer_.at(0) == 'Q') {
		uint len = package_len() - not_logging_bytes;
		if (len < bufer_.size()) {
			std::string meta("-- QUERY SIZE: ");
			meta += std::to_string(len) + "\n";
			write(outfile_, meta.c_str(), meta.size());
			write(outfile_, &(bufer_[5]), len);
			write(outfile_, "\n", 1);
		}
	}
}

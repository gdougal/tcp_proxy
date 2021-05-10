//
// Created by Gilberto Dougal on 5/10/21.
//

#ifndef PROXY_SERVER_BRIDGE_HPP
#define PROXY_SERVER_BRIDGE_HPP

#include <iostream>
#include <list>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <csignal>
#include <vector>
#define PORTION_SIZE	512

enum state {
	READ_FROM_CLIENT,
	SEND_TO_DB,
	READ_FROM_DB,
	SEND_TO_CLIENT,
	FINALL
};

class Bridge {
	enum state															cur_state_;
	int																			fd_client_proxy_;
	int																			fd_proxy_db_;
	int																			outfile_;
	std::vector<char>												bufer_;
	std::function<void()>										handler_;
	std::shared_ptr<char> 									tmp_;
public:
	Bridge(sockaddr_in &info, int client_to_proxy, int file);
	virtual ~Bridge();
	void				caller(bool a);
	int					getFdClientProxy()	const;
	int					getFdProxyDb()			const;
	enum state	getCurState()				const;

private:

	void	send_to_db();
	void	read_from_client();
	void	send_to_client();
	void	read_from_db();
	uint	package_len();
	void	logger();
};



#endif //PROXY_SERVER_BRIDGE_HPP

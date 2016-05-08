#define _WIN32_WINNT 0x0601

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

#include "slavetcpconnection.hpp"

using boost::asio::ip::tcp;

typedef unsigned short uint16_t;

class SlaveServer
{
public:
	SlaveServer(uint16_t port);
	~SlaveServer();
	void run();

private:
	uint16_t port;
	tcp::acceptor *acceptor_;
	boost::asio::io_service *io_service;

	void start_accept();
	void handle_accept(SlaveTCPConnection::pointer new_connection,
      const boost::system::error_code& error);
};
#include "slaveserver.hpp"

SlaveServer::SlaveServer(uint16_t port)
: port(port)
{}

void SlaveServer::run()
{
	io_service = new boost::asio::io_service();
	acceptor_ = new tcp::acceptor(*io_service, tcp::endpoint(tcp::v4(), port));
	start_accept();
    io_service->run();
}

void SlaveServer::start_accept()
{
	SlaveTCPConnection::pointer new_connection =
      SlaveTCPConnection::create(*io_service);

    acceptor_->async_accept(new_connection->socket(),
        boost::bind(&SlaveServer::handle_accept, this, new_connection,
          boost::asio::placeholders::error));
}

void SlaveServer::handle_accept(SlaveTCPConnection::pointer new_connection,
      const boost::system::error_code& error)
{
	if (!error)
	{
		new_connection->start();
		start_accept();
	}
}

SlaveServer::~SlaveServer()
{
	delete acceptor_;
	delete io_service;
}
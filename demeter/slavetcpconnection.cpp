#include "slavetcpconnection.hpp"

SlaveTCPConnection::pointer SlaveTCPConnection::create(boost::asio::io_service& io_service)
{
	return SlaveTCPConnection::pointer(new SlaveTCPConnection(io_service));
}

tcp::socket& SlaveTCPConnection::socket()
{
	return socket_;
}

void SlaveTCPConnection::start()
{
	socket_.async_read_some(boost::asio::buffer(buffer),
		boost::bind(&SlaveTCPConnection::handle_read, shared_from_this(),
			boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	//message_ = "12345";
	//boost::asio::async_write(socket_, boost::asio::buffer(message_),
    //    boost::bind(&SlaveTCPConnection::handle_write, shared_from_this(),
    //      boost::asio::placeholders::error,
    //      boost::asio::placeholders::bytes_transferred));
}

void SlaveTCPConnection::handle_read(const boost::system::error_code& error,
      size_t bytes_transferred)
{
	buffer[bytes_transferred] = 0;
	message_ = std::string(buffer);
	boost::asio::async_write(socket_, boost::asio::buffer(message_),
        boost::bind(&SlaveTCPConnection::handle_write, shared_from_this(),
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
}

void SlaveTCPConnection::handle_write(const boost::system::error_code& error,
      size_t bytes_transferred)
{}
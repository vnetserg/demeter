#define _WIN32_WINNT 0x0601

#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "logging.h"
#include "slavegenerator.hpp"

using boost::asio::ip::tcp;

class SlaveTCPConnection
  : public boost::enable_shared_from_this<SlaveTCPConnection>
{
public:
	typedef boost::shared_ptr<SlaveTCPConnection> pointer;

	static pointer create(boost::asio::io_service& io_service);
	tcp::socket& socket();
	void start();

private:
	tcp::socket socket_;
	std::string message_;
	boost::asio::streambuf input_buffer;
	SlaveGenerator::pointer slave_generator;
	boost::asio::deadline_timer *timer;

	SlaveTCPConnection(boost::asio::io_service& io_service)
		: socket_(io_service)
	{
	}

	void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
	void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
	void ttl_expired();
};
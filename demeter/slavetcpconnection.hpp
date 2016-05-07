#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

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
	char buffer[3];

	SlaveTCPConnection(boost::asio::io_service& io_service)
		: socket_(io_service)
	{
	}

	void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
	void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
};
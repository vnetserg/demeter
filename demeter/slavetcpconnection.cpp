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
	INFO << "<" << boost::lexical_cast<std::string>(socket_.remote_endpoint()) << 
		"> Установлено TCP-соединение";

	boost::asio::async_read_until(socket_, input_buffer, '\000',
		boost::bind(&SlaveTCPConnection::handle_read, shared_from_this(),
			boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void SlaveTCPConnection::handle_read(const boost::system::error_code& error,
      size_t bytes_transferred)
{
	std::string client = boost::lexical_cast<std::string>(socket_.remote_endpoint());
	if (bytes_transferred == 0)
	{
		WARN << "<" << client << "> Соединение разорвано клиентом";
		return;
	}
	
	boost::asio::streambuf::const_buffers_type bufs = input_buffer.data();
	std::string str(boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + input_buffer.size()-1);
	std::stringstream ss(str);

	boost::property_tree::ptree pt;
	try {
		boost::property_tree::read_json(ss, pt);
	} catch(std::exception&) {
		WARN << "<" << client << "> Получен невалидный JSON";
		WARN << "<" << client << "> Сброс соединения";
		socket_.close();
		return;
	}

	slave_generator = SlaveGenerator::create(pt, socket_.get_io_service());
	if (!slave_generator->isValid())
	{
		WARN << "<" << client << "> Получено невалидное описание потока";
		WARN << "<" << client << "> Сброс соединения";
		socket_.close();
		return;
	}
	slave_generator->run();

	uint16_t port = slave_generator->getPort();
	char reply[3];
	reply[0] = port % 256;
	reply[1] = port / 256;
	reply[2] = 0;

	message_ = std::string(reply);
	boost::asio::async_write(socket_, boost::asio::buffer(message_),
        boost::bind(&SlaveTCPConnection::handle_write, shared_from_this(),
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
	
	uint16_t ttl = slave_generator->getTtl();
	timer = new boost::asio::deadline_timer(socket_.get_io_service());
	timer->expires_from_now(boost::posix_time::seconds(ttl));
	timer->async_wait(boost::bind(&SlaveTCPConnection::ttl_expired, shared_from_this()));

	INFO << "<" << client << "> Выделен UDP-порт " << port << " на " << ttl << " сек";
}

void SlaveTCPConnection::handle_write(const boost::system::error_code& error,
      size_t bytes_transferred)
{
	INFO << "<" << boost::lexical_cast<std::string>(socket_.remote_endpoint())
		<< "> Завершение TCP-сессии";
	socket_.close();
}

void SlaveTCPConnection::ttl_expired()
{
	INFO << "<UDP:" << slave_generator->getPort() << "> TTL истёк";
	slave_generator->expire();
	delete timer;
}
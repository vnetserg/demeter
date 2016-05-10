#include "mastergenerator.hpp"

MasterGenerator::MasterGenerator(std::string address, uint16_t tcp_port,
				uint16_t flows, uint16_t ttl, std::string infile)
	: address(address), tcp_port(tcp_port),
	flows(flows), ttl(ttl), infile(infile)
{
	if (!boost::filesystem::exists(infile))
		throw std::runtime_error("файл описания потока не существует");

	ptree pt;
	try {
		boost::property_tree::read_json(infile, pt);
	} catch(std::exception&) {
		throw std::runtime_error("невалидный JSON в файле описания потока");
	}

	try {
		parseTree(pt);
	}
	catch(std::exception) {
		throw std::runtime_error("некорректное описание потока");
	}

	ttl_timer = new boost::asio::deadline_timer(io_service);
}

void MasterGenerator::parseTree(ptree pt)
{
	cycle = pt.get<bool>("cycle");
	
	ptree slave_tree;
	slave_tree.put("cycle", "%cycle%");
	slave_tree.put("ttl", "%ttl%");
	slave_tree.put("length", "%length%");
	slave_tree.put("script", "%script%");
	std::stringstream script;
	script << "[";
	bool need_comma = false;

	length = -1;
	BOOST_FOREACH(boost::property_tree::ptree::value_type &v, pt.get_child("script"))
	{
		std::pair< ptree::const_assoc_iterator,
               ptree::const_assoc_iterator > bounds = v.second.equal_range("");
		
		double ts = bounds.first->second.get_value<double>();
		if (ts < 0 || ts < length)
			throw std::exception();
		length = ts;

		std::advance(bounds.first, 1);
		uint16_t sz = bounds.first->second.get_value<uint16_t>();
		if (sz > 1500)
			throw std::exception();

		std::advance(bounds.first, 1);
		int dir = bounds.first->second.get_value<int>();
		if (dir != 0 && dir != 1)
			throw std::exception();
		
		if (dir == 0)
			segments.push_back(segment(ts, sz));
		else
		{
			if (need_comma)
				script << ",";
			need_comma = true;
			script << "[" << ts << "," << sz << "]";
		}
    }
	script << "]";

	if (segments.size() == 0 || !need_comma)
		throw std::exception();

	std::stringstream ss;
	boost::property_tree::write_json(ss, slave_tree);
	slave_json = ss.str();
	
	boost::replace_all(slave_json, "\"%script%\"", script.str());
	boost::replace_all(slave_json, "\"%cycle%\"", boost::lexical_cast<std::string>(cycle));
	boost::replace_all(slave_json, "\"%ttl%\"", boost::lexical_cast<std::string>(ttl));
	boost::replace_all(slave_json, "\"%length%\"", 
		boost::replace_all_copy(boost::lexical_cast<std::string>(length), ",", "."));
	boost::replace_all(slave_json, " ", "");
	boost::replace_all(slave_json, "\r", "");
	boost::replace_all(slave_json, "\n", "");

	slave_json.push_back('\000');
	//INFO << slave_json;
}

void MasterGenerator::run()
{
	namespace ip = boost::asio::ip;
	ip::tcp::socket socket(io_service);
	ip::tcp::endpoint endpoint = ip::tcp::endpoint(
		ip::address::from_string(address), tcp_port);
	
	try {
		socket.connect(endpoint);
	} catch (std::exception) {
		ERR << "Не удалось установить соединение с сервером";
		return;
	}

	INFO << "Установлено соединение со сервером";

	socket.write_some(boost::asio::buffer(slave_json));
	INFO << "Отправлено описание потока данных";

	boost::system::error_code error;
	size_t len = socket.read_some(boost::asio::buffer(buffer, 2), error);

	if (len == 0) {
		ERR << "Соединение разорвано сервером";
		socket.close();
		return;
	}
	if (len != 2) {
		ERR << "Получено " << len << " байт ответа (ожидалось 2)";
		INFO << "Завершение соединения";
		socket.close();
		return;
	}

	uint16_t udp_port = buffer[1] * 256 + buffer[0];
	INFO << "Получен UDP-порт: " << udp_port;
	INFO << "Завершение TCP-соединения";
	socket.close();

	slave_endpoint = ip::udp::endpoint(
		ip::address::from_string(address), udp_port);
	base_time = boost::posix_time::second_clock::local_time();
	double ts = now();
	for (int i = 0; i < flows; i++)
	{
		boost::asio::deadline_timer *timer = new boost::asio::deadline_timer(io_service);
		ip::udp::socket *udp_socket = new ip::udp::socket(io_service, ip::udp::endpoint(ip::udp::v4(), 0));
		launchFlow(0, ts, udp_socket, timer);
	}

	INFO << "Запущены потоки трафика: " << flows;

	ttl_timer->expires_from_now(boost::posix_time::seconds(ttl));
	ttl_timer->async_wait(boost::bind(&MasterGenerator::stop, shared_from_this()));

	io_service.run();
}

void MasterGenerator::launchFlow(int ind, double started, boost::asio::ip::udp::socket *udp_socket,
								 boost::asio::deadline_timer *timer)
{
	namespace ip = boost::asio::ip;
	
	uint16_t sz = segments[ind].second;
	udp_socket->async_send_to(boost::asio::buffer(buffer, sz), slave_endpoint,
		boost::bind(&MasterGenerator::handleSend, shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));

	if (ind == segments.size()-1)
	{
		if (!cycle)
		{
			timer->async_wait(boost::bind(&MasterGenerator::deleteSocket,
				shared_from_this(), udp_socket));
			udp_socket = new ip::udp::socket(io_service, ip::udp::endpoint(ip::udp::v4(), 0));
		}

		double ts = segments[0].first;
		double secsLeft = secsToMoment(started + length + ts);
		if (secsLeft > 0)
		{
			timer->expires_from_now(boost::posix_time::microseconds(secsLeft * 1000000));
			timer->async_wait(boost::bind(&MasterGenerator::launchFlow,
				shared_from_this(), 0, started+length, udp_socket, timer));
		}
		else
			launchFlow(0, started+length, udp_socket, timer);
	}
	else
	{
		double ts = segments[ind+1].first;
		double secsLeft = secsToMoment(started + ts);
		if (secsLeft > 0)
		{
			timer->expires_from_now(boost::posix_time::microseconds(secsLeft * 1000000));
			timer->async_wait(boost::bind(&MasterGenerator::launchFlow,
				shared_from_this(), ind+1, started, udp_socket, timer));
		}
		else
			launchFlow(ind+1, started, udp_socket, timer);
	}
}

void MasterGenerator::deleteSocket(boost::asio::ip::udp::socket *udp_socket)
{
	delete udp_socket;
}

void MasterGenerator::handleSend(const boost::system::error_code& error, std::size_t bytes_transferred)
{
	
}

double MasterGenerator::now()
{
	return (boost::posix_time::second_clock::local_time() - base_time).total_milliseconds() / 1000.0;
}

double MasterGenerator::secsToMoment(double ts)
{
	return ts - now();
}

void MasterGenerator::stop()
{
	INFO << "Генерация трафика завершена";
	io_service.stop();
}

MasterGenerator::~MasterGenerator()
{
	delete ttl_timer;
}

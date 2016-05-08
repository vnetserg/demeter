#include "slavegenerator.hpp"

SlaveGenerator::SlaveGenerator(ptree pt, boost::asio::io_service& io_service)
	: socket(io_service, udp::endpoint(udp::v4(), 0))
{
	base_time = boost::posix_time::second_clock::local_time();
	valid = true;
	try {
		parsePtree(pt);
	} catch(std::exception&) {
		valid = false;
	}
	if (segments.size() == 0)
		valid = false;
	port = socket.local_endpoint().port();
}

void SlaveGenerator::parsePtree(ptree pt)
{
	ttl = pt.get<int>("ttl");
	cycle = pt.get<bool>("cycle");
	length = pt.get<double>("length");
	BOOST_FOREACH(boost::property_tree::ptree::value_type &v, pt.get_child("script"))
	{
		std::pair< ptree::const_assoc_iterator,
               ptree::const_assoc_iterator > bounds = v.second.equal_range("");
		double ts = bounds.first->second.get_value<double>();
		std::advance(bounds.first, 1);
		uint16_t sz = bounds.first->second.get_value<uint16_t>();
		segments.push_back(segment(ts, sz));
    }
}

void SlaveGenerator::run()
{
	if (!valid) return;
	INFO << "<UDP:" << port << "> Запущен сервер";
	startReceive();
}

void SlaveGenerator::startReceive()
{
	if (valid)
		socket.async_receive_from(
			boost::asio::buffer(buffer), remote_endpoint,
			boost::bind(&SlaveGenerator::handleReceive, shared_from_this(),
			  boost::asio::placeholders::error,
			  boost::asio::placeholders::bytes_transferred));
}

double SlaveGenerator::now()
{
	return (boost::posix_time::second_clock::local_time() - base_time).total_milliseconds() / 1000.0;
}

double SlaveGenerator::secsToMoment(double ts)
{
	return ts - now();
}

void SlaveGenerator::handleReceive(const boost::system::error_code& error, std::size_t bytes_transferred)
{
	if (!valid) return;

	udp::endpoint client_endpoint = remote_endpoint;
	if (!active_endpoints.count(client_endpoint))
	{
		active_endpoints.insert(client_endpoint);
		boost::asio::deadline_timer *timer = new boost::asio::deadline_timer(socket.get_io_service());
		double ts = segments[0].first;
		if (ts > 0)
		{
			timer->expires_from_now(boost::posix_time::microseconds(ts * 1000000));
			timer->async_wait(boost::bind(&SlaveGenerator::launchFlow,
				shared_from_this(), 0, now(), timer, client_endpoint));
		}
		else
			launchFlow(0, now(), timer, client_endpoint);
	}
	startReceive();
}

void SlaveGenerator::launchFlow(int ind, double started,
								boost::asio::deadline_timer *timer, udp::endpoint client_endpoint)
{
	if (!valid)
	{
		delete timer;
		return;
	}

	uint16_t sz = segments[ind].second;

	socket.async_send_to(boost::asio::buffer(buffer, sz), client_endpoint,
		boost::bind(&SlaveGenerator::handleSend, shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));

	if (ind == segments.size()-1)
		if (cycle)
		{
			double ts = segments[0].first;
			double secsLeft = secsToMoment(started + length + ts);
			if (secsLeft > 0)
			{
				timer->expires_from_now(boost::posix_time::microseconds(secsLeft * 1000000));
				timer->async_wait(boost::bind(&SlaveGenerator::launchFlow,
					shared_from_this(), 0, started+length, timer, client_endpoint));
			}
			else
				launchFlow(0, started+length, timer, client_endpoint);
		}
		else
		{
			active_endpoints.erase(client_endpoint);
			delete timer;
			return;
		}
	else
	{
		double ts = segments[ind+1].first;
		double secsLeft = secsToMoment(started + ts);
		if (secsLeft > 0)
		{
			timer->expires_from_now(boost::posix_time::microseconds(secsLeft * 1000000));
			timer->async_wait(boost::bind(&SlaveGenerator::launchFlow,
				shared_from_this(), ind+1, started, timer, client_endpoint));
		}
		else
			launchFlow(ind+1, started, timer, client_endpoint);
	}
}

void SlaveGenerator::handleSend(const boost::system::error_code& error, std::size_t bytes_transferred)
{}

SlaveGenerator::~SlaveGenerator()
{
}

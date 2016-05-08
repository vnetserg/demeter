#pragma once
#define _WIN32_WINNT 0x0601

#include <boost/asio.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>

#include <utility>
#include <vector>
#include <set>
#include <ctime>

#include "logging.h"

typedef boost::property_tree::ptree ptree;
typedef unsigned short uint16_t;
typedef std::pair<double, uint16_t> segment;

using boost::asio::ip::udp;

class SlaveGenerator
  : public boost::enable_shared_from_this<SlaveGenerator>
{
public:
	typedef boost::shared_ptr<SlaveGenerator> pointer;
	static pointer create(ptree& pt, boost::asio::io_service& io_service)
		{ return pointer(new SlaveGenerator(pt, io_service)); }

	~SlaveGenerator();

	uint16_t getPort() { return port; }
	uint16_t getTtl() { return ttl; }
	bool isValid() { return valid; }
	void expire() { valid = false; }
	void run();

private:
	SlaveGenerator(ptree pt, boost::asio::io_service& io_service);
	void parsePtree(ptree pt);
	double now();
	double secsToMoment(double ts);
	void startReceive();
	void handleReceive(const boost::system::error_code& error, std::size_t bytes_transferred);
	void launchFlow(int ind, double started, boost::asio::deadline_timer *timer, udp::endpoint);
	void handleSend(const boost::system::error_code& error, std::size_t bytes_transferred);
	
	uint16_t port, ttl;
	double length;
	bool valid, cycle;
	std::vector<segment> segments;
	udp::socket socket;
	udp::endpoint remote_endpoint;

	std::set<udp::endpoint> active_endpoints;

	boost::posix_time::ptime base_time;

	char buffer[2048];
};

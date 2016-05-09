#pragma once
#define _WIN32_WINNT 0x0601

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/lexical_cast.hpp>

#include <utility>
#include <vector>

#include "logging.h"

typedef unsigned short uint16_t;
typedef std::pair<double, uint16_t> segment;
typedef boost::property_tree::ptree ptree;

class MasterGenerator
	: public boost::enable_shared_from_this<MasterGenerator>
{
public:
	typedef boost::shared_ptr<MasterGenerator> pointer;
	static pointer create(std::string address, uint16_t port,
		uint16_t flows, uint16_t time, std::string infile)
	{ return pointer(new MasterGenerator(address, port, flows, time, infile)); }

	~MasterGenerator();

	void run();

private:
	MasterGenerator(std::string address, uint16_t port,
		uint16_t flows, uint16_t ttl, std::string infile);
	void parseTree(ptree pt);

	std::string address, infile;
	uint16_t port, flows, ttl;

	std::vector<segment> segments;
	double length;
	bool cycle;

	boost::asio::io_service io_service;
	boost::asio::ip::tcp::socket *tcp_socket;

	std::string slave_json;
};

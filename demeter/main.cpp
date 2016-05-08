#define _WIN32_WINNT 0x0601

#include <string>
#include <iostream>
#include <fstream>

#include <boost/program_options.hpp>
#include <boost/regex.hpp>

#include "logging.h"
#include "slaveserver.hpp"

typedef unsigned short uint16_t;

struct Parameters {
	uint16_t serve;
	std::string connect;
	uint16_t port;
	uint16_t flows;
	uint16_t time;
	std::string infile;
};

int parameters (struct Parameters *params, int argc, char **argv);
void start(const struct Parameters &params);

int main(int argc, char **argv)
{
	try
	{
		setlocale(LC_ALL, "rus");

		struct Parameters params;
		//if (parameters(&params, argc, argv))
		//	return 0;

		params.serve = 10000;
		start(params);
	}
	catch(std::exception& e)
    {
        std::cerr << "������: " << e.what() << "\n";
        return 1;
    }
    catch(...)
    {
        std::cerr << "����������� ������!" << "\n";
        return 1;
    }

	return 0;
}

int parameters (struct Parameters *params, int argc, char **argv)
{
	namespace po = boost::program_options;

	po::options_description desc("��������� ��������� �������");
    desc.add_options()
        ("help", "������� ������ �����")
		("serve,s", po::value<uint16_t>(&(params->serve)),
			"����������� ��� slave �� ��������� �����")
		("connect,c", po::value<std::string>(&(params->connect)),
			"����������� �� slave �� ���������� IP")
		("port,p", po::value<uint16_t>(&(params->port)),
			"������� TCP-���� ��� ���������� �� slave")
		("flows,f", po::value<uint16_t>(&(params->flows))->default_value(10),
			"���������� ������� �������")
		("time,t", po::value<uint16_t>(&(params->time))->default_value(10),
			"������� ������ ������������ ������")
		("infile,i", po::value<std::string>(&(params->infile)),
			"���� � ���������������� ������������� ������")
    ;

    po::variables_map vm;        
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);    

    if (vm.count("help") || !vm.count("serve") && !vm.count("connect")) {
		std::cout << desc << "\n";
        return 1;
    }

	if (vm.count("serve") && vm.count("connect"))
		throw std::exception("������ ������������ ���� � master'��, � slave'�� (����� -s � -c)");

	if (vm.count("connect") && !vm.count("port"))
		throw std::exception("�� ������ ���� ��� ���������� �� slave (���� -p)");
	
	if (vm.count("connect") && !vm.count("infile"))
		throw std::exception("�� ������ ���� � ���������������� ������������� ������ (���� -i)");

	if (vm.count("serve") && params->serve == 0)
		throw std::exception("���������� ��������� ������ �� ����� 0");

	if (vm.count("connect"))
	{
		boost::regex ip("^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
		boost::match_results<std::string::const_iterator> what;
		if(0 == boost::regex_match(params->connect, what, ip, boost::match_default))
			throw std::exception("������ �������� IP-�����");
	}

	if (!vm.count("serve"))
		params->serve = 0;

	return 0;
}

void start(const struct Parameters &params)
{
	namespace kw = boost::log::keywords;
	namespace log = boost::log;
	namespace sinks = boost::log::sinks;

	INFO << "������ Demeter v0.00";

	if (params.serve)
	{
		INFO << "�����: Slave";
		INFO << "����: " << params.serve;

		SlaveServer ss(params.serve);
		ss.run();
	}
	else
	{
		INFO << "�����: Master";
		INFO << "������� �����: " << params.connect;
		INFO << "����: " << params.port;
		INFO << "����� �������: " << params.flows;
		INFO << "����������: " << params.time << " ���";
		INFO << "������� ����: " << params.infile;
	}
}

#include <string>
#include <iostream>
#include <fstream>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>

typedef unsigned short uint16_t;

struct Parameters {
	uint16_t serve;
	std::string connect;
	uint16_t port;
	uint16_t flows;
	uint16_t time;
	std::string infile;
	std::string logfile;
};

int parameters (struct Parameters *params, int argc, char **argv);
int start(const struct Parameters &params);

int main(int argc, char **argv)
{
	try
	{
		setlocale(LC_ALL, "rus");

		struct Parameters params;
		if (parameters(&params, argc, argv))
			return 0;

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
		("logfile,l", po::value<std::string>(&(params->logfile))->default_value("demeter.log"),
			"���� ��� ������ ����")
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

	if (vm.count("serve") && vm["serve"].as<uint16_t>() == 0)
		throw std::exception("���������� ��������� ������ �� ����� 0");

	if (!vm.count("serve"))
		params->serve = 0;

	return 0;
}

int start(const struct Parameters &params)
{
	using namespace boost::posix_time;
    using namespace boost::gregorian;

	std::fstream fs;
	fs.open(params.logfile.c_str(), std::fstream::app);

	if (fs.fail())
		throw std::exception("�� ������� ������� ���-����");

	ptime now = second_clock::local_time();
	fs << "[" << to_simple_string(now) << "] ������ Demeter v0.00\n";

	if (params.serve)
		fs << "�����: Slave\n"
			<< "����: " << params.serve << "\n\n";
	else
		fs << "�����: Master\n"
			<< "������� �����: " << params.connect << "\n"
			<< "����: " << params.port << "\n"
			<< "����� �������: " << params.flows << "\n"
			<< "����������: " << params.time << " ���\n"
			<< "������� ����: " << params.infile << "\n\n";

	fs.close();

	return 0;
}
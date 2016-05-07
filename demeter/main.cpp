#include <string>
#include <iostream>
#include <fstream>

#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include <boost/program_options.hpp>
#include <boost/regex.hpp>

#include "slaveserver.hpp"

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
        std::cerr << "Ошибка: " << e.what() << "\n";
        return 1;
    }
    catch(...)
    {
        std::cerr << "Неизвестная ошибка!" << "\n";
        return 1;
    }

	return 0;
}

int parameters (struct Parameters *params, int argc, char **argv)
{
	namespace po = boost::program_options;

	po::options_description desc("Доступные параметры запуска");
    desc.add_options()
        ("help", "вывести список опций")
		("serve,s", po::value<uint16_t>(&(params->serve)),
			"запуститься как slave на указанном порту")
		("connect,c", po::value<std::string>(&(params->connect)),
			"соединиться со slave по указанному IP")
		("port,p", po::value<uint16_t>(&(params->port)),
			"указать TCP-порт для соединения со slave")
		("flows,f", po::value<uint16_t>(&(params->flows))->default_value(10),
			"количество потоков трафика")
		("time,t", po::value<uint16_t>(&(params->time))->default_value(10),
			"сколько секунд генерировать трафик")
		("infile,i", po::value<std::string>(&(params->infile)),
			"файл с характеристиками генерируемого потока")
		("logfile,l", po::value<std::string>(&(params->logfile))->default_value("demeter.log"),
			"файл для записи лога")
    ;

    po::variables_map vm;        
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);    

    if (vm.count("help") || !vm.count("serve") && !vm.count("connect")) {
		std::cout << desc << "\n";
        return 1;
    }

	if (vm.count("serve") && vm.count("connect"))
		throw std::exception("нельзя одновременно быть и master'ом, и slave'ом (ключи -s и -c)");

	if (vm.count("connect") && !vm.count("port"))
		throw std::exception("не указан порт для соединения со slave (ключ -p)");
	
	if (vm.count("connect") && !vm.count("infile"))
		throw std::exception("не указан файл с характеристиками генерируемого потока (ключ -i)");

	if (vm.count("serve") && params->serve == 0)
		throw std::exception("невозможно запустить сервер на порту 0");

	if (vm.count("connect"))
	{
		boost::regex ip("^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
		boost::match_results<std::string::const_iterator> what;
		if(0 == boost::regex_match(params->connect, what, ip, boost::match_default))
			throw std::exception("указан неверный IP-адрес");
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

	log::add_file_log(
        kw::file_name = params.logfile,
        kw::rotation_size = 10 * 1024 * 1024,
        kw::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
        kw::format = "[%TimeStamp%]: %Message%"
    );
	log::add_common_attributes();

	BOOST_LOG_TRIVIAL(info) << "Запуск Demeter v0.00";

	if (params.serve)
	{
		BOOST_LOG_TRIVIAL(info) << "Режим: Slave";
		BOOST_LOG_TRIVIAL(info)	<< "Порт: " << params.serve;

		SlaveServer ss(params.serve);
		ss.run();
	}
	else
	{
		BOOST_LOG_TRIVIAL(info) << "Режим: Master";
		BOOST_LOG_TRIVIAL(info)	<< "Сетевой адрес: " << params.connect;
		BOOST_LOG_TRIVIAL(info)	<< "Порт: " << params.port;
		BOOST_LOG_TRIVIAL(info)	<< "Число потоков: " << params.flows;
		BOOST_LOG_TRIVIAL(info)	<< "Активность: " << params.time << " сек";
		BOOST_LOG_TRIVIAL(info)	<< "Входной файл: " << params.infile;
	}
}

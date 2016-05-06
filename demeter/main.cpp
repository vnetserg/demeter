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

	if (vm.count("serve") && vm["serve"].as<uint16_t>() == 0)
		throw std::exception("невозможно запустить сервер на порту 0");

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
		throw std::exception("не удалось открыть лог-файл");

	ptime now = second_clock::local_time();
	fs << "[" << to_simple_string(now) << "] Запуск Demeter v0.00\n";

	if (params.serve)
		fs << "Режим: Slave\n"
			<< "Порт: " << params.serve << "\n\n";
	else
		fs << "Режим: Master\n"
			<< "Сетевой адрес: " << params.connect << "\n"
			<< "Порт: " << params.port << "\n"
			<< "Число потоков: " << params.flows << "\n"
			<< "Активность: " << params.time << " сек\n"
			<< "Входной файл: " << params.infile << "\n\n";

	fs.close();

	return 0;
}
#include "mastergenerator.hpp"

MasterGenerator::MasterGenerator(std::string address, uint16_t port,
				uint16_t flows, uint16_t ttl, std::string infile)
	: address(address), port(port), flows(flows),
	ttl(ttl), infile(infile)
{
	if (!boost::filesystem::exists(infile))
		throw std::exception("файл описания потока не существует");

	ptree pt;
	try {
		boost::property_tree::read_json(infile, pt);
	} catch(std::exception&) {
		throw std::exception("невалидный JSON в файле описания потока");
	}

	try {
		parseTree(pt);
	}
	catch(std::exception) {
		throw std::exception("некорректное описание потока");
	}
}

void MasterGenerator::parseTree(ptree pt)
{
	cycle = pt.get<bool>("cycle");
	
	ptree slave_tree;
	slave_tree.put("cycle", "%cycle%");
	slave_tree.put("ttl", "%ttl%");
	slave_tree.put("length", "%length%");
	slave_tree.put("script", "%script%");
	std::stringstream script("[");
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

	//INFO << slave_json;
}

void MasterGenerator::run()
{
	
}

MasterGenerator::~MasterGenerator()
{
}

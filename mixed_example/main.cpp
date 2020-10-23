#include "verbs.hpp"

#include <boost/program_options.hpp>

#include <iostream>

namespace po = boost::program_options;

int run_client(const po::variables_map& _vm);

int run_server(const po::variables_map& _vm);

int main(int _argc, char* _argv[])
{
    try {
        po::options_description desc{"Options"};
        desc.add_options()
            ("server,s", po::bool_switch(), "Run application as a server.")
            ("host,h", po::value<std::string>(), "The host to connect to.")
            ("port,p", po::value<std::string>()->default_value("9090"), "The port to connect to.")
            ("help", po::bool_switch(), "Show this message.");

        po::variables_map vm;
        po::store(po::parse_command_line(_argc, _argv, desc), vm);
        po::notify(vm);

        if (vm["help"].as<bool>()) {
            std::cout << desc << '\n';
            return 0;
        }

        if (vm["server"].as<bool>()) {
            return run_server(vm);
        }
        
        return run_client(vm);
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
    }

    return 0;
}

int run_client(const po::variables_map& _vm)
{
    const auto host = _vm["host"].as<std::string>();
    const auto port = _vm["port"].as<std::string>();

    return 0;
}

int run_server(const po::variables_map& _vm)
{
    const auto port = _vm["port"].as<std::string>();

    return 0;
}


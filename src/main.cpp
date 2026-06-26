// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>
#include "EmulationSession.h"

namespace po = boost::program_options;

po::options_description get_config_file_options()
{
    po::options_description desc("Configuration File Options");
    desc.add_options()
        ("BIOS", po::value<std::string>()->required(), "Game Boy BIOS")
        ("Cartridge", po::value<std::string>()->required(), "Cartridge to load");
    return desc;
}

int main()
{
    try
    {
        // Build our system
        EmulationSession session;

        // Process configuration file, exit if there are any errors as we won't know how to boot the system
        std::ifstream configFile("GameBoy.cfg");
        if (!configFile)
        {
            std::cerr << "Error: Unable to open configuration file GameBoy.cfg exiting!" << std::endl;
            return 1;
        }

        po::variables_map vmConfig;
        po::options_description configFileOptions = get_config_file_options();

        // Validate required options before moving on
        try
        {
            po::store(po::parse_config_file(configFile, configFileOptions), vmConfig);
            po::notify(vmConfig);
        }
        catch (const po::unknown_option& e)
        {
            std::cerr << "Unknown option encountered: " << e.what() << std::endl;
            return 1;
        }
        catch (const po::required_option& e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }

        session.setBIOSPath(vmConfig["BIOS"].as<std::string>());
        session.setCartridgePath(vmConfig["Cartridge"].as<std::string>());
        session.run();

        return 0;
    }
    catch (const std::runtime_error& error)
    {
        std::cerr << "Error: Runtime error exception: " << error.what() << std::endl;
        return 1;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Error: General exception: " << error.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Error: Caught an unknown exception!" << std::endl;
        return 1;
    }
}

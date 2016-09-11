/*
 * Copyright (C) 2015-2016 Olzhas Rakhimov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <QApplication>

#include <boost/exception/all.hpp>
#include <boost/program_options.hpp>

#include "src/config.h"
#include "src/error.h"
#include "src/initializer.h"
#include "src/model.h"
#include "src/settings.h"

#include "mainwindow.h"

namespace po = boost::program_options;

namespace {

/**
 * Parses the command-line arguments.
 *
 * @param[in] argc  Count of arguments.
 * @param[in] argv  Values of arguments.
 * @param[out] vm  Variables map of program options.
 *
 * @returns 0 for success.
 * @returns 1 for errored state.
 * @returns -1 for information only state like help and version.
 */
int parseArguments(int argc, char *argv[], po::variables_map *vm)
{
    std::string usage = "Usage:    scram-gui [input-files] [options]";
    po::options_description desc("Options");
    desc.add_options()
            ("help", "Display this help message")
            ("input-files", po::value<std::vector<std::string>>(),
             "XML input files with analysis constructs")
            ("config-file", po::value<std::string>(),
             "XML file with analysis configurations");
    try {
        po::store(po::parse_command_line(argc, argv, desc), *vm);
    } catch (std::exception &err) {
        std::cerr << "Option error: " << err.what() << "\n\n"
                  << usage << "\n\n"
                  << desc << "\n";
        return 1;
    }

    po::notify(*vm);

    po::positional_options_description p;
    p.add("input-files", -1);

    po::store(
        po::command_line_parser(argc, argv).options(desc).positional(p).run(),
        *vm);
    po::notify(*vm);

    // Process command-line arguments.
    if (vm->count("help")) {
        std::cout << usage << "\n\n" << desc << "\n";
        return -1;
    }

    if (!vm->count("input-files") && !vm->count("config-file")) {
        std::cerr << "No input or configuration file is given.\n\n";
        std::cerr << usage << "\n\n" << desc << "\n";
        return 1;
    }
    return 0;
}

/**
 * Main body of command-line entrance to run the program.
 *
 * @param[in] vm  Variables map of program options.
 *
 * @return The initialized model with analysis containers.
 *
 * @throws Error  Internal problems specific to SCRAM like validation.
 * @throws boost::exception  Boost errors.
 * @throws std::exception  All other problems.
 */
std::shared_ptr<scram::mef::Model> acceptCmdLine(const po::variables_map &vm)
{
    scram::core::Settings settings;
    std::vector<std::string> inputFiles;
    // Get configurations if any.
    // Invalid configurations will throw.
    if (vm.count("config-file")) {
        auto config = std::make_unique<scram::Config>(
            vm["config-file"].as<std::string>());
        settings = config->settings();
        inputFiles = config->input_files();
    }

    // Add input files from the command-line.
    if (vm.count("input-files")) {
        auto cmdInput = vm["input-files"].as<std::vector<std::string>>();
        inputFiles.insert(inputFiles.end(), cmdInput.begin(), cmdInput.end());
    }
    // Process input files
    // into valid analysis containers and constructs.
    // Validation phase happens upon processing.
    auto init = std::make_unique<scram::mef::Initializer>(inputFiles, settings);

    return init->model();
}

} // namespace

int main(int argc, char *argv[])
{
    std::shared_ptr<scram::mef::Model> model;
    if (argc > 1) {
        try {
            po::variables_map vm;
            int ret = parseArguments(argc, argv, &vm);
            if (ret == 1)
                return 1;
            if (ret == -1)
                return 0;
            model = acceptCmdLine(vm);
        } catch (scram::Error &scramErr) {
            std::cerr << "SCRAM Error\n" << scramErr.what() << std::endl;
            return 1;
        } catch (boost::exception &boostErr) {
            std::cerr << "Boost Exception:\n"
                      << boost::diagnostic_information(boostErr) << std::endl;
            return 1;
        } catch (std::exception &stdErr) {
            std::cerr << "Standard Exception:\n" << stdErr.what() << std::endl;
            return 1;
        }
    }

    QApplication a(argc, argv);
    scram::gui::MainWindow w;
    w.setModel(std::move(model));
    w.show();

    return a.exec();
}

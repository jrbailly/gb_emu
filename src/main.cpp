#include "application.h"
#include "config.h"
#include <cstdlib>
#include <cxxopts.hpp>
#include <iostream>

/**
 * @brief Parses command line arguments.
 *
 * @param argc Number of command line arguments.
 * @param argv Array of command line arguments.
 * @param config Structure to store parsed configuration.
 * @return True if parsing was successful, false otherwise (e.g., help requested).
 */
bool ParseCommandLine(int argc, char **argv, Config &config)
{
    cxxopts::Options options(argv[0]);

    options.add_options()("f,filename", "ROM file", cxxopts::value<std::string>())(
        "s,screen_scale", "Screen size", cxxopts::value<int>()->default_value("3"))(
        "a,audio_filter", "Sound High Pass Filter", cxxopts::value<int>()->default_value("1"))(
        "r,record_file", "Inputs record file", cxxopts::value<std::string>())("h,help", "Help");

    auto result = options.parse(argc, argv);

    if (result.count("help"))
    {
        std::cout << options.help() << std::endl;
        return false;
    }
    if (result.count("filename"))
        config._romfile = result["filename"].as<std::string>();
    config._screen_scale = result["screen_scale"].as<int>();
    if (result.count("record_file"))
        config._recordfile = result["record_file"].as<std::string>();
    config._audio_filter = result["audio_filter"].as<int>();
    return true;
}

/**
 * @brief Main entry point of the application.
 *
 * @param argc Number of command line arguments.
 * @param argv Array of command line arguments.
 * @return 0 if the application runs successfully.
 */
int main(int argc, char **argv)
{
    Config Configuration;
    Application App;

    try
    {
        // Configuration._romfile = "d:\\workspace\\gb_emu\\Super Mario Land (World).gb";
        // Configuration._recordfile = "d:\\workspace\\gb_emu\\inputs.txt";
        if (!ParseCommandLine(argc, argv, Configuration))
            return (EXIT_SUCCESS);
        App.MainLoop(Configuration);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return (EXIT_FAILURE);
    }
    return (EXIT_SUCCESS);
}

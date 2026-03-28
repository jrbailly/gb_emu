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
bool parse_command_line(int argc, char **argv, Config &config)
{
    cxxopts::Options options(argv[0]);

    options.add_options()("f,filename", "ROM file", cxxopts::value<std::string>())(
        "s,screen_scale", "Screen size", cxxopts::value<int>()->default_value("3"))(
        "a,audio_filter", "Sound High Pass Filter", cxxopts::value<int>()->default_value("1"))(
        "r,record_file", "Inputs record file", cxxopts::value<std::string>())("h,help", "Help")(
        "v,version", "Show version");

    auto result = options.parse(argc, argv);

    if (result.count("filename"))
        config._romfile = result["filename"].as<std::string>();
    config._screen_scale = result["screen_scale"].as<int>();
    if (result.count("record_file"))
        config._recordfile = result["record_file"].as<std::string>();
    config._audio_filter = result["audio_filter"].as<int>();
    if (result.count("version"))
    {
        std::cout << app_version << std::endl;
        return false;
    }
    if (result.count("help") || config._romfile.empty())
    {
        std::cout << options.help() << std::endl;
        return false;
    }
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
    Config configuration;
    Application app;

    try
    {
        if (!parse_command_line(argc, argv, configuration))
            return (EXIT_SUCCESS);
        app.init(configuration);
        app.main_loop();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return (EXIT_FAILURE);
    }
    return (EXIT_SUCCESS);
}

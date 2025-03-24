#include <iostream>

#include "application.h"
#include "config.h"

bool ParseCommandLine(int argc, char **argv, Config &config)
{
    int option = 0;
    bool Valid = false;

    /*while ((option = getopt(argc, argv, "f:")) != -1)
    {
        switch (option)
        {
        case 'f':
            config.mRomFile = optarg;
            Valid = true;
            break;
        default:
            break;
        }
    }*/
    if (!Valid)
        std::cout << argv[0] << " -f <filename>" << std::endl;
    return (Valid);
}

int main(int argc, char **argv)
{
    Config Configuration;
    Application App;

    // if (ParseCommandLine(argc, argv, Configuration))
    // Configuration.mRomFile = "/media/data/workspace/gb_emu/Mario's Picross (USA, Europe) (SGB Enhanced).gb";
    //    Configuration.mRomFile = "/media/data/workspace/gb_emu/cpu_instrs.gb";
    Configuration.mRomFile = "/media/data/workspace/gb_emu/Super Mario Land (World).gb";
    // Configuration.mRomFile = "d:\\workspace\\gb_emu\\sound.gb";
    {
        try
        {
            App.MainLoop(Configuration);
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << std::endl;
        }
    }
    return (0);
}

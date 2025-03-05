#include <iostream>
#include <unistd.h>

#include "application.h"
#include "config.h"

bool ParseCommandLine(int argc, char **argv, Config &config)
{
    int option = 0;
    bool Valid = false;

    while ((option = getopt(argc, argv, "f:")) != -1)
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
    }
    if (!Valid)
        std::cout << argv[0] << " -f <filename>" << std::endl;
    return (Valid);
}

int main(int argc, char **argv)
{
    Config Configuration;
    Application App;

    // if (ParseCommandLine(argc, argv, Configuration))
    Configuration.mRomFile = "/media/data/rom/Super Mario Land (World).gb";
    {
        App.MainLoop(Configuration);
    }
    return (0);
}

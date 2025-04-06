#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <string>

struct Config
{
    std::string _romfile;
    int _screen_scale;
    std::string _recordfile;
    bool _audio_filter;
};
#endif
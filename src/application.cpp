#include "application.h"
#include <SDL3/SDL.h>
#include <format>

Application::Application()
{
    if (!SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS))
        throw std::runtime_error(std::format("SDL_Init : {}", SDL_GetError()));
}

auto Application::MainLoop(const Config &Configuration) -> void
{
    mEmulator = std::make_unique<Emulator>(Configuration);
    mEmulator->init();
    mEmulator->loop();
    SDL_Quit();
}
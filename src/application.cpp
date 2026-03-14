#include "application.h"
#include "frontend_sdl.h"
#include <SDL3/SDL.h>
#include <format>

Application::Application()
{
    if (!SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS))
        throw std::runtime_error(std::format("SDL_Init : {}", SDL_GetError()));
}

auto Application::MainLoop(const Config &Configuration) -> void
{
    mFrontend = std::make_unique<Frontend_SDL>(Configuration);
    mEmulator = std::make_unique<Emulator>(Configuration);
    mEmulator->init();
    while (!mEmulator->step_frame())
        mFrontend->play_audio(mEmulator->get_audio_buffer());
    SDL_Quit();
}

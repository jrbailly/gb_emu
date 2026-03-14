#include "application.h"
#include "frontend_sdl.h"
#include "vbm_record.h"
#include <SDL3/SDL.h>
#include <format>

Application::Application()
{
}

auto Application::Init(const Config &Configuration) -> void
{
    mFrontend = std::make_unique<FrontendSDL>(Configuration);
    mEmulator = std::make_unique<Emulator>(Configuration);
    mEmulator->init();
    if (!Configuration._recordfile.empty())
    {
        mRecord = std::make_unique<VbmRecord>();
        mRecord->parse_file(Configuration._recordfile);
    }
}

auto Application::MainLoop() -> void
{
    int pad = 0;
    int button = 0;

    while (!mQuit)
    {
        mQuit = mFrontend->get_input(pad, button);
        if (mRecord)
            mRecord->get_input(pad, button);
        if (mFrontend->pop_save_request())
            mEmulator->save_state();
        if (mFrontend->pop_load_request())
            mEmulator->load_state();
        if (!mQuit)
        {
            mEmulator->set_input(pad, button);
            mFrontend->delay(mEmulator->step_frame());
            mFrontend->play_audio(mEmulator->get_audio_buffer());
        }
    }
}

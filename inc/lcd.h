#ifndef _LCD_H_
#define _LCD_H_

#include "cpu.h"
#include "ram.h"
#include <SDL3/SDL.h>

const int max_tiles = 256;
const int width_tiles = 8;
const int height_tiles = 8;
const int line_width = max_tiles * width_tiles;
class LCD
{
  public:
    enum Register
    {
        LCDC = 0xFF40,
        STAT = 0xFF41,
        SCY = 0xFF42,
        SCX = 0xFF43,
        LY = 0xFF44,
        LYC = 0xFF45,
        BGP = 0xFF47,
        OBP0 = 0xFF48,
        OBP1 = 0xFF49
    };
    enum GrayLevel
    {
        WHITE = 0,
        LIGHT_GRAY,
        DARK_GRAY,
        BLACK,
        TRANSPARENT
    };
    LCD(MBC1 &ram);
    void step(uint32_t cycles_count);
    void renderer();
    void reset();

  private:
    void updateBGP0();
    void updateOBP0();
    void updateBGP1();
    void loadSurfaceSprites();
    void loadSurfaceBackground();
    void drawSprites();
    void drawBackground();

  private:
    MBC1 &mRAM;
    uint32_t mNext_line_cycle;
    SDL_Window *mWindow;
    SDL_Renderer *mRenderer;
    SDL_Surface *mSurfaceSprites;
    SDL_Texture *mTextureSprites;
    SDL_Surface *mSurfaceBackground;
    SDL_Texture *mTextureBackground;
    uint32_t mBGP0[4];
    uint32_t mOBP0[4];
    uint32_t mOBP1[4];
    uint32_t mColors[5];
    bool mReloadSurface;
};

#endif
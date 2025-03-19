#ifndef _LCD_H_
#define _LCD_H_

#include "cpu.h"
#include "ram.h"
#include <SDL3/SDL.h>

const int cycles_per_line = 456;
const int max_tiles = 512;
const int width_tiles = 8;
const int height_tiles = 8;
const int line_width = max_tiles * width_tiles;
const int oam_address = 0xfe00;
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
        OBP1 = 0xFF49,
        WY = 0xFF4A,
        WX = 0xFF4B,
    };
    enum TilesAddress
    {
        BLOCK0 = 0x8000,
        BLOCK1 = 0x8800,
        BLOCK2 = 0x9000,
    };
    enum BackgroundAddress
    {
        AREA0 = 0x9800,
        AREA1 = 0x9C00,
    };
    enum LCDC
    {
        BG_ENABLE = 0x1,
        OBJ_ENABLE = 0x2,
        OBJ_SIZE = 0x4,
        BG_TILE_AREA = 0x8,
        BG_DATA_AREA = 0x10,
        WIN_ENABLE = 0x20,
        WIN_TILE_AREA = 0x40,
        LCD_ENABLE = 0x80
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
    void step(uint32_t cycles_count, uint16_t last_addr);
    void renderer();
    void reset();

  private:
    void updateStat();
    void updateBGP0();
    void updateOBP0();
    void updateBGP1();
    void loadSurfaceSprites();
    void loadSurfaceBackground();
    void drawSprites();
    void drawBackgroundLine();

  private:
    MBC1 &mRAM;
    int mNext_line_cycle;
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
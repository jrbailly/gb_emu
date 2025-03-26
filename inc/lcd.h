#ifndef _LCD_H_
#define _LCD_H_

#include "cpu.h"
#include "ram.h"
#include <SDL3/SDL.h>

static constexpr int screen_width = 160;
static constexpr int screen_height = 144;
static constexpr int tile_maps_width = 256;
static constexpr int tile_maps_height = 256;
static constexpr int tiles_width = 8;
static constexpr int tiles_height = 8;
static constexpr int max_tiles = 512;
static constexpr int line_width = max_tiles * tiles_width;
static constexpr int oam_size = 160;
static constexpr int tiles_memory_size = 0x1800;
static constexpr int cycles_per_line = 456;
static constexpr int max_lines = 154;
static constexpr int frame_cycle_count = cycles_per_line * max_lines;
static constexpr int frame_duration = ((long long int)frame_cycle_count * 1000000) / CPU_FREQ;
class LCD
{
  public:
    enum Register
    {
        OAM = 0xFE00,
        LCDC = 0xFF40,
        STAT = 0xFF41,
        SCY = 0xFF42,
        SCX = 0xFF43,
        LY = 0xFF44,
        LYC = 0xFF45,
        DMA = 0xFF46,
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
    enum LCDStatus
    {
        LYC_LY = 0x4,
        MODE0_INT = 0x8,
        MODE1_INT = 0x10,
        MODE2_INT = 0x20,
        LYC_INT = 0x40,
    };
    enum SpriteAttributes
    {
        PALETTE = 0x10,
        X_FLIP = 0x20,
        Y_FLIP = 0x40,
        PRIORITY = 0x80,
    };
    enum GrayLevel
    {
        TRANSPARENT = 0,
        LIGHT_GRAY,
        DARK_GRAY,
        BLACK,
        WHITE,
    };
    LCD(RamBus &ram);
    virtual ~LCD();
    void init(RamBus &ram);
    void step(int cycles_count);
    void renderer();
    void set_scale(int scale);

  private:
    void create_window();
    void destroy_window();
    void scanline();
    void update_stat();
    void update_BGP0();
    void update_OBP0();
    void update_BGP1();
    void load_surface_sprites();
    void load_surface_background();
    void draw_sprites(bool priority);
    void draw_background_line();
    bool draw_window_line();

  private:
    RamBus &_ram;
    int _scale;
    int _next_line_cycle;
    SDL_Window *_window;
    SDL_Renderer *_renderer;
    SDL_Surface *_surface_sprites;
    SDL_Texture *_texture_sprites;
    SDL_Surface *_surface_background;
    SDL_Texture *_texture_background;
    unsigned int _BGP0[4];
    unsigned int _OBP0[4];
    unsigned int _OBP1[4];
    unsigned int _colors[5];
    bool _reload_surface;
};

#endif
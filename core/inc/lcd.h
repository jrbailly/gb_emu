#ifndef _LCD_H_
#define _LCD_H_

#include "cpu.h"
#include "ram.h"
#include <array>
#include <span>

static constexpr int screen_width = 160;
static constexpr int screen_height = 144;
static constexpr int tile_maps_width = 256;
static constexpr int tile_maps_height = 256;
static constexpr int tile_maps_size = 32;
static constexpr int tiles_width = 8;
static constexpr int tiles_height = 8;
static constexpr int max_tiles = 512;
static constexpr int line_width = max_tiles * tiles_width;
static constexpr int oam_size = 160;
static constexpr int tiles_memory_size = 0x1800;
static constexpr int cycles_mode2 = 80;
static constexpr int cycles_mode3 = 172;
static constexpr int cycles_mode0 = 204;
static constexpr int cycles_per_line = 456;
static constexpr int max_lines = 154;
static constexpr int texture_padding = 2 * tiles_width;
static constexpr int texture_offset = tiles_width;

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
    enum Status
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
    enum Mode
    {
        MODE0 = 0,
        MODE1,
        MODE2,
        MODE3,
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
    auto init(RamBus &ram) -> void;
    auto step(int cycles_count) -> void;
    auto load_state() -> void;
    auto get_frame_buffer() const -> std::span<const uint32_t>
    {
        return _framebuffer_ready;
    }

  private:
    auto scanline() -> void;
    auto update_stat() -> void;
    auto update_BGP0() -> void;
    auto update_OBP0() -> void;
    auto update_OBP1() -> void;
    auto load_texture_sprites() -> void;
    auto load_texture_background() -> void;
    auto draw_sprites(uint32_t *datas) -> void;
    auto draw_background_line(uint32_t *datas) -> void;
    auto draw_window_line(uint32_t *datas) -> bool;

  private:
    RamBus &_ram;
    bool _stat_interrupt;
    int _next_ly;
    int _next_op_cycle;
    Mode _current_mode;
    Mode _next_mode;
    std::array<uint32_t, (screen_width + texture_padding) * screen_height> _framebuffer;
    std::array<uint32_t, (screen_width + texture_padding) * screen_height> _framebuffer_ready;
    unsigned int _BGP0[4];
    unsigned int _OBP0[4];
    unsigned int _OBP1[4];
    unsigned int _colors[5];
    bool _reload_surface;
    bool _reload_sprite;
    bool _reload_background;
    std::array<int, 4> _mode_cyles;
    std::array<int, max_tiles> _background_change;
    std::array<int, max_tiles> _sprite_change;
    std::array<std::array<std::array<int, tiles_height>, line_width>, 384> _texture_sprites;
    std::array<std::array<std::array<int, tiles_height>, line_width>, 384> _texture_background;
};

#endif
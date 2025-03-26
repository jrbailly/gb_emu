#include "lcd.h"
#include "ram.h"
#include <cstdio>
#include <functional>

/**
 * @brief Construct a new LCD object
 *
 * @param ram
 */
LCD::LCD(RamBus &ram) : _ram(ram), _scale(1)
{
    _next_line_cycle = cycles_per_line;
    _colors[GrayLevel::TRANSPARENT] = 0x0;
    _colors[GrayLevel::LIGHT_GRAY] = 0xA0A0A0FF;
    _colors[GrayLevel::DARK_GRAY] = 0x585858FF;
    _colors[GrayLevel::BLACK] = 0x000000FF;
    _colors[GrayLevel::WHITE] = 0xFFFFFFFF;
    _BGP0[0] = _colors[GrayLevel::TRANSPARENT];
    _BGP0[1] = _colors[GrayLevel::LIGHT_GRAY];
    _BGP0[2] = _colors[GrayLevel::DARK_GRAY];
    _BGP0[3] = _colors[GrayLevel::BLACK];
    _window = nullptr;
    _renderer = nullptr;
    _surface_sprites = nullptr;
    _texture_sprites = nullptr;
    _surface_background = nullptr;
    _texture_background = nullptr;
    _reload_surface = true;
}

LCD::~LCD()
{
    destroy_window();
}

void LCD::create_window()
{
    destroy_window();
    _window = SDL_CreateWindow("", _scale * screen_width, _scale * screen_height, 0);
    if (!_window)
        throw std::runtime_error(std::string("SDL_CreateWindow : ") + SDL_GetError());
    _renderer = SDL_CreateRenderer(_window, NULL);
    if (!_renderer)
        throw std::runtime_error(std::string("SDL_CreateRenderer : ") + SDL_GetError());
    _surface_sprites = SDL_CreateSurface(line_width, 2 * tiles_height, SDL_PIXELFORMAT_RGBA8888);
    if (!_surface_sprites)
        throw std::runtime_error(std::string("SDL_CreateSurface : ") + SDL_GetError());
    _surface_background = SDL_CreateSurface(line_width, tiles_height, SDL_PIXELFORMAT_RGBA8888);
    if (!_surface_background)
        throw std::runtime_error(std::string("SDL_CreateSurface : ") + SDL_GetError());
    if (!SDL_SetRenderDrawColor(_renderer, 255, 255, 255, 255))
        throw std::runtime_error(std::string("SDL_SetRenderDrawColor : ") + SDL_GetError());
}

void LCD::destroy_window()
{
    if (_window)
        SDL_DestroyWindow(_window);
    if (_renderer)
        SDL_DestroyRenderer(_renderer);
    if (_surface_sprites)
        SDL_DestroySurface(_surface_sprites);
    if (_surface_background)
        SDL_DestroySurface(_surface_background);
}

void LCD::init(RamBus &ram)
{
    ram.register_callback(Register::BGP, [this](RamBus &ram, int addr, unsigned char val) { this->update_BGP0(); });
    ram.register_callback(Register::OBP0, [this](RamBus &ram, int addr, unsigned char val) { this->update_OBP0(); });
    ram.register_callback(Register::OBP1, [this](RamBus &ram, int addr, unsigned char val) { this->update_BGP1(); });
    ram.register_callback(Register::DMA, [this](RamBus &ram, int addr, unsigned char val) {
        int start_address = val << 8;
        int end_address = start_address + oam_size;
        int dst_address = Register::OAM;
        for (int i = 0; i < oam_size; ++i)
            ram.write(dst_address + i, ram[start_address + i]);
    });
    ram.register_callback_range(TilesAddress::BLOCK0, tiles_memory_size,
                                [this](RamBus &ram, int addr, unsigned char val) { _reload_surface = true; });
    create_window();
}

void LCD::step(int cycles_count)
{
    unsigned char interrupt;

    _next_line_cycle -= cycles_count;
    if (_next_line_cycle <= 0)
    {
        unsigned char ly = (_ram[Register::LY] + 1) % max_lines;

        interrupt = _ram[CPU::Register::IF];
        _ram.write(Register::LY, ly);
        _next_line_cycle += cycles_per_line;
        // vblank
        if (ly == 144)
            _ram.write(CPU::Register::IF, interrupt | 0x1);
        update_stat();
        scanline();
    }
}

void LCD::renderer()
{
    if (_reload_surface)
    {
        load_surface_background();
        load_surface_sprites();
        if (_texture_sprites)
            SDL_DestroyTexture(_texture_sprites);
        if (_texture_background)
            SDL_DestroyTexture(_texture_background);
        _texture_sprites = SDL_CreateTextureFromSurface(_renderer, _surface_sprites);
        if (!_texture_sprites)
            throw std::runtime_error(std::string("SDL_CreateTextureFromSurface : ") + SDL_GetError());
        _texture_background = SDL_CreateTextureFromSurface(_renderer, _surface_background);
        if (!_texture_background)
            throw std::runtime_error(std::string("SDL_CreateTextureFromSurface : ") + SDL_GetError());
        SDL_SetTextureScaleMode(_texture_sprites, SDL_SCALEMODE_NEAREST);
        SDL_SetTextureScaleMode(_texture_background, SDL_SCALEMODE_NEAREST);
        _reload_surface = false;
    }
    if (_ram[Register::LCDC] & LCD_ENABLE)
    {
        if (!SDL_SetRenderScale(_renderer, _scale, _scale))
            throw std::runtime_error(std::string("SDL_SetRenderScale : ") + SDL_GetError());
        if (!SDL_RenderPresent(_renderer))
            throw std::runtime_error(std::string("SDL_RenderPresent : ") + SDL_GetError());
        if (!SDL_RenderClear(_renderer))
            throw std::runtime_error(std::string("SDL_RenderClear : ") + SDL_GetError());
    }
}

void LCD::set_scale(int scale)
{
    _scale = scale;
    destroy_window();
    create_window();
}

void LCD::scanline()
{
    bool display_window_line = false;

    if (_ram[Register::LCDC] & LCD_ENABLE)
    {
        if (_ram[Register::LCDC] & OBJ_ENABLE)
            draw_sprites(true);
        if (_ram[Register::LCDC] & WIN_ENABLE)
            display_window_line = draw_window_line();
        if (_ram[Register::LCDC] & BG_ENABLE && !display_window_line)
            draw_background_line();
        if (_ram[Register::LCDC] & OBJ_ENABLE)
            draw_sprites(false);
    }
}

void LCD::update_stat()
{
    unsigned char stat = _ram[Register::STAT] & 0xF8;
    unsigned char ly = _ram[Register::LY];
    unsigned char lyc = _ram[Register::LYC];
    unsigned char interrupt = _ram[CPU::Register::IF];

    if (ly >= 144)
        stat |= 0x1;
    else
        stat |= 0x0;
    if (ly == lyc)
        stat |= LYC_LY;
    if ((stat & MODE1_INT) && (ly == 144))
        interrupt |= 0x2;
    if ((stat & LYC_INT) && (ly == lyc))
        interrupt |= 0x2;
    _ram.write(CPU::Register::IF, interrupt);
    _ram.write(Register::STAT, stat);
}

void LCD::update_BGP0()
{
    unsigned char palette = _ram[Register::BGP];

    for (int i = 0; i < 8; i += 2)
        _BGP0[i / 2] = _colors[(palette >> i) & 0x3];
    _reload_surface = true;
}

void LCD::update_OBP0()
{
    unsigned char palette = _ram[Register::OBP0];
    unsigned char color;

    for (int i = 2; i < 8; i += 2)
    {
        color = (palette >> i) & 0x3;
        if (color == 0)
            _OBP0[i / 2] = _colors[WHITE];
        else
            _OBP0[i / 2] = _colors[color];
    }
    _OBP0[0] = _colors[GrayLevel::TRANSPARENT];
    _reload_surface = true;
}

void LCD::update_BGP1()
{
    unsigned char palette = _ram[Register::OBP1];
    unsigned char color;

    for (int i = 2; i < 8; i += 2)
    {
        color = (palette >> i) & 0x3;
        if (color == 0)
            _OBP1[i / 2] = _colors[WHITE];
        else
            _OBP1[i / 2] = _colors[color];
    }
    _OBP1[0] = _colors[GrayLevel::TRANSPARENT];
    _reload_surface = true;
}

void LCD::load_surface_sprites()
{
    uint32_t *datas = static_cast<uint32_t *>(_surface_sprites->pixels);
    int address = TilesAddress::BLOCK0;
    unsigned char value;

    for (int i = 0; i < 256; ++i)
    {
        for (int j = 0; j < tiles_height; ++j)
        {
            for (int k = 0; k < 8; k++)
            {
                value = ((_ram[address] >> (7 - k)) & 1) | (((_ram[address + 1] >> (7 - k)) & 1) << 1);
                datas[(j * line_width) + (i * tiles_width) + k] = _OBP0[value];
                datas[((j + tiles_height) * line_width) + (i * tiles_width) + k] = _OBP1[value];
            }
            address += 2;
        }
    }
}

void LCD::load_surface_background()
{
    uint32_t *datas = static_cast<uint32_t *>(_surface_background->pixels);
    int address = TilesAddress::BLOCK0;
    unsigned char value;

    for (int i = 0; i < 384; ++i)
    {
        for (int line = 0; line < tiles_height; ++line)
        {
            for (int k = 0; k < 8; k++)
            {
                value = ((_ram[address] >> (7 - k)) & 1) | (((_ram[address + 1] >> (7 - k)) & 1) << 1);
                datas[(line * line_width) + (i * tiles_width) + k] = _BGP0[value];
            }
            address += 2;
        }
    }
}

void LCD::draw_sprites(bool priority)
{
    int address = Register::OAM;
    int attribute;
    int line = _ram[LY];
    int y;
    int height = tiles_height;
    unsigned char value;
    float angle;
    SDL_FRect src;
    SDL_FRect dst;
    SDL_FlipMode flip;

    if (_ram[LCDC] & LCDC::OBJ_SIZE)
        height *= 2;
    for (int i = 0; i < 40; ++i)
    {
        flip = SDL_FLIP_NONE;
        angle = 0;
        y = _ram[address] - 16;
        dst.x = _ram[address + 1] - 8;
        value = _ram[address + 2];
        attribute = _ram[address + 3];
        if (((attribute & PRIORITY) != 0 && priority) || ((attribute & PRIORITY) == 0 && !priority))
        {
            if (line >= y && line < y + height)
            {
                if (line - y >= tiles_height)
                    value++;
                src.y = (line - y) % tiles_height;
                if (attribute & X_FLIP)
                {
                    flip = SDL_FLIP_HORIZONTAL;
                    angle = 0;
                }
                if (attribute & Y_FLIP)
                    src.y = height - 1 - src.y;
                if (attribute & PALETTE)
                    src.y += tiles_height;
                src.x = (value * tiles_width);
                src.w = tiles_width;
                src.h = 1;
                dst.y = line;
                dst.w = tiles_width;
                dst.h = 1;
                SDL_RenderTextureRotated(_renderer, _texture_sprites, &src, &dst, angle, nullptr, flip);
            }
        }
        address += 4;
    }
}

void LCD::draw_background_line()
{
    int address = BackgroundAddress::AREA0;
    int line = _ram[LY];
    int y = (_ram[SCY] + line) % tile_maps_height;
    int x = _ram[SCX];
    int x_offset = x % tiles_width;
    int value;
    SDL_FRect src;
    SDL_FRect dst;

    if (_ram[Register::LCDC] & BG_TILE_AREA)
        address = BackgroundAddress::AREA1;
    address += ((y / 8) * 32);
    for (int dst_x = 0; dst_x <= 20; ++dst_x)
    {
        value = _ram[address + (x / 8)];
        if (value < 128 && (_ram[Register::LCDC] & BG_DATA_AREA) == 0)
            value += 256;
        src.x = (value * tiles_width);
        src.y = y % tiles_height;
        src.w = tiles_width;
        src.h = 1;
        dst.x = (dst_x * tiles_width) - x_offset;
        dst.y = line;
        dst.w = tiles_width;
        dst.h = 1;
        SDL_RenderTexture(_renderer, _texture_background, &src, &dst);
        x = (x + tiles_width) % tile_maps_width;
    }
}

auto LCD::draw_window_line() -> bool
{
    int address = BackgroundAddress::AREA0;
    int line = _ram[LY];
    int y = line - _ram[WY];
    int x = _ram[WX];
    int value;
    SDL_FRect src;
    SDL_FRect dst;
    bool show_tile = false;

    if (_ram[Register::LCDC] & WIN_TILE_AREA)
        address = BackgroundAddress::AREA1;
    address += ((y / tiles_width) * 32);
    if (y >= 0)
    {
        for (int dst_x = 0; dst_x < 21; ++dst_x)
        {
            if (dst_x * tiles_width >= _ram[WX])
            {
                value = _ram[address + (x / 8)];
                if (value < 128 && (_ram[Register::LCDC] & BG_DATA_AREA) == 0)
                    value += 256;
                src.x = (value * tiles_width);
                src.y = line % tiles_height;
                src.w = tiles_width;
                src.h = 1;
                dst.x = (dst_x * tiles_width) - 7;
                dst.y = line;
                dst.w = tiles_width;
                dst.h = 1;
                SDL_RenderTexture(_renderer, _texture_background, &src, &dst);
                x = (x + tiles_width) % tile_maps_width;
                show_tile = true;
            }
        }
    }
    return (show_tile);
}
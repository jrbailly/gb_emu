#include "lcd.h"
#include "ram.h"
#include <cstdio>
#include <format>
#include <functional>

/**
 * @brief Construct a new LCD object
 *
 * Initializes the LCD with a reference to the RamBus and sets default values for colors, scale, and pointers.
 *
 * @param ram Reference to the RamBus object used for memory access
 */
LCD::LCD(RamBus &ram) : _ram(ram)
{
    _stat_interrupt = false;
    _next_ly = 0;
    _scale = 1;
    _current_mode = Mode::MODE2;
    _next_mode = Mode::MODE2;
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
    _texture_sprites = nullptr;
    _texture_background = nullptr;
    _reload_surface = true;
    _reload_sprite = true;
    _reload_background = true;
    _mode_cyles[Mode::MODE2] = cycles_mode2;
    _mode_cyles[Mode::MODE3] = cycles_mode3;
    _mode_cyles[Mode::MODE0] = cycles_mode0;
    _mode_cyles[Mode::MODE1] = cycles_per_line;
    _next_op_cycle = 0;
    _background_change.fill(0);
    _sprite_change.fill(0);
    _ram.write_register(Register::LY, 0);
}

/**
 * @brief Destroy the LCD object
 *
 * Cleans up by destroying the window and associated resources.
 */
LCD::~LCD()
{
    destroy_window();
}

/**
 * @brief Create a new window for the LCD
 *
 * Sets up an SDL window, renderer, and textures for sprites and background rendering.
 * Throws an exception if any SDL operation fails.
 */
auto LCD::create_window() -> void
{
    destroy_window();
    _window = SDL_CreateWindow("", _scale * screen_width, _scale * screen_height, 0);
    if (!_window)
        throw std::runtime_error(std::format("SDL_CreateWindow : {}", SDL_GetError()));
    _renderer = SDL_CreateRenderer(_window, NULL);
    if (!_renderer)
        throw std::runtime_error(std::format("SDL_CreateRenderer : {}", SDL_GetError()));
    _texture_sprites = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, line_width,
                                         2 * tiles_height);
    if (!_texture_sprites)
        throw std::runtime_error(std::format("SDL_CreateTexture : {}", SDL_GetError()));
    _texture_background =
        SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, line_width, tiles_height);
    if (!_texture_background)
        throw std::runtime_error(std::format("SDL_CreateTexture : {}", SDL_GetError()));
    if (!SDL_SetRenderDrawColor(_renderer, 255, 255, 255, 255))
        throw std::runtime_error(std::format("SDL_SetRenderDrawColor : {}", SDL_GetError()));
    if (!SDL_SetTextureScaleMode(_texture_sprites, SDL_SCALEMODE_NEAREST))
        throw std::runtime_error(std::format("SDL_SetTextureScaleMode : {}", SDL_GetError()));
    if (!SDL_SetTextureScaleMode(_texture_background, SDL_SCALEMODE_NEAREST))
        throw std::runtime_error(std::format("SDL_SetTextureScaleMode : {}", SDL_GetError()));
}

/**
 * @brief Destroy the window and associated resources
 *
 * Frees the SDL window, renderer, and texture resources if they exist.
 */
auto LCD::destroy_window() -> void
{
    if (_window)
        SDL_DestroyWindow(_window);
    if (_renderer)
        SDL_DestroyRenderer(_renderer);
    if (_texture_sprites)
        SDL_DestroyTexture(_texture_sprites);
    if (_texture_background)
        SDL_DestroyTexture(_texture_background);
}

/**
 * @brief Initialize the LCD with the given RamBus
 *
 * Registers callbacks for palette and tile updates and creates the display window.
 *
 * @param ram Reference to the RamBus object for memory operations
 */
auto LCD::init(RamBus &ram) -> void
{
    ram.register_callback(Register::BGP, [this](RamBus &, int, unsigned char) { this->update_BGP0(); });
    ram.register_callback(Register::OBP0, [this](RamBus &, int, unsigned char) { this->update_OBP0(); });
    ram.register_callback(Register::OBP1, [this](RamBus &, int, unsigned char) { this->update_OBP1(); });
    ram.register_callback(Register::DMA, [](RamBus &ram, int, unsigned char val) {
        int start_address = val << 8;
        ram.write_range(start_address, Register::OAM, oam_size);
    });
    ram.register_callback_range(TilesAddress::BLOCK0, tiles_memory_size, [this](RamBus &, int addr, unsigned char) {
        int index = (addr - TilesAddress::BLOCK0) / (2 * tiles_width);
        _background_change[index] = 1;
        _sprite_change[index] = 1;
        _reload_surface = true;
    });
    create_window();
}

/**
 * @brief Advance the LCD simulation by a given number of cycles
 *
 * Updates the LCD state, triggers scanlines, and handles VBlank interrupts.
 *
 * @param cycles_count Number of cycles to advance the simulation
 */
auto LCD::step(int cycles_count) -> void
{
    unsigned char ly;

    if (_ram[Register::LCDC] & LCD_ENABLE)
    {
        _next_op_cycle -= cycles_count;
        if (_next_op_cycle <= 0)
        {
            _current_mode = _next_mode;
            if (_next_ly != _ram[Register::LY])
                _ram.write_register(Register::LY, _next_ly);
            ly = _ram[Register::LY];
            _next_op_cycle += _mode_cyles[_current_mode];
            switch (_current_mode)
            {
            case Mode::MODE2:
                _next_mode = Mode::MODE3;
                break;
            case Mode::MODE3:
                scanline();
                _next_mode = Mode::MODE0;
                break;
            case Mode::MODE0:
                if (ly + 1 < screen_height)
                    _next_mode = Mode::MODE2;
                else
                    _next_mode = Mode::MODE1;
                _next_ly = ly + 1;
                break;
            case Mode::MODE1:
                _next_mode = Mode::MODE1;
                _next_ly = (ly + 1) % max_lines;
                if ((ly + 1) == max_lines)
                {
                    renderer();
                    _next_mode = Mode::MODE2;
                }
                break;
            }
            update_stat();
        }
    }
    else
    {
        _next_mode = Mode::MODE2;
        _ram.write_register(Register::LY, 0);
        _next_op_cycle = 0;
        _next_ly = 0;
    }
}

/**
 * @brief Render the current frame if LCD is enabled
 *
 * Updates the display by scaling, presenting, and clearing the renderer.
 * Throws an exception if SDL operations fail.
 */
auto LCD::renderer() -> void
{
    // draw_background_tiles();
    if (!SDL_SetRenderScale(_renderer, _scale, _scale))
        throw std::runtime_error(std::format("SDL_SetRenderScale : {}", SDL_GetError()));
    if (!SDL_FlushRenderer(_renderer))
        throw std::runtime_error(std::format("SDL_FlushRenderer : {}", SDL_GetError()));
    if (!SDL_RenderPresent(_renderer))
        throw std::runtime_error(std::format("SDL_RenderPresent : {}", SDL_GetError()));
    if (!SDL_RenderClear(_renderer))
        throw std::runtime_error(std::format("SDL_RenderClear : {}", SDL_GetError()));
}

/**
 * @brief Set the scale of the LCD window
 *
 * Updates the display scale and recreates the window accordingly.
 *
 * @param scale The new scale factor for the window
 */
auto LCD::set_scale(int scale) -> void
{
    _scale = scale;
    destroy_window();
    create_window();
}

/**
 * @brief Reload palettes and textures
 *
 */
auto LCD::load_state() -> void
{
    update_BGP0();
    update_OBP0();
    update_OBP1();
    load_texture_background();
    load_texture_sprites();
}

/**
 * @brief Process a single scanline
 *
 * Handles rendering of sprites, window, and background for the current scanline if LCD is enabled.
 */
auto LCD::scanline() -> void
{
    bool display_window_line = false;

    if (_reload_surface)
    {
        load_texture_background();
        load_texture_sprites();
        _reload_surface = false;
        _reload_sprite = false;
        _reload_background = false;
    }
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

/**
 * @brief Update the STAT register and handle interrupts
 *
 * Updates the STAT register based on the current line and triggers interrupts as needed.
 */
auto LCD::update_stat() -> void
{
    unsigned char stat = _ram[Register::STAT];
    unsigned char ly = _ram[Register::LY];
    unsigned char lyc = _ram[Register::LYC];
    unsigned char interrupt = _ram[CPU::Register::IF];
    bool stat_interrupt = false;

    if (ly == screen_height)
        interrupt |= 0x1;
    stat = (stat & 0xFC) | _current_mode;
    if (ly == lyc)
        stat |= LYC_LY;
    if ((stat & MODE0_INT) && _current_mode == Mode::MODE0)
        stat_interrupt = true;
    if ((stat & MODE1_INT) && _current_mode == Mode::MODE1)
        stat_interrupt = true;
    if ((stat & MODE2_INT) && _current_mode == Mode::MODE2)
        stat_interrupt = true;
    if ((stat & LYC_INT) && (ly == lyc))
        stat_interrupt = true;
    if (!_stat_interrupt && stat_interrupt)
        interrupt |= 0x2;
    _stat_interrupt = stat_interrupt;
    _ram.write_register(CPU::Register::IF, interrupt);
    _ram.write_register(Register::STAT, stat);
}

/**
 * @brief Update the BGP0 palette
 *
 * Refreshes the background palette (BGP0) based on the current BGP register value.
 */
auto LCD::update_BGP0() -> void
{
    unsigned char palette = _ram[Register::BGP];

    for (int i = 0; i < 8; i += 2)
        _BGP0[i / 2] = _colors[(palette >> i) & 0x3];
    _reload_surface = true;
    _reload_background = true;
}

/**
 * @brief Update the OBP0 palette
 *
 * Refreshes the first object palette (OBP0) based on the current OBP0 register value.
 */
auto LCD::update_OBP0() -> void
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
    _reload_sprite = true;
}

/**
 * @brief Update the BGP1 palette
 *
 * Refreshes the second object palette (OBP1) based on the current OBP1 register value.
 */
auto LCD::update_OBP1() -> void
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
    _reload_sprite = true;
}

/**
 * @brief Load the sprite surface into the texture
 *
 * Updates the sprite texture with data from the tile memory using OBP0 and OBP1 palettes.
 */
auto LCD::load_texture_sprites() -> void
{
    uint32_t *datas;
    int address;
    int tile_index = 0;
    int pitch;
    unsigned char value;
    SDL_Rect rect{0, 0, line_width, 2 * tiles_height};

    if (!SDL_LockTexture(_texture_sprites, &rect, (void **)&(datas), &pitch))
        throw std::runtime_error(std::format("SDL_LockTexture : {}", SDL_GetError()));
    for (int i = 0; i < 256; ++i)
    {
        if (_sprite_change[i] != 0 || _reload_sprite)
        {
            address = TilesAddress::BLOCK0 + (i * tiles_width * 2);
            for (int j = 0; j < tiles_height; ++j)
            {
                for (int k = 0; k < 8; k++)
                {
                    value = ((_ram[address] >> (7 - k)) & 1) | (((_ram[address + 1] >> (7 - k)) & 1) << 1);
                    datas[(j * line_width) + tile_index + k] = _OBP0[value];
                    datas[((j + tiles_height) * line_width) + tile_index + k] = _OBP1[value];
                }
                address += 2;
            }
        }
        _sprite_change[i] = 0;
        tile_index += tiles_width;
    }
    SDL_UnlockTexture(_texture_sprites);
}

/**
 * @brief Load the background surface into the texture
 *
 * Updates the background texture with data from the tile memory using the BGP0 palette.
 */
auto LCD::load_texture_background() -> void
{
    uint32_t *datas = nullptr;
    int address = TilesAddress::BLOCK0;
    int tile_index = 0;
    int pitch = 0;
    unsigned char value;
    SDL_Rect rect{0, 0, line_width, tiles_height};

    if (!SDL_LockTexture(_texture_background, &rect, (void **)&(datas), &pitch))
        throw std::runtime_error(std::format("SDL_LockTexture : {}", SDL_GetError()));
    for (int i = 0; i < 384; ++i)
    {
        if (_background_change[i] != 0 || _reload_background)
        {
            address = TilesAddress::BLOCK0 + (i * tiles_width * 2);
            for (int line = 0; line < tiles_height; ++line)
            {
                for (int k = 0; k < 8; k++)
                {
                    value = ((_ram[address] >> (7 - k)) & 1) | (((_ram[address + 1] >> (7 - k)) & 1) << 1);
                    datas[(line * line_width) + tile_index + k] = _BGP0[value];
                }
                address += 2;
            }
        }
        _background_change[i] = 0;
        tile_index += tiles_width;
    }
    SDL_UnlockTexture(_texture_background);
}

/**
 * @brief Draw sprites with the given priority
 *
 * Renders sprites on the current scanline based on priority and attributes like flipping.
 *
 * @param priority Whether to draw sprites with priority over the background
 */
auto LCD::draw_sprites(bool priority) -> void
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
                {
                    if (line - y < tiles_height)
                        value++;
                    else
                        value--;
                    src.y = tiles_height - 1 - src.y;
                }
                if (attribute & PALETTE)
                    src.y += tiles_height;
                src.x = (value * tiles_width);
                src.w = tiles_width;
                src.h = 1;
                dst.y = line;
                dst.w = tiles_width;
                dst.h = 1;
                if (!SDL_RenderTextureRotated(_renderer, _texture_sprites, &src, &dst, angle, nullptr, flip))
                    throw std::runtime_error(std::format("SDL_RenderTextureRotated : {}", SDL_GetError()));
            }
        }
        address += 4;
    }
}

/**
 * @brief Draw a single line of the background
 *
 * Renders the background for the current scanline with scrolling offsets.
 */
auto LCD::draw_background_line() -> void
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
        if (!SDL_RenderTexture(_renderer, _texture_background, &src, &dst))
            throw std::runtime_error(std::format("SDL_RenderTexture : {}", SDL_GetError()));
        x = (x + tiles_width) % tile_maps_width;
    }
}

/**
 * @brief Draw a single line of the window
 *
 * Renders the window for the current scanline if visible.
 *
 * @return true if the window line was drawn, false otherwise
 */
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
        for (int dst_x = _ram[WX] / tiles_width; dst_x < 21; ++dst_x)
        {
            value = _ram[address + (x / 8)];
            if (value < 128 && (_ram[Register::LCDC] & BG_DATA_AREA) == 0)
                value += 256;
            src.x = (value * tiles_width);
            src.y = line % tiles_height;
            src.w = tiles_width;
            src.h = 1;
            dst.x = (dst_x * tiles_width);
            dst.y = line;
            dst.w = tiles_width;
            dst.h = 1;
            if (!SDL_RenderTexture(_renderer, _texture_background, &src, &dst))
                throw std::runtime_error(std::format("SDL_RenderTexture : {}", SDL_GetError()));
            x = (x + tiles_width) % tile_maps_width;
            show_tile = true;
        }
    }
    return (show_tile);
}

auto LCD::draw_background_tiles() -> bool
{
    int y = 0;
    SDL_FRect src;
    SDL_FRect dst;

    for (int i = 0; i < 384; i++)
    {
        src.x = (i * tiles_width);
        src.y = 0;
        src.w = tiles_width;
        src.h = tiles_height;
        dst.x = screen_width + ((i % 16) * tiles_width);
        dst.y = y;
        dst.w = tiles_width;
        dst.h = tiles_height;
        if ((i + 1) % 16 == 0)
            y += tiles_height;
        if (!SDL_RenderTexture(_renderer, _texture_background, &src, &dst))
            throw std::runtime_error(std::format("SDL_RenderTexture : {}", SDL_GetError()));
    }
    return (true);
}

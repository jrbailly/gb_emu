#include "lcd.h"
#include "ram.h"
#include <algorithm>
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
    _op_cycle = 0;
    _wnd_line = 0;
    _current_mode = Mode::INIT;
    _next_mode = Mode::MODE2;
    _colors[GrayLevel::TRANSPARENT] = 0x0;
    _colors[GrayLevel::LIGHT_GRAY] = 0xA0A0A0FF;
    _colors[GrayLevel::DARK_GRAY] = 0x585858FF;
    _colors[GrayLevel::BLACK] = 0x000000FF;
    _colors[GrayLevel::WHITE] = 0xFFFFFFFF;
    _BGP0[0] = _colors[GrayLevel::WHITE];
    _BGP0[1] = _colors[GrayLevel::LIGHT_GRAY];
    _BGP0[2] = _colors[GrayLevel::DARK_GRAY];
    _BGP0[3] = _colors[GrayLevel::BLACK];
    _reload_surface = true;
    _reload_sprite = true;
    _reload_background = true;
    _mode_cyles[Mode::MODE2] = cycles_mode2 - cycles_intr;
    _mode_cyles[Mode::MODE3] = cycles_mode3 - cycles_intr;
    _mode_cyles[Mode::MODE0] = cycles_mode0 - cycles_intr;
    _mode_cyles[Mode::MODE1] = cycles_per_line - cycles_intr;
    _mode_cyles[Mode::INTR] = 0;
    _mode_cyles[Mode::INIT] = cycles_mode2 - cycles_intr;
    _background_change.fill(0);
    _sprite_change.fill(0);
    _ram.write_register(Register::LY, 0);
    _ram.write_register(Register::LCDC, 0x91);
}

/**
 * @brief Destroy the LCD object
 *
 * Cleans up by destroying the window and associated resources.
 */
LCD::~LCD()
{
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
        _op_cycle -= cycles_count;
        while (_op_cycle <= 0)
        {
            ly = _ram[Register::LY];
            if (_current_mode == MODE1 || _current_mode == MODE2)
                _ram.write(Register::LY, (ly + 1) % max_lines);
            switch (_current_mode)
            {
            case Mode::INIT:
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
                break;
            case Mode::MODE1:
                _next_mode = Mode::MODE1;
                if ((ly + 1) == max_lines)
                {
                    _framebuffer_ready = _framebuffer;
                    _next_mode = Mode::INIT;
                    _wnd_line = 0;
                }
                break;
            case Mode::INTR:
                update_interrupt();
                _op_cycle += _current_op_cycle;
                _current_mode = _next_mode;
                return;
            default:
                break;
            }
            update_stat();
            _current_op_cycle = _mode_cyles[_current_mode];
            _current_mode = Mode::INTR;
            _op_cycle += cycles_intr;
        }
    }
    else
    {
        _current_mode = Mode::INIT;
        _ram.write_register(Register::LY, 0);
        _ram.write_register(Register::STAT, _ram[Register::STAT] & 0xFC);
        _op_cycle = 0;
        _wnd_line = 0;
        _reload_surface = true;
        _reload_sprite = true;
        _reload_background = true;
    }
}

/**
 * @brief Reload palettes and textures
 *
 */
auto LCD::load_state() -> void
{
    _reload_sprite = true;
    _reload_background = true;
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
        uint32_t *datas = _framebuffer.data() + _ram[LY] * (screen_width + texture_padding);

        if (_ram[Register::LCDC] & BG_ENABLE)
            draw_background_line(datas);
        if (_ram[Register::LCDC] & WIN_ENABLE)
            draw_window_line(datas);
        if (_ram[Register::LCDC] & OBJ_ENABLE)
            draw_sprites(datas);
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

    stat = (stat & 0xFC) | _current_mode;
    if (ly == lyc)
        stat |= LYC_LY;
    _ram.write_register(Register::STAT, stat);
}

/**
 * @brief Update the IF register to handle interrupts
 *
 * Updates the IF register based on the current and current line.
 */
auto LCD::update_interrupt() -> void
{
    unsigned char stat = _ram[Register::STAT];
    unsigned char ly = _ram[Register::LY];
    unsigned char lyc = _ram[Register::LYC];
    unsigned char interrupt = _ram[CPU::Register::IF];
    unsigned char current_mode = stat & 0x3;
    bool stat_interrupt = false;

    if (ly == screen_height)
        interrupt |= CPU::IFFlag::VBLANK;
    if ((stat & MODE0_INT) && current_mode == Mode::MODE0)
        stat_interrupt = true;
    if ((stat & MODE1_INT) && current_mode == Mode::MODE1)
        stat_interrupt = true;
    if ((stat & MODE2_INT) && current_mode == Mode::MODE2)
        stat_interrupt = true;
    if ((stat & LYC_INT) && (ly == lyc))
        stat_interrupt = true;
    if (!_stat_interrupt && stat_interrupt)
        interrupt |= CPU::IFFlag::LCD;
    _stat_interrupt = stat_interrupt;
    _ram.write_register(CPU::Register::IF, interrupt);
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
}

/**
 * @brief Load the sprite surface into the texture
 *
 * Updates the sprite texture with data from the tile memory using OBP0 and OBP1 palettes.
 */
auto LCD::load_texture_sprites() -> void
{
    int address;
    unsigned char value;

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
                    _texture_sprites[i][k][j] = value;
                }
                address += 2;
            }
            _sprite_change[i] = 0;
        }
    }
}

/**
 * @brief Load the background surface into the texture
 *
 * Updates the background texture with data from the tile memory using the BGP0 palette.
 */
auto LCD::load_texture_background() -> void
{
    int address = TilesAddress::BLOCK0;
    unsigned char value;

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
                    _texture_background[i][k][line] = value;
                }
                address += 2;
            }
            _background_change[i] = 0;
        }
    }
}

/**
 * @brief Draw sprites with the given priority
 *
 * Renders sprites on the current scanline based on priority and attributes like flipping.
 *
 * @param priority Whether to draw sprites with priority over the background
 */
auto LCD::draw_sprites(uint32_t *datas) -> void
{
    int line = _ram[LY];
    int height = tiles_height;
    std::array<OamEntry, oam_objects> oam_entries;

    // Sprite priority
    _ram.read_range(Register::OAM, oam_objects * sizeof(OamEntry),
                    reinterpret_cast<unsigned char *>(oam_entries.data()));
    std::sort(oam_entries.begin(), oam_entries.end(), [](const OamEntry &a, const OamEntry &b) { return a.x > b.x; });

    if (_ram[LCDC] & LCDC::OBJ_SIZE)
        height *= 2;
    for (const auto &entry : oam_entries)
    {
        int y = entry.y - oam_y_offset;
        int x = entry.x - oam_x_offset + texture_offset;
        uint8_t value = entry.value;
        int attribute = entry.attribute;
        int src_y;

        if (_ram[LCDC] & LCDC::OBJ_SIZE)
            value &= 0xFE;
        if (line >= y && line < y + height)
        {
            src_y = line - y;
            if (attribute & Y_FLIP)
                src_y = height - src_y - 1;
            if (src_y >= tiles_height)
                value++;
            src_y %= tiles_height;
            for (int i = 0; i < tiles_width; ++i)
            {
                int pixel;
                unsigned int color;

                if (attribute & X_FLIP)
                    pixel = _texture_sprites[value][7 - i][src_y];
                else
                    pixel = _texture_sprites[value][i][src_y];
                if (attribute & PALETTE)
                    color = _OBP1[pixel];
                else
                    color = _OBP0[pixel];
                if (color != _colors[GrayLevel::TRANSPARENT] &&
                    (((attribute & PRIORITY) == 0) || ((attribute & PRIORITY) && datas[x] == _BGP0[0])))
                    datas[x] = color;
                x++;
            }
        }
    }
}

/**
 * @brief Draw a single line of the background
 *
 * Renders the background for the current scanline with scrolling offsets.
 */
auto LCD::draw_background_line(uint32_t *datas) -> void
{
    int address = BackgroundAddress::AREA0;
    int line = _ram[LY];
    int y = (_ram[SCY] + line) % tile_maps_height;
    int x = _ram[SCX];
    int x_offset = x % tiles_width;
    int value;
    int index;

    if (_ram[Register::LCDC] & BG_TILE_AREA)
        address = BackgroundAddress::AREA1;
    address += ((y / tiles_width) * tile_maps_size);

    y %= tiles_height;
    for (int dst_x = 0; dst_x <= 20; ++dst_x)
    {
        value = _ram[address + (x / tiles_width)];
        if (value < 128 && (_ram[Register::LCDC] & BG_DATA_AREA) == 0)
            value += 256;
        index = texture_offset + dst_x * tiles_width - x_offset;
        for (int i = 0; i < tiles_width; ++i)
            datas[index++] = _BGP0[_texture_background[value][i][y]];
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
auto LCD::draw_window_line(uint32_t *datas) -> void
{
    int address = BackgroundAddress::AREA0;
    int line = _ram[LY];
    int y = line - _ram[WY];
    int x = _ram[WX] - 7;
    int x_offset = x % tiles_width;
    int value;
    int index;
    int index_address = 0;

    if (_ram[Register::LCDC] & WIN_TILE_AREA)
        address = BackgroundAddress::AREA1;
    address += ((_wnd_line / tiles_width) * 32);
    if (y >= 0 && (x / tiles_width) <= 20)
    {
        int tile_y = _wnd_line % tiles_height;
        for (int dst_x = (x / tiles_width); dst_x <= 20; ++dst_x)
        {
            value = _ram[address + index_address++];
            if (value < 128 && (_ram[Register::LCDC] & BG_DATA_AREA) == 0)
                value += 256;
            index = texture_offset + dst_x * tiles_width - x_offset;
            for (int i = 0; i < tiles_width; ++i)
                datas[index++] = _BGP0[_texture_background[value][i][tile_y]];
            index_address %= (tile_maps_width / tiles_width);
        }
        _wnd_line++;
    }
}
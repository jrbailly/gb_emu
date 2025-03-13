#include "lcd.h"
#include "ram.h"
#include <cstdio>

LCD::LCD(MBC1 &ram) : mRAM(ram)
{
    mNext_line_cycle = 456;
    mWindow = SDL_CreateWindow("", 320, 288, 0);
    mRenderer = SDL_CreateRenderer(mWindow, NULL);
    mSurfaceSprites = SDL_CreateSurface(line_width, 2 * height_tiles, SDL_PIXELFORMAT_RGBA8888);
    mSurfaceBackground = SDL_CreateSurface(line_width, height_tiles, SDL_PIXELFORMAT_RGBA8888);
    mColors[GrayLevel::WHITE] = 0xFFFFFFFF;
    mColors[GrayLevel::LIGHT_GRAY] = 0xD3D3D3FF;
    mColors[GrayLevel::DARK_GRAY] = 0xA9A9A9FF;
    mColors[GrayLevel::BLACK] = 0x000000FF;
    mColors[GrayLevel::TRANSPARENT] = 0x0;
    mBGP0[0] = mColors[GrayLevel::WHITE];
    mBGP0[1] = mColors[GrayLevel::LIGHT_GRAY];
    mBGP0[2] = mColors[GrayLevel::DARK_GRAY];
    mBGP0[3] = mColors[GrayLevel::BLACK];
    mReloadSurface = true;
}

void LCD::step(uint32_t cycles_count, uint16_t last_addr)
{
    if (last_addr == Register::BGP)
        updateBGP0();
    else if (last_addr == Register::OBP0)
        updateOBP0();
    else if (last_addr == Register::OBP1)
        updateBGP1();
    else if (last_addr >= 0x8000 && last_addr < 0x9800)
        mReloadSurface = true;
    if (cycles_count > mNext_line_cycle)
    {
        uint8_t ly = mRAM[Register::LY] + 1;

        mRAM.write(Register::LY, ly);
        mNext_line_cycle += cycles_per_line;

        if (ly == 144) // vblank
            mRAM.write(CPU::Register::IF, 0x1);
        updateStat();
    }
}

void LCD::renderer()
{
    if (mReloadSurface)
    {
        loadSurfaceBackground();
        loadSurfaceSprites();
        mTextureSprites = SDL_CreateTextureFromSurface(mRenderer, mSurfaceSprites);
        mTextureBackground = SDL_CreateTextureFromSurface(mRenderer, mSurfaceBackground);
    }
    if (mRAM[Register::LCDC] & LCD_ENABLE)
    {
        SDL_RenderClear(mRenderer);
        if (mRAM[Register::LCDC] & BG_ENABLE)
            drawBackground();
        if (mRAM[Register::LCDC] & OBJ_ENABLE)
            drawSprites();
        SDL_SetRenderScale(mRenderer, 2, 2);
        SDL_RenderPresent(mRenderer);
    }
}

void LCD::reset()
{
    mNext_line_cycle = cycles_per_line;
    mRAM.write(Register::LY, 0);
    mReloadSurface = false;
}

void LCD::updateStat()
{
    uint8_t stat = mRAM[Register::STAT] & 0xF8;
    uint8_t ly = mRAM[Register::LYC];
    uint8_t lyc = mRAM[Register::LYC];
    uint8_t interrupt = mRAM[CPU::Register::IF];
    uint8_t mode = 0;

    if (ly >= 144)
        stat |= 0x1;
    if (ly == lyc)
        stat |= 0x4;
    if ((stat & 0x10) && (ly == 144))
        interrupt |= 0x2;
    if ((stat & 0x40) && (ly == lyc))
        interrupt |= 0x2;
    mRAM.write(CPU::Register::IF, interrupt);
    mRAM.write(Register::STAT, stat);
}

void LCD::updateBGP0()
{
    uint8_t palette = mRAM[Register::BGP];

    for (int i = 0; i < 8; i += 2)
        mBGP0[i / 2] = mColors[(palette >> i) & 0x3];
    mReloadSurface = true;
}

void LCD::updateOBP0()
{
    uint8_t palette = mRAM[Register::OBP0];

    for (int i = 0; i < 8; i += 2)
    {
        mOBP0[i / 2] = mColors[(palette >> i) & 0x3];
        if (((palette >> i) & 0x3) == 0)
            mOBP0[i / 2] = mColors[GrayLevel::TRANSPARENT];
    }
    mReloadSurface = true;
}

void LCD::updateBGP1()
{
    uint8_t palette = mRAM[Register::OBP1];

    for (int i = 0; i < 8; i += 2)
    {
        mOBP1[i / 2] = mColors[(palette >> i) & 0x3];
        if (((palette >> i) & 0x3) == 0)
            mOBP1[i / 2] = mColors[GrayLevel::TRANSPARENT];
    }
    mReloadSurface = true;
}

void LCD::loadSurfaceSprites()
{
    uint32_t *datas = static_cast<uint32_t *>(mSurfaceSprites->pixels);
    uint16_t address = TilesAddress::BLOCK0;
    uint8_t value;

    for (int i = 0; i < 256; ++i)
    {
        for (int j = 0; j < height_tiles; ++j)
        {
            for (int k = 0; k < 8; k++)
            {
                value = (((mRAM[address] >> (7 - k)) & 1) << 1) | ((mRAM[address + 1] >> (7 - k)) & 1);
                datas[(j * line_width) + (i * width_tiles) + k] = mOBP0[value];
                datas[((j + height_tiles) * line_width) + (i * width_tiles) + k] = mOBP1[value];
            }
            address += 2;
        }
    }
}

void LCD::loadSurfaceBackground()
{
    uint32_t *datas = static_cast<uint32_t *>(mSurfaceBackground->pixels);
    uint16_t address = TilesAddress::BLOCK2;
    uint8_t value;

    if (mRAM[Register::LCDC] & BG_DATA_AREA)
        address = TilesAddress::BLOCK0;
    for (int i = 0; i < 256; ++i)
    {
        if (address == TilesAddress::BLOCK2 + 0x800)
            address = TilesAddress::BLOCK1;
        for (int line = 0; line < height_tiles; ++line)
        {
            for (int k = 0; k < 8; k++)
            {
                value = (((mRAM[address] >> (7 - k)) & 1) << 1) | ((mRAM[address + 1] >> (7 - k)) & 1);
                datas[(line * line_width) + (i * width_tiles) + k] = mBGP0[value];
            }
            address += 2;
        }
    }
}

void LCD::drawSprites()
{
    uint16_t address = oam_address;
    uint8_t attribute;
    uint8_t value;
    float angle;
    SDL_FRect src;
    SDL_FRect dst;
    SDL_FlipMode flip;

    SDL_SetTextureScaleMode(mTextureSprites, SDL_SCALEMODE_NEAREST);
    for (int i = 0; i < 40; ++i)
    {
        flip = SDL_FLIP_NONE;
        angle = 0;
        dst.y = mRAM[address] - 16;
        dst.x = mRAM[address + 1] - 8;
        value = mRAM[address + 2];
        attribute = mRAM[address + 3];

        src.y = 0;
        if (attribute & 0x10)
            src.y = height_tiles;
        if (attribute & 0x20)
        {
            flip = SDL_FLIP_VERTICAL;
            angle = 180;
        }
        if (attribute & 0x40)
        {
            flip = SDL_FLIP_HORIZONTAL;
            angle = 180;
        }
        address += 4;
        src.x = (value * width_tiles);
        src.w = width_tiles;
        src.h = height_tiles;
        dst.w = width_tiles;
        dst.h = height_tiles;
        SDL_RenderTextureRotated(mRenderer, mTextureSprites, &src, &dst, angle, nullptr, flip);
    }
}

void LCD::drawBackground()
{
    int address = BackgroundAddress::AREA0;
    int y = mRAM[SCY];
    int x = mRAM[SCX];
    int wy = mRAM[WY];
    int wx = mRAM[WX];
    int y_offset = y % 8;
    int x_offset = x % 8;
    uint8_t value;
    SDL_FRect src;
    SDL_FRect dst;

    // std::cout << (mRAM[LCDC] & WIN_ENABLE) << " " << y << " " << x << " " << wx << " " << wy << std::endl;
    SDL_SetTextureScaleMode(mTextureBackground, SDL_SCALEMODE_NEAREST);
    if (mRAM[Register::LCDC] & BG_TILE_AREA)
        address = BackgroundAddress::AREA1;
    for (int dst_y = 0; dst_y <= 18; ++dst_y)
    {
        x = mRAM[SCX];
        for (int dst_x = 0; dst_x <= 20; ++dst_x)
        {
            value = mRAM[address + (y * 4 + x / 8)];
            src.x = (value * width_tiles);
            src.y = 0;
            src.w = width_tiles;
            src.h = height_tiles;
            dst.x = (dst_x * width_tiles) - x_offset;
            dst.y = (dst_y * height_tiles) - y_offset;
            dst.w = width_tiles;
            dst.h = height_tiles;
            SDL_RenderTexture(mRenderer, mTextureBackground, &src, &dst);
            x = (x + 8) % 256;
        }
        y = (y + 8) % 256;
    }
}

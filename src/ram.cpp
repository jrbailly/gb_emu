#include "ram.h"
#include "cartridge.h"
#include <fstream>
/**
 * @brief Construct a new RamBus object and fill the memory with '0'.
 *
 */
RamBus::RamBus()
{
    _write_callbacks.resize(0x10000);
    _ram.fill(0);
}

/**
 * @brief Write a memory range in ram
 *
 * @param datas Source array to copy
 * @param size Size to copy
 * @param address Destination address
 */
auto RamBus::write_range(const unsigned char *datas, int size, int address) -> void
{
    if (address + size > Ram::ram_size)
        throw std::runtime_error(std::string("RamBus::write_range invalid size : ") + std::to_string(size));
    std::copy(datas, datas + size, _ram.begin() + address);
}

/**
 * @brief Load a saved cartridge ram from file
 *
 * @param romfile ROM filepath
 */
auto RamBus::load_ram(const std::string &rom_file) -> void
{
    std::string filename = rom_file + ".ram";
    std::ifstream input(filename.data(), std::ios::binary);
    std::streamsize bytesRead;
    std::array<unsigned char, ROM_RAM_SIZE> block;

    if (input)
    {

        input.read(reinterpret_cast<char *>(block.data()), ROM_RAM_SIZE);
        bytesRead = input.gcount();
        if (bytesRead == ROM_RAM_SIZE)
            std::copy(block.begin(), block.end(), _ram.begin() + ROM_RAM_ADDRESS);
    }
}

/**
 * @brief Save cartridge ram to a file. The extension ".ram" will be added to the rom file.
 *
 * @param rom_file ROM filepath
 */
auto RamBus::save_ram(const std::string &rom_file) -> void
{
    std::string filename = rom_file + ".ram";
    std::ofstream output(filename.data(), std::ios::binary);
    std::streamsize bytesRead;

    if (output)
        output.write(reinterpret_cast<char *>(&_ram[ROM_RAM_ADDRESS]), ROM_RAM_SIZE);
}

/**
 * @brief Register a callback to call when a write is done at a specific address
 *
 * @param address Register address
 * @param fnc Callback function
 */
auto RamBus::register_callback(int address, ram_callback fnc) -> void
{
    _write_callbacks[address] = fnc;
}

/**
 * @brief Register a callback to call when a write is done at a specific address
 *
 * @param start_address Register address
 * @param size Range length
 * @param fnc Callback function
 */
auto RamBus::register_callback_range(int start_address, unsigned int size, ram_callback fnc) -> void
{
    for (int i = 0; i < size; ++i)
        _write_callbacks[start_address + i] = fnc;
}

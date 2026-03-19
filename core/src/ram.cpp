#include "ram.h"
#include "cartridge.h"
#include <format>
#include <fstream>

/**
 * @brief Construct a new RamBus object and fill the memory with '0'.
 *
 */
RamBus::RamBus(bool unittest) : _unittest(unittest)
{
    _write_callbacks.resize(0x10000);
    clear();
}

/**
 * @brief Clear memory, fill with '0'
 */
auto RamBus::clear() -> void
{
    _ram.fill(0);
}

/**
 * @brief Read a memory range in ram
 *
 * @param datas Destination array to copy
 * @param size Size to copy
 * @param address Address
 */
auto RamBus::read_range(unsigned int address, size_t size, unsigned char *dst_datas) -> void
{
    if (address + size > Ram::ram_size)
        throw std::runtime_error(std::format("RamBus::write_range invalid size : ") + std::to_string(size));
    std::copy(_ram.begin() + address, _ram.begin() + address + size, dst_datas);
}

/**
 * @brief Write a memory range in ram
 *
 * @param datas Source array to copy
 * @param size Size to copy
 * @param address Destination address
 */
auto RamBus::write_range(const unsigned char *datas, size_t size, unsigned int address) -> void
{
    if (address + size > Ram::ram_size)
        throw std::runtime_error(std::format("RamBus::write_range invalid size : ") + std::to_string(size));
    std::copy(datas, datas + size, _ram.begin() + address);
}

/**
 * @brief Write a memory range in ram
 *
 * @param start_address Source address
 * @param dst_address Destination address to copy
 * @param size Copy size
 */
auto RamBus::write_range(unsigned int start_address, unsigned int dst_address, size_t size) -> void
{
    if (dst_address + size > Ram::ram_size)
        throw std::runtime_error(std::format("RamBus::write_range invalid size : ") + std::to_string(size));
    std::copy(_ram.begin() + start_address, _ram.begin() + start_address + size, _ram.begin() + dst_address);
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
auto RamBus::register_callback_range(int start_address, size_t size, ram_callback fnc) -> void
{
    for (size_t i = 0; i < size; ++i)
        _write_callbacks[start_address + i] = fnc;
}

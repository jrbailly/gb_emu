#ifndef _ISERIALIZABLE_H_
#define _ISERIALIZABLE_H_

#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include <vector>

using StateValue = std::variant<int, float, std::vector<uint8_t>>;
using StateMap = std::map<std::string, StateValue>;

class ISerializable
{
  public:
    virtual auto save_state(StateMap &state) -> void = 0;
    virtual auto load_state(const StateMap &state) -> void = 0;
    virtual ~ISerializable() = default;
};

#endif

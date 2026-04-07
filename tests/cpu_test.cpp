#include "cpu.h"
#include "ram.h"
#include <filesystem>
#include <format>
#include <fstream>
#include <gtest/gtest.h>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

std::vector<std::string> GetJsonFiles(const std::string &directory)
{
    std::vector<std::string> files;
    for (const auto &entry : std::filesystem::directory_iterator(directory))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".json")
            files.push_back(entry.path().string());
    }
    return files;
}

class CpuInstructionTest : public ::testing::TestWithParam<std::string>
{
  protected:
    void RunInstructionTest(const std::string &jsonFile)
    {
        RamBus ram(true);
        CPU cpu(ram);
        std::ifstream file(jsonFile);
        nlohmann::json testData;
        StateMap registers;
        std::string name;

        registers["interrupt"] = 0;
        if (!file.is_open())
            FAIL() << "Cannot open file : " << jsonFile;
        file >> testData;

        for (const auto &test : testData)
        {
            // set initial state
            ram.clear();
            name = test["name"];
            for (const auto &[key, value] : test["initial"].items())
            {
                if (value.is_number())
                    registers[key] = value.get<int>();
                else if (key == "ram")
                    for (const auto &ram_value : value)
                        ram.write_register(ram_value[0].get<int>(), ram_value[1].get<int>());
            }
            cpu.load_state(registers);

            // run
            uint8_t actual_cycles = cpu.step();

            // final state
            cpu.save_state(registers);
            for (const auto &[key, value] : test["final"].items())
            {
                if (value.is_number())
                    EXPECT_EQ(std::get<int>(registers[key]), value.get<int>())
                        << std::format("Test \"{}\" Register \"{}\"", name, key);
                else if (key == "ram")
                {
                    for (const auto &ram_value : value)
                    {
                        int address = ram_value[0].get<int>();
                        EXPECT_EQ(ram[address], ram_value[1].get<int>())
                            << std::format("Test \"{}\" Address \"{}\" ", name, address);
                    }
                }
            }
            if (test.contains("cycles"))
                EXPECT_EQ(actual_cycles, test["cycles"].get<int>()) << std::format("Test \"{}\" Cycles", name);
        }
    }
};

TEST_P(CpuInstructionTest, RunTestFromJson)
{
    std::string jsonFile = GetParam();
    RunInstructionTest(jsonFile);
}

INSTANTIATE_TEST_SUITE_P(CpuInstructionTests, CpuInstructionTest,
                         ::testing::ValuesIn(GetJsonFiles("../tests/cpu_tests")));

#include "controllers.h"
#include "cpu.h"
#include "ram.h"
#include <gtest/gtest.h>

// JOYP selection is active-low:
//   bit 5 (BUTTON) = 0 → button row selected → _buttons returned
//   bit 4 (DPAD)   = 0 → dpad   row selected → _dpads  returned
// A pressed button/dpad clears the corresponding bit in the low nibble (active-low).
// Expected JOYP pattern: ~(ESelect::X | pressed_mask)
//   → clears the select bit (row active) and the input bit (key pressed)
//   → upper bits 7-6 stay 1 (0xC0), unused select bit stays 1

class ControllersTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        ram.clear();
        controllers.init(ram);
    }

    /**
     * @brief Computes the expected JOYP register value for a given selection and pressed inputs.
     * @param select  One or both ESelect bits OR-ed together. Controls which row is active (active-low).
     * @param button_mask Pressed DButtonMask bit(s). Default 0 means no button pressed.
     * @param pad_mask    Pressed DPadMask bit(s). Default 0 means no direction pressed.
     * @return Expected uint8_t value of the JOYP register.
     */
    static uint8_t expected_joyp(uint8_t select = 0, uint8_t button_mask = 0, uint8_t pad_mask = 0)
    {
        return (~(select | button_mask | pad_mask));
    }

    RamBus ram{true};
    Controllers controllers{ram};
};

/**
 * @brief Verifies that pressing button A triggers the JOYPAD interrupt and reflects the correct JOYP value.
 * @details set_input is called with A pressed (bit 0 cleared). The button row is selected by writing
 *          ~ESelect::BUTTON to JOYP. Expects IF JOYPAD flag set and JOYP = ~(BUTTON | A).
 */
TEST_F(ControllersTest, ButtonA_Pressed)
{
    controllers.set_input(0xF, 0xF ^ Controllers::DButtonMask::A);
    ram.write(Controllers::Register::JOYP, ~Controllers::ESelect::BUTTON);

    EXPECT_EQ(ram[CPU::Register::IF] & CPU::IFFlag::JOYPAD, CPU::IFFlag::JOYPAD);
    EXPECT_EQ(ram[Controllers::Register::JOYP],
              expected_joyp(Controllers::ESelect::BUTTON, Controllers::DButtonMask::A));
}

/**
 * @brief Verifies that pressing button B triggers the JOYPAD interrupt and reflects the correct JOYP value.
 * @details set_input is called with B pressed (bit 1 cleared). The button row is selected by writing
 *          ~ESelect::BUTTON to JOYP. Expects IF JOYPAD flag set and JOYP = ~(BUTTON | B).
 */
TEST_F(ControllersTest, ButtonB_Pressed)
{
    controllers.set_input(0xF, 0xF ^ Controllers::DButtonMask::B);
    ram.write(Controllers::Register::JOYP, ~Controllers::ESelect::BUTTON);

    EXPECT_EQ(ram[CPU::Register::IF] & CPU::IFFlag::JOYPAD, CPU::IFFlag::JOYPAD);
    EXPECT_EQ(ram[Controllers::Register::JOYP],
              expected_joyp(Controllers::ESelect::BUTTON, Controllers::DButtonMask::B));
}

/**
 * @brief Verifies that pressing SELECT triggers the JOYPAD interrupt and reflects the correct JOYP value.
 * @details set_input is called with SELECT pressed (bit 2 cleared). The button row is selected by writing
 *          ~ESelect::BUTTON to JOYP. Expects IF JOYPAD flag set and JOYP = ~(BUTTON | SELECT).
 */
TEST_F(ControllersTest, ButtonSelect_Pressed)
{
    controllers.set_input(0xF, 0xF ^ Controllers::DButtonMask::SELECT);
    ram.write(Controllers::Register::JOYP, ~Controllers::ESelect::BUTTON);

    EXPECT_EQ(ram[CPU::Register::IF] & CPU::IFFlag::JOYPAD, CPU::IFFlag::JOYPAD);
    EXPECT_EQ(ram[Controllers::Register::JOYP],
              expected_joyp(Controllers::ESelect::BUTTON, Controllers::DButtonMask::SELECT));
}

/**
 * @brief Verifies that pressing START triggers the JOYPAD interrupt and reflects the correct JOYP value.
 * @details set_input is called with START pressed (bit 3 cleared). The button row is selected by writing
 *          ~ESelect::BUTTON to JOYP. Expects IF JOYPAD flag set and JOYP = ~(BUTTON | START).
 */
TEST_F(ControllersTest, ButtonStart_Pressed)
{
    controllers.set_input(0xF, 0xF ^ Controllers::DButtonMask::START);
    ram.write(Controllers::Register::JOYP, ~Controllers::ESelect::BUTTON);

    EXPECT_EQ(ram[CPU::Register::IF] & CPU::IFFlag::JOYPAD, CPU::IFFlag::JOYPAD);
    EXPECT_EQ(ram[Controllers::Register::JOYP],
              expected_joyp(Controllers::ESelect::BUTTON, Controllers::DButtonMask::START));
}

/**
 * @brief Verifies that pressing RIGHT triggers the JOYPAD interrupt and reflects the correct JOYP value.
 * @details set_input is called with RIGHT pressed (bit 0 cleared). The dpad row is selected by writing
 *          ~ESelect::DPAD to JOYP. Expects IF JOYPAD flag set and JOYP = ~(DPAD | RIGHT).
 */
TEST_F(ControllersTest, DPad_Right_Pressed)
{
    controllers.set_input(0xF ^ Controllers::DPadMask::RIGHT, 0xF);
    ram.write(Controllers::Register::JOYP, ~Controllers::ESelect::DPAD);

    EXPECT_EQ(ram[CPU::Register::IF] & CPU::IFFlag::JOYPAD, CPU::IFFlag::JOYPAD);
    EXPECT_EQ(ram[Controllers::Register::JOYP],
              expected_joyp(Controllers::ESelect::DPAD, 0, Controllers::DPadMask::RIGHT));
}

/**
 * @brief Verifies that pressing LEFT triggers the JOYPAD interrupt and reflects the correct JOYP value.
 * @details set_input is called with LEFT pressed (bit 1 cleared). The dpad row is selected by writing
 *          ~ESelect::DPAD to JOYP. Expects IF JOYPAD flag set and JOYP = ~(DPAD | LEFT).
 */
TEST_F(ControllersTest, DPad_Left_Pressed)
{
    controllers.set_input(0xF ^ Controllers::DPadMask::LEFT, 0xF);
    ram.write(Controllers::Register::JOYP, ~Controllers::ESelect::DPAD);

    EXPECT_EQ(ram[CPU::Register::IF] & CPU::IFFlag::JOYPAD, CPU::IFFlag::JOYPAD);
    EXPECT_EQ(ram[Controllers::Register::JOYP],
              expected_joyp(Controllers::ESelect::DPAD, 0, Controllers::DPadMask::LEFT));
}

/**
 * @brief Verifies that pressing UP triggers the JOYPAD interrupt and reflects the correct JOYP value.
 * @details set_input is called with UP pressed (bit 2 cleared). The dpad row is selected by writing
 *          ~ESelect::DPAD to JOYP. Expects IF JOYPAD flag set and JOYP = ~(DPAD | UP).
 */
TEST_F(ControllersTest, DPad_Up_Pressed)
{
    controllers.set_input(0xF ^ Controllers::DPadMask::UP, 0xF);
    ram.write(Controllers::Register::JOYP, ~Controllers::ESelect::DPAD);

    EXPECT_EQ(ram[CPU::Register::IF] & CPU::IFFlag::JOYPAD, CPU::IFFlag::JOYPAD);
    EXPECT_EQ(ram[Controllers::Register::JOYP],
              expected_joyp(Controllers::ESelect::DPAD, 0, Controllers::DPadMask::UP));
}

/**
 * @brief Verifies that pressing DOWN triggers the JOYPAD interrupt and reflects the correct JOYP value.
 * @details set_input is called with DOWN pressed (bit 3 cleared). The dpad row is selected by writing
 *          ~ESelect::DPAD to JOYP. Expects IF JOYPAD flag set and JOYP = ~(DPAD | DOWN).
 */
TEST_F(ControllersTest, DPad_Down_Pressed)
{
    controllers.set_input(0xF ^ Controllers::DPadMask::DOWN, 0xF);
    ram.write(Controllers::Register::JOYP, ~Controllers::ESelect::DPAD);

    EXPECT_EQ(ram[CPU::Register::IF] & CPU::IFFlag::JOYPAD, CPU::IFFlag::JOYPAD);
    EXPECT_EQ(ram[Controllers::Register::JOYP],
              expected_joyp(Controllers::ESelect::DPAD, 0, Controllers::DPadMask::DOWN));
}

/**
 * @brief Verifies the AND behaviour when both rows are selected simultaneously (A + DOWN).
 * @details Both select bits are cleared (~(BUTTON | DPAD)), so the callback returns _buttons & _dpads.
 *          A (0x01) and DOWN (0x08) have distinct bit positions: _buttons = 0xE, _dpads = 0x7, AND = 0x6.
 *          Expects IF JOYPAD flag set and JOYP = ~(BUTTON | DPAD | A | DOWN).
 */
TEST_F(ControllersTest, Select0x00_A_And_Down_Pressed)
{
    controllers.set_input(0xF ^ Controllers::DPadMask::DOWN, 0xF ^ Controllers::DButtonMask::A);
    ram.write(Controllers::Register::JOYP, ~(Controllers::ESelect::BUTTON | Controllers::ESelect::DPAD));

    EXPECT_EQ(ram[CPU::Register::IF] & CPU::IFFlag::JOYPAD, CPU::IFFlag::JOYPAD);
    EXPECT_EQ(ram[Controllers::Register::JOYP],
              expected_joyp(Controllers::ESelect::BUTTON | Controllers::ESelect::DPAD,
                            Controllers::DButtonMask::A, Controllers::DPadMask::DOWN));
}

/**
 * @brief Verifies the AND behaviour when both rows are selected simultaneously (B + UP).
 * @details Both select bits are cleared (~(BUTTON | DPAD)), so the callback returns _buttons & _dpads.
 *          B (0x02) and UP (0x04) have distinct bit positions: _buttons = 0xD, _dpads = 0xB, AND = 0x9.
 *          Expects IF JOYPAD flag set and JOYP = ~(BUTTON | DPAD | B | UP).
 */
TEST_F(ControllersTest, Select0x00_B_And_Up_Pressed)
{
    controllers.set_input(0xF ^ Controllers::DPadMask::UP, 0xF ^ Controllers::DButtonMask::B);
    ram.write(Controllers::Register::JOYP, ~(Controllers::ESelect::BUTTON | Controllers::ESelect::DPAD));

    EXPECT_EQ(ram[CPU::Register::IF] & CPU::IFFlag::JOYPAD, CPU::IFFlag::JOYPAD);
    EXPECT_EQ(ram[Controllers::Register::JOYP],
              expected_joyp(Controllers::ESelect::BUTTON | Controllers::ESelect::DPAD,
                            Controllers::DButtonMask::B, Controllers::DPadMask::UP));
}

/**
 * @brief Verifies the AND behaviour when both rows are selected simultaneously (START + RIGHT).
 * @details Both select bits are cleared (~(BUTTON | DPAD)), so the callback returns _buttons & _dpads.
 *          START (0x08) and RIGHT (0x01) have distinct bit positions: _buttons = 0x7, _dpads = 0xE, AND = 0x6.
 *          Expects IF JOYPAD flag set and JOYP = ~(BUTTON | DPAD | START | RIGHT).
 */
TEST_F(ControllersTest, Select0x00_Start_And_Right_Pressed)
{
    controllers.set_input(0xF ^ Controllers::DPadMask::RIGHT, 0xF ^ Controllers::DButtonMask::START);
    ram.write(Controllers::Register::JOYP, ~(Controllers::ESelect::BUTTON | Controllers::ESelect::DPAD));

    EXPECT_EQ(ram[CPU::Register::IF] & CPU::IFFlag::JOYPAD, CPU::IFFlag::JOYPAD);
    EXPECT_EQ(ram[Controllers::Register::JOYP],
              expected_joyp(Controllers::ESelect::BUTTON | Controllers::ESelect::DPAD,
                            Controllers::DButtonMask::START, Controllers::DPadMask::RIGHT));
}

/**
 * @brief Verifies that when neither row is selected (both select bits high), all input bits read as 1.
 * @details Writing BUTTON | DPAD (0x30) sets both select lines high, deselecting both rows.
 *          The callback falls into the else branch and returns low = 0x0F regardless of pressed inputs.
 *          Expects JOYP = 0xFF (0xC0 | 0x30 | 0x0F).
 */
TEST_F(ControllersTest, NothingSelected_Returns0x0F)
{
    controllers.set_input(0xF ^ Controllers::DPadMask::LEFT, 0xF ^ Controllers::DButtonMask::A);
    ram.write(Controllers::Register::JOYP, Controllers::ESelect::BUTTON | Controllers::ESelect::DPAD);

    EXPECT_EQ(ram[CPU::Register::IF] & CPU::IFFlag::JOYPAD, CPU::IFFlag::JOYPAD);
    EXPECT_EQ(ram[Controllers::Register::JOYP], 0xFF);
}

/**
 * @brief Verifies that calling set_input with unchanged values does not trigger the JOYPAD interrupt.
 * @details A first call with A + LEFT pressed sets IF, which is then manually cleared.
 *          A second call with the exact same inputs must leave IF untouched.
 *          Expects IF JOYPAD flag clear and JOYP reflecting the current button state.
 */
TEST_F(ControllersTest, NoInterrupt_When_Inputs_Unchanged)
{
    int dpads   = 0xF ^ Controllers::DPadMask::LEFT;
    int buttons = 0xF ^ Controllers::DButtonMask::A;

    controllers.set_input(dpads, buttons);
    ram.write_register(CPU::Register::IF, 0x00);

    controllers.set_input(dpads, buttons);
    ram.write(Controllers::Register::JOYP, ~Controllers::ESelect::BUTTON);

    EXPECT_EQ(ram[CPU::Register::IF] & CPU::IFFlag::JOYPAD, 0);
    EXPECT_EQ(ram[Controllers::Register::JOYP],
              expected_joyp(Controllers::ESelect::BUTTON, Controllers::DButtonMask::A));
}

/**
 * @brief Verifies that releasing a button does not trigger the JOYPAD interrupt.
 * @details The interrupt must only fire on a falling edge (button newly pressed).
 *          A first call presses A, setting IF. After clearing IF, a second call
 *          releases A (no new press). IF must remain clear.
 */
TEST_F(ControllersTest, NoInterrupt_When_Button_Released)
{
    controllers.set_input(0xF, 0xF ^ Controllers::DButtonMask::A); // press A → interrupt fires
    ram.write_register(CPU::Register::IF, 0x00);                    // clear interrupt flag

    controllers.set_input(0xF, 0xF); // release A → no new press, no interrupt

    EXPECT_EQ(ram[CPU::Register::IF] & CPU::IFFlag::JOYPAD, 0);
}

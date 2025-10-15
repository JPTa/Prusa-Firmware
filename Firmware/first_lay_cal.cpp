//! @file
//! @date Jun 10, 2019
//! @author Marek Bel
//! @brief First layer (Z offset) calibration

#include "first_lay_cal.h"
#include "Configuration_var.h"
#include "Marlin.h"
#include "messages.h"
#include "cmdqueue.h"
#include "mmu2.h"
#include <avr/pgmspace.h>
#include <math.h>

//! @brief Count extrude length
//!
//! @param layer_height layer height in mm
//! @param extrusion_width extrusion width in mm
//! @param extrusion_length extrusion length in mm
//! @return filament length in mm which needs to be extruded to form line
static constexpr float __attribute__((noinline)) count_e(float layer_height, float extrusion_width, float extrusion_length, float filament_diameter=1.75f)
{
    return (extrusion_length * ((M_PI * pow(layer_height, 2)) / 4 + layer_height * (extrusion_width - layer_height))) / ((M_PI * pow(filament_diameter, 2)) / 4);
}

//! @brief Extrusion spacing
//!
//! @param layer_height layer height in mm
//! @param extrusion_width extrusion width in mm
//! @return filament length in mm which needs to be extruded to form line
static constexpr float spacing(float layer_height, float extrusion_width, float overlap_factor=1.f)
{
    return extrusion_width - layer_height * (overlap_factor - M_PI/4);
}

// Common code extracted into one function to reduce code size
static void lay1cal_common_enqueue_loop(const char * const * cmd_sequence, const uint8_t steps) {
    for (uint8_t i = 0; i < steps; ++i)
    {
        void * const pgm_ptr = pgm_read_ptr(cmd_sequence + i);

        // M702 is currently only used with MMU enabled
        if (pgm_ptr == MSG_M702 && !MMU2::mmu2.Enabled()) {
            continue;
        }

        enquecommand_P(static_cast<char*>(pgm_ptr));
    }
}

static const char extrude_fmt_X[] PROGMEM = "G1X%.4fE%.4f";
static const char extrude_fmt_Y[] PROGMEM = "G1Y%.4fE%.4f";
static const char zero_extrusion[] PROGMEM = "G92E0";
static const char feedrate_F1080[] PROGMEM = "G1F1080";
#ifndef NEW_FIRST_LAYER_CAL
static constexpr int8_t invert = 1;
static constexpr float short_length = 20;
static constexpr float square_width = short_length;
#else
static constexpr int8_t invert = -1;
static constexpr float short_length = 13.2812; //max_pos[1]/2 / meander * 2
static constexpr float square_width = short_length*2;
#endif //NEW_FIRST_LAYER_CAL
static constexpr float long_length = 150;

//! @brief Wait for preheat
void lay1cal_wait_preheat()
{
    static const char preheat_cmd_2[] PROGMEM = "M190";
    static const char preheat_cmd_3[] PROGMEM = "M109";
    static const char preheat_cmd_4[] PROGMEM = "G28";

    static const char * const preheat_cmd[] PROGMEM =
    {
        MSG_M107,
        preheat_cmd_2,
        preheat_cmd_3,
        preheat_cmd_4,
        zero_extrusion
    };

    lay1cal_common_enqueue_loop(preheat_cmd, sizeof(preheat_cmd)/sizeof(preheat_cmd[0]));
}

//! @brief Load filament
//! @param cmd_buffer character buffer needed to format gcodes
//! @param filament filament to use (applies for MMU only)
//! @returns true if extra purge distance is needed in case of MMU prints (after a toolchange), otherwise false
bool lay1cal_load_filament(uint8_t filament)
{
    if (MMU2::mmu2.Enabled())
    {
        enquecommand_P(MSG_M83);
        enquecommand_P(PSTR("G1Y-3F1000"));
        enquecommand_P(PSTR("G1Z0.4"));

        uint8_t currentTool = MMU2::mmu2.get_current_tool();
        if(currentTool == filament ){
            // already have the correct tool loaded - do nothing
            return false;
        } else if( currentTool != (uint8_t)MMU2::FILAMENT_UNKNOWN){
            // some other slot is loaded, perform an unload first
            enquecommand_P(MSG_M702);
        }
        // perform a toolchange
        enquecommandf_P(PSTR("T%d"), filament);
        return true;
    }
    return false;
}

//! @brief Print intro line
//! @param extraPurgeNeeded false if the first MMU-related "G1 E29" have to be skipped because the nozzle is already full of filament
//! @param layer_height the height of the calibration layer
//! @param extrusion_width the width of the extrusion layer
void lay1cal_intro_line(bool extraPurgeNeeded, float layer_height, float extrusion_width)
{
    static const char cmd_intro_mmu_3[] PROGMEM = "G1X55E29F1073";
    static const char cmd_intro_mmu_4[] PROGMEM = "G1X5E29F1800";
    static const char cmd_intro_mmu_5[] PROGMEM = "G1X55E8F2000";
    static const char cmd_intro_mmu_6[] PROGMEM = "G1Z0.3F1000";
    static const char cmd_intro_mmu_8[] PROGMEM = "G1X240E25F2200";
    static const char cmd_intro_mmu_9[] PROGMEM = "G1Y-2F1000";
    static const char cmd_intro_mmu_10[] PROGMEM = "G1X202.5E8F1400";
    static const char cmd_intro_mmu_11[] PROGMEM = "G1Z0.2";
    static const char * const cmd_intro_mmu[] PROGMEM =
    {
        // first 2 items are only relevant if filament was not loaded - i.e. extraPurgeNeeded == true
        cmd_intro_mmu_3,
        cmd_intro_mmu_4,
        cmd_intro_mmu_5,
        cmd_intro_mmu_6,
        zero_extrusion,
        cmd_intro_mmu_8,
        cmd_intro_mmu_9,
        cmd_intro_mmu_10,
        cmd_intro_mmu_11,
    };

    if (MMU2::mmu2.Enabled())
    {
        for (uint8_t i = (extraPurgeNeeded ? 0 : 2); i < (sizeof(cmd_intro_mmu)/sizeof(cmd_intro_mmu[0])); ++i)
        {
            enquecommand_P(static_cast<char*>(pgm_read_ptr(&cmd_intro_mmu[i])));
        }
    }
    else
    {
        enquecommand_P(feedrate_F1080); //fixed velocity for the intro line
        enquecommandf_P(extrude_fmt_X, 60.f, count_e(layer_height, extrusion_width * 4.f, 60));
        enquecommandf_P(extrude_fmt_X, 202.5f, count_e(layer_height, extrusion_width * 8.f, 142.5));
    }
}

//! @brief Setup for printing meander
void lay1cal_before_meander()
{
#ifndef NEW_FIRST_LAYER_CAL
    static const char cmd_pre_meander_4[] PROGMEM = "G1E-1.5F2100";
    static const char cmd_pre_meander_5[] PROGMEM = "G1Z5F7200";
#endif //NEW_FIRST_LAYER_CAL
    static const char cmd_pre_meander_6[] PROGMEM = "M204S1000"; //set acceleration

    static const char * const cmd_pre_meander[] PROGMEM =
    {
            zero_extrusion,
            MSG_G90,
            MSG_M83, // use relative distances for extrusion
#ifndef NEW_FIRST_LAYER_CAL
            cmd_pre_meander_4,
            cmd_pre_meander_5,
#endif //NEW_FIRST_LAYER_CAL
            cmd_pre_meander_6,
    };

    lay1cal_common_enqueue_loop(cmd_pre_meander, (sizeof(cmd_pre_meander)/sizeof(cmd_pre_meander[0])));
}

//! @brief Print meander start
void lay1cal_meander_start(float layer_height, float extrusion_width)
{
#ifndef NEW_FIRST_LAYER_CAL
    enquecommand_P(PSTR("G1X50Y155"));
#endif //_NEW_FIRST_LAYER_CAL
    static const char fmt1[] PROGMEM = "G1Z%.2f";
    enquecommandf_P(fmt1, layer_height);
    enquecommand_P(feedrate_F1080);
    enquecommand_P(MSG_G91); //enable relative XYZ
#ifdef NEW_FIRST_LAYER_CAL
    enquecommandf_P(extrude_fmt_Y, short_length, count_e(layer_height, extrusion_width, short_length));
    enquecommandf_P(extrude_fmt_X, long_length*invert, count_e(layer_height, extrusion_width, long_length));
    enquecommandf_P(extrude_fmt_Y, -short_length*invert, count_e(layer_height, extrusion_width, short_length));
#else
    enquecommandf_P(extrude_fmt_X, 25.f*invert, count_e(layer_height, extrusion_width * 4.f, 25));
    enquecommandf_P(extrude_fmt_X, 25.f*invert, count_e(layer_height, extrusion_width * 2.f, 25));
    enquecommandf_P(extrude_fmt_X, 100.f*invert, count_e(layer_height, extrusion_width, 100));
    enquecommandf_P(extrude_fmt_Y, -20.f*invert, count_e(layer_height, extrusion_width, 20));
#endif //_NEW_FIRST_LAYER_CAL
}

//! @brief Print meander
//! @param cmd_buffer character buffer needed to format gcodes
void lay1cal_meander(float layer_height, float extrusion_width)
{
    const float long_extrusion = count_e(layer_height, extrusion_width, long_length);
    const float short_extrusion = count_e(layer_height, extrusion_width, short_length);

    for(int8_t i = 0, xdir = -invert; i <= 4; i++, xdir = -xdir)
    {
        enquecommandf_P(extrude_fmt_X, xdir * long_length, long_extrusion);
        enquecommandf_P(extrude_fmt_Y, invert * -short_length, short_extrusion);
    }
#ifdef NEW_FIRST_LAYER_CAL
    constexpr float mid_length = 0.5f * long_length - 0.5f * square_width;
    const float mid_extrusion = count_e(layer_height, extrusion_width, mid_length);
    enquecommandf_P(extrude_fmt_X, -mid_length, mid_extrusion); //~Middle of bed X125
    enquecommandf_P(extrude_fmt_Y, short_length, short_extrusion); //~Middle of bed Y105
#endif //NEW_FIRST_LAYER_CAL

}

//! @brief Print square
//!
//! This function enqueues 4 lines of the square, so it needs to be called multiple times
//!
//! @param cmd_buffer character buffer needed to format gcodes
void lay1cal_square(float layer_height, float extrusion_width)
{
    const float Y_spacing = spacing(layer_height, extrusion_width);
    const float long_extrusion = count_e(layer_height, extrusion_width, square_width);
    const float short_extrusion = count_e(layer_height, extrusion_width, Y_spacing);

    for (uint8_t i = 0; i < 4; i++)
    {
        enquecommandf_P(extrude_fmt_X, square_width*invert, long_extrusion);
        enquecommandf_P(extrude_fmt_Y, -Y_spacing*invert, short_extrusion);
        enquecommandf_P(extrude_fmt_X, -square_width*invert, long_extrusion);
        enquecommandf_P(extrude_fmt_Y, -Y_spacing*invert, short_extrusion);
    }
}

void lay1cal_finish()
{
    static const char cmd_cal_finish_3[] PROGMEM = "G1E-0.075F2100"; // Retract
    static const char cmd_cal_finish_4[] PROGMEM = "M140S0";         // Turn off bed heater
    static const char cmd_cal_finish_5[] PROGMEM = "G1Z10F1300";     // Lift Z
    static const char cmd_cal_finish_6[] PROGMEM = "G1X10Y180F4000"; // Go to parking position
    static const char cmd_cal_finish_8[] PROGMEM = "M104S0";         // Turn off hotend heater

    static const char * const cmd_cal_finish[] PROGMEM =
    {
        MSG_G90,          // Set to Absolute Positioning
        MSG_M107,         // Turn off printer fan
        cmd_cal_finish_3, // Retract
        cmd_cal_finish_4, // Turn off bed heater
        cmd_cal_finish_5, // Lift Z
        cmd_cal_finish_6, // Go to parking position
        MSG_M702,         // Unload filament (MMU only)
        cmd_cal_finish_8, // Turn off hotend heater
        MSG_M84           // Disable stepper motors
    };

    lay1cal_common_enqueue_loop(cmd_cal_finish, (sizeof(cmd_cal_finish)/sizeof(cmd_cal_finish[0])));
}

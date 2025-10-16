
# Prusa-Firmware with support for OLED display / 0.9 deg motors

Combined work by others: 3d-gussner (OLED support), Guy Kuo (0.9 deg motors, geared extruders, etc.)

# VFA fix summary, firmware compilation, and shopping list

VFA's are a solved issue with 0.9 degree stepper motors. This post is a summary of what is needed to use 0.9 degree motors.

# Now based on Prusa MK3 branch (3.14.x)
This branch adds support for:

OLED displays (f.e. Winstar WEH002004AWPP5N00000 or Raystar REC002004BWPP5N00000)

0.9 degree motors on XYZ

Geared Extruders

Slice thermistor

Slice Magnum Mosquito

E3D Volcano

E3D Revo

Filament load/unload and z dimension changes for Slice magnum, Bondtech Prusa Upgrade MK3 & MK3S extruders, Skelestruder

Mini ramming during unload to reduce unloaded filament tip size. Does not require MMU2S for this feature.


## Firmware
You will need this 0.9 degree motor firmware.

Be able to compile this firmware before doing any hardware changes. Once motors have been changed, the first thing you need to do is update the firmware to this 0.9 motor support version.

Extruder microstep rate has been reduced to avoid overrunning EINSY. E-steps are now 1/2 of what was needed for prior BNB firmware. 

Set e-steps to new values via terminal window.
-
PLEASE DO A FACTORY RESET WITH DATA ERASURE after installing this firmware. Failure to clear out old EEPROM setttings can produce odd printer behavior.
-

### Compiling firmware for 0.9 degree motor support
Look in the upstream Prusa README.md document for instructions regarding compilation of the firmware. 
Warning: You are in experimental firmware territory here.


#### Set Printer Variant

Use the correct variant file for your printer model: 

Mk3 - mk3.h
Mk3s - mk3s.h

Copy either one to Firmware/Configuration_prusa.h and edit it to match your setup.

#### Set Motor Defines, Compile, Upload

My firmware branch includes changes for 0.9 degree motor support. All the important edits have been tagged with a comment.
Search for Kuo in all sketch tabs to view my changes.

For most users, you only need to specify which motors are 0.9 degree units.

1. Launch Arduino IDE

2. Open the Firmware.ino file which is inside your firmware folder

3. Select the Configuration_prusa.h tab

4. You must adjust defines in Configuration_prusa.h to match your actual motor setup for X Y and E axes. Also, if you have a BMG extruder,  uncomment #define BMG_EXTRUDER

Look for...

```
//Geared extruders now set for lower microstepping to avoid overruning EINSY during fast retracts or MMU2S filament moves. 
//Factory reset and delete all data after installing this firmware. Otherwise EEPROM settings override settings in this firmware.
//After installing this firmware, send M350 and M92 commands to force correct micro-stepping and e-step rates. M350 must be first
//because M350 command will sometimes alter existing M92 setting
//
//e-steps values for M92 depend on your extruder gearing.
//xxx = 280 for non-geared extruder
//xxx = 415 for BMG extruder (special Bondtech compensated value)
//xxx = 420 for 3:1 extruder
//xxx = 473 for BNBSX with 54:16 gearing
//xxx = 490 for BNBSX, Short Ears, Skelestruder with 56:16 gearing
//
//non-geared extruder, 1.8 degree motor
//M350 E32
//M92 E280
//M500
//
//non-geared extruder, 0.9 degree motor
//M350 E16
//M92 E280
//M500
//
//geared extruder, 1.8 degree motor
//M350 E16
//M92 Exxx
//M500
//
//geared extruder, 0.9 degree motor
//M350 E8
//M92 Exxx
//M500
//
//Follow with power off/on and M503 to verify settings are correct.

//====== Kuo Uncommented def(s) below specify 0.9 degree stepper motors on x, y, z, e axis
//Motors used should be 1 amp or lower current rating to avoid overheating TMC2130 drivers in Stealthchop.
//Kuo recommended 0.9 degree motors for X, Y, or direct drive E are Moons MS17HA2P4100 or OMC 17HM15-0904S 
//
#define X_AXIS_MOTOR_09 //kuo exper X axis
#define Y_AXIS_MOTOR_09 //kuo exper Y axis
//#define Z_AXIS_MOTOR_09 //kuo exper Z axis
//#define E_AXIS_MOTOR_09 //kuo exper EXTRUDER
```

My _AXIS_MOTOR_09 defines are probably all you need to modify. The rest of my firmware changes are controlled by these defines.

Uncomment only the axes that you want to be 0.9 degree motors and any other extruder options you are using. For example, if you have 0.9 degree motors only on X & Y, a BNBSX Extruder and Slice thermistor it would look like...
```
#define X_AXIS_MOTOR_09 //kuo exper X axis
#define Y_AXIS_MOTOR_09 //kuo exper Y axis
//#define Z_AXIS_MOTOR_09 //kuo exper Z axis
//#define E_AXIS_MOTOR_09 //kuo exper EXTRUDER

//====== Kuo Uncomment ONLY ONE or NONE of below for geared extruders
//Don't forget to also send gcode to set e-steps as detailed earlier
//Reversion back from geared extruder requires sending M92 E280 & M500 to printer
//
//#define SKELESTRUDER // Uncomment if you have a skelestruder. Applies the patches for load distances and Z height.
//#define BONDTECH_PRUSA_UPGRADE_MK3 //Kuo Uncomment for Bondtech MK3 extruder upgrade. 3:1 extruder. This also sets Z_MAX_POS 205.
//#define BONDTECH_PRUSA_UPGRADE_MK3S //Kuo Uncomment for Bondtech MK3S extruder upgrade. (Note the S!!!!) 3:1 extruder. This also sets Z_MAX_POS 205.
//#define EXTRUDER_GEARRATIO_30 //Kuo Uncomment for extruder with gear ratio 3.0. 
#define EXTRUDER_GEARRATIO_3375 //Kuo Uncomment for extruder with gear ratio 3.375 like 54:16 BNBSX.
//#define EXTRUDER_GEARRATIO_35 //Kuo Uncomment for extruder with gear ratio 3.5 like 56:16 Bunny and Bear Short Ears or Skelestruder.

//====== Kuo E3D Volcano Support
//#define E3D_VOLCANO //uncomment to adjust Z_MAX_POS to accomodate 8.5 mm greater Volcano extruder height
//====== Kuo Slice Support
#define SLICETHERMISTOR //uncomment for Slice Thermistor
//#define SLICEMAGNUM //uncomment to adjust MMU2S filament laod/unload distances for Slice Magnum

//====== Kuo End of defines one normally needs to change ======
```

Compile the firmware. 
Do a full factory reset with data erase. 
Don't let setup wizard run 1st time. You should set e-steps and microstepping first.

#Microstepping and e-steps
If you are using a geared extruder, you must also set e-steps and micro-stepping. Although my branch firmware includes settings for those items, they are not always accepted by the printer.

Use a terminal to make the setting and verify they have been accepted. For instance on a BNBSX extruder you would issue

M350 E16 //set extruder microstepping
M92 E473 //set e-steps
M500 //store the settings

M503 //read current settings so you can verify the extruder motor microsteps and e-steps are 16 and 473

NB: M350 must be BEFORE M92. Otherwise, M350 may alter existing the e-steps value

Once you have verified e-steps and microstepping are correct, you can proceed with setup wizard.

## Hardware
### Motors
Select either below listed Moons or OMC 0.9 degree steppers. Both dramatically reduce VFA's compared to any 1.8 degree motor. 

OMC motor is 1/2 cost of Moons but requires soldering of cable adapter. Moon's are plug-in compatible with Yotino cable harness.

Moon's are best at reducing VFA's with linearity correction OFF. They do not tune better with linearity correction. However, the Moon's have suffer a signficant rate of defective units shipped. After receiving a Moons' check it spins with very little notchinesss.

OMC's are baseline slightly worse than Moons, but can be tuned to achieve better than Moons with linearity correction (1.130 - 1.140). However, forget to set linearity correction and the OMCs are worse than Moons. 

NB: Prusa firmware older than 3.8.0 does not store linearity correction settings to EEPROM unless you let the menu time out by itself.

OMC's are a little bit louder during printing, but not by much. 

Both choices of motor require grinding a shaft flat for the y-axis.

It's a matter of cost, install effort, and whether one can be bothered to set linearity correction (once) for X and Y. Either motor choice greatly reduces VFA's.

[Moons Stepper Motors](https://www.amazon.com/MOONS-Stepper-Accuracy-Cable00723-MS17HA2P4100/dp/B072HT8PLM)	$53 x 2
Moons 0.9	MS17HA2P4100 
Voltage	3.9 volt
Resistance	3.9 ohm
Current	1.0 A
Hold Torque	0.39 Nm
Weight	290 gm
inductance	11.2 mH

or

[OMC Stepper Motors](https://www.amazon.com/gp/product/B00W98OYE4)	$20 x 2
STEPPERONLINE 0.9° OMC 17HM15-0904S
Voltage: 12-24V
Resistance	6.0ohms
Hold Torque 0.36 Nm
Current	0.9A 
Weight	280 gm
Inductance	12.0mH

[JST-PH Connector Kit](https://www.amazon.com/gp/product/B07DCL9J8K)	$15
2.0mm JST-PH Connector Kit, with JST-PH 5/6/7 Pin Housing 
6 pin connector to adapt OMC motor wires to harness.
Optionally, skip these connectors and directly solder motor wires to harness
I prefer to add standardized connector because I test multiple motors.



### Heatsinks for TMC2130's
Obtain heatsinks for TMZ2130's x all 4 axes. May as well cool Z and E while taking care of X & Y)
Required to avoid 0.9 motors overheating TMC2130 drivers, especially if run in Stealth mode. Heatsinks attach to NON-COMPONENT side of EINSY PC board. They do NOT attach on the TMC2130 chips themselves, but to thermal vias on empty side of EINSY board. Must also cut holes in back of EINSY case for fitment & adequate ventilation. Clean PC board with IPA to let self-stick thermal tape do its job. Position on the vias!.

[Driver Heatsinks](https://www.amazon.com/gp/product/B07GWR3TWZ)	$9
Driver Heatsinks for TMC2130 (12 pcs)
Use these or similar self-stick, driver heatsinks for stock EINSY enclosure. 

or

Reverse orientation EINSY case users must use low profile heatsinks because reverse case orients heatsinks towards heatbed. Extruder cable will catch on high profile heatsinks. Use low profile, Pi heatsinks.

[Raspberry Pi Heatsink Kit](https://www.amazon.com/gp/product/B07217N5LS)	$8
Raspberry Pi Heatsink Kit (lower profile than usual driver heatsinks)
I use these Pi heatsinks with my reverse EINSY case. Smaller and lower profile but still get the job done.

### Motor Cables
[Stepper Motor Cables](https://www.amazon.com/gp/product/B07CBV8DVZ)	$7
YOTINO Bipolar Stepper Motor Cables, 4 x 100cm Long NEMA 17 Extended Connector Cable (XH2.54 4Pin-6Pin)

Wiring is as in pictures. Pay attention to wire order and pin positions at both EINSY and motor ends
NB: if using retrograde motor extruder like BNBSX Short Ears, plug EINSY end in 180 degrees rotated
NB2: LDO's use different pinout not detailed here.

![EINSY end of cable](https://github.com/guykuo/Prusa-Firmware/blob/0.9-Degree-Stepper-Support/einsy-end-of-cable.jpg)

![Moons motor connector wire order detail](https://github.com/guykuo/Prusa-Firmware/blob/0.9-Degree-Stepper-Support/moons-end-of-cable.jpg)

![OMC cable connector](https://github.com/guykuo/Prusa-Firmware/blob/0.9-Degree-Stepper-Support/OMC%20stepper%20online%20wiring%20adapt.JPG)

If you want pre-assembled connector and wire assemblies, while not specifically intended for stepper motors, these RGB LED strip connectors are a great alternative.  They have color-coded wires that match the standard stepper motor wire colors, they are meant to handle a couple of amps or more.  These are great for steppers that have wires already attached.

[DIY Stepper Cables: RGB LED Strjp Cables](https://www.amazon.com/gp/product/B01DC0KKJU/) 	$9
BTF-LIGHTING 10 Pairs 4pin SM JST 15cm Cable Female/Male connectors for Led Strip RGB 5050 3528 WS2801 APA02


### Drive Pulleys (optional but recommended)
[10mm Drive Pulley for GT2](https://www.amazon.com/gp/product/B07BH26P2D) $8.90 for 4
BALITENSEN GT2 Timing Pulley 16 Teeth 5mm Bore, Width 10mm for GT2 Belt 
(optional) Replace X and Y motor pulleys with these to reduce 2mm, vertical GT2 tooth artifact. This particular drive pulley yielded lower tooth engagement 2mm artifact during 1st phase testing.

### E-Steps and microstepping
Geared extruders now set for lower microstepping to avoid overruning EINSY during fast retracts or MMU2S filament moves. 
Factory reset and delete all data after installing this firmware. Otherwise EEPROM settings override settings in this firmware.
After installing this firmware, send M350 and M92 commands to force correct micro-stepping and e-step rates. M350 should be done FIRST because M350 will sometimes modified prior M92 value.
```
e-steps values for M92 depend on your extruder gearing.
xxx = 280 for non-geared extruder
xxx = 415 for BMG extruder (special Bondtech compensated value)
xxx = 420 for 3:1 extruder
xxx = 473 for BNBSX with 54:16 gearing
xxx = 490 for BNBSX, Short Ears, Skelestruder with 56:16 gearing

non-geared extruder, 1.8 degree motor
M350 E32
M92 E280
M500

non-geared extruder, 0.9 degree motor
M350 E16
M92 E280
M500

geared extruder, 1.8 degree motor
M350 E16
M92 Exxx
M500

geared extruder, 0.9 degree motor
M350 E8
M92 Exxx
M500

Follow with power off/on and M503 to verify settings are correct.
```

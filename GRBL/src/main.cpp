/*
  main.cpp - Initialization and main loop for Grbl
  Part of Grbl
  Copyright (c) 2014-2016 Sungeun K. Jeon for Gnea Research LLC

	2018 -	Bart Dring This file was modifed for use on the ESP32
					CPU. Do not use this with Grbl for atMega328P
    2022 -  wangchong 
    
  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "main.h"
#include <WiFi.h>

#ifdef ENABLE_TFT
#include "UI/lv_port/lv_port_init.h"
#include "UI/tft_driver/tft_lcd.h"
#endif

File testlv_file;

void setup() {
    disableAllMessage();                        /* Delet all message    */ 
    disableDWT();                               /* disable all dwt      */ 
    grbl_init();                                /* enter grbl init      */ 
}

void loop() {
    while(1) { run_once(); }                    /* enter grbl task      */
}

void grbl_init() {

#ifdef USE_I2S_OUT
    i2s_out_init();                             /* The I2S out must be initialized before it can access the expanded GPIO port */
#endif

#ifdef ENABLE_TFT
    tft_lcd.tftBglightInit();
    tft_lcd.tftBglightSetOff();
#endif

#ifdef ENABLE_WIFI
    WebUI::wifi_config.init();                      /* init wifi state */
#endif
    sysInitFinish = false;

    client_init();                                  /* Setup serial baud rate and interrupts */
    report_machine_type(CLIENT_SERIAL);             /* show the map name at startup */
    settings_init();                                /* Load Grbl settings from non-volatile storage */
    stepper_init();                                 /* Configure stepper pins and interrupt timers */
    system_ini();                                   /* Configure pinout pins and pin-change interrupt (Renamed due to conflict with esp32 files) */
    init_motors();
    memset(sys_position, 0, sizeof(sys_position));  /* Clear machine position. */

#ifdef FORCE_INITIALIZATION_ALARM               /* Initialize system state. */
    sys.state = State::Alarm;                   /* Force Grbl into an ALARM state upon a power-cycle or hard reset. */
#else
    sys.state = State::Idle;                    /* Initialize system state. */
#endif
   
/*  Check for power-up and set system alarm if homing is enabled to force homing cycle
    by setting Grbl's alarm state. Alarm locks out all g-code commands, including the
    startup scripts, but allows access to settings and internal commands. Only a homing
    cycle '$H' or kill alarm locks '$X' will disable the alarm.
    NOTE: The startup script will run after successful completion of the homing cycle, but
    not after disabling the alarm locks. Prevents motion startup blocks from crashing into
    things uncontrollably. Very bad. */
#ifdef HOMING_INIT_LOCK
    if (homing_enable->get()) { sys.state = State::Alarm; }
#endif

    my_fs.begin();                                                      /* 开启内部文件系统, 挂载后无需取消挂载*/ 

#ifdef ENABLE_TFT
    ui.lvglTaskInit();                                                  /* Init LCD LVGL */
#endif

    // init spindle
    Spindles::Spindle::select();

#ifdef ENABLE_WIFI
    WebUI::wifi_config.begin();
#endif

#ifdef ENABLE_BLUETOOTH
    WebUI::bt_config.begin();
#endif
    WebUI::inputBuffer.begin();                                         /* init ringbuffer */ 

    sysInitFinish = true;
}

void reset_mc_config(void) {

    State prior_state = sys.state;
    memset(&sys, 0, sizeof(system_t));                                                  /* Clear system struct variable. */ 
    sys.state                            = prior_state;                                 /* defaule sys state */
    sys.f_override                       = FeedOverride::Default;                       /* Set to 100% */ 
    sys.r_override                       = RapidOverride::Default;                      /* Set to 100% */ 
    sys.spindle_speed_ovr                = SpindleSpeedOverride::Default;               /* Set to 100% */ 
    memset(sys_probe_position, 0, sizeof(sys_probe_position));                          /* Clear probe position. */ 

    sys_probe_state                      = Probe::Off;
    sys_rt_exec_state.value              = 0;
    sys_rt_exec_accessory_override.value = 0;
    sys_rt_exec_alarm                    = ExecAlarm::None;
    cycle_stop                           = false;
    sys_rt_f_override                    = FeedOverride::Default;
    sys_rt_r_override                    = RapidOverride::Default;
    sys_rt_s_override                    = SpindleSpeedOverride::Default;

    client_reset_read_buffer(CLIENT_ALL);
}

static void reset_variables() {
    reset_mc_config();                                                                  /* Reset Grbl primary systems. */
    gc_init();                                                                          /* Set g-code parser to default state */
    spindle->stop();                                                                    /* Stop spindle */
    coolant_init();                                                                     /* Init coolant */
    limits_init();                                                                      /* Init limit */
    probe_init();                                                                       /* Init probe */
    plan_reset();                                                                       /* Clear block buffer and planner variables */ 
    st_reset();                                                                         /* Clear stepper subsystem variables */ 
    plan_sync_position();                                                               /* Sync cleared gcode and planner positions to current system position. */
    gc_sync_position();
    report_init_message(CLIENT_ALL);

    // used to keep track of a jog command sent to mc_line() so we can cancel it.
    // this is needed if a jogCancel comes along after we have already parsed a jog and it is in-flight.
    sys_pl_data_inflight = NULL;
}

void run_once() {
    reset_variables();
    // Start Grbl main loop. Processes program inputs and executes them.
    // This can exit on a system abort condition, in which case run_once()
    // is re-executed by an enclosing loop.
    protocol_main_loop();
}

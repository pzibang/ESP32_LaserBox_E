/*
    Spindle.cpp

    A Base class for spindles and spinsle like things such as lasers

    Part of Grbl_ESP32

    2020 -  Bart Dring

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

    TODO
        Add Spindle spin up/down delays

    Get rid of dependance on machine definition #defines
        SPINDLE_OUTPUT_PIN
        SPINDLE_ENABLE_PIN
        SPINDLE_DIR_PIN

*/
#include "Spindle.h"

#include "PWMSpindle.h"
#include "Laser.h"

namespace Spindles {
    PWM      pwm;
    Laser    laser;

    void Spindle::select() {
        switch (static_cast<SpindleType>(spindle_type->get())) {
            case SpindleType::PWM:
                spindle = &pwm;
            break;
            case SpindleType::LASER:
                spindle = &laser;
            break;
        }
        spindle->init();
    }

    // ========================= Spindle ==================================

    bool Spindle::inLaserMode() {
        return false;  // default for basic spindle is false
    }

    void Spindle::sync(SpindleState state, uint32_t rpm) {
        if (sys.state == State::CheckMode) {
            return;
        }
        protocol_buffer_synchronize();  // Empty planner buffer to ensure spindle is set when programmed.
        set_state(state, rpm);
    }

    void Spindle::deinit() { stop(); }
}

 Spindles::Spindle* spindle;

/*
   Author: towedROV NTNU 2022

   This is a class to control a stepper.
   Documatation is in readME and comments.
   Also check the stepper_demo.ino file.
*/

#ifndef WingStepper_h
#define WingStepper_h
#include "arduino.h"


class WingStepper {

  private:
    int _step_pin;
    int _dir_pin;
    int _sensor_pin;

    unsigned long _timer1 = 0; // used for timing the steps        Note: might overflow after 70 minutes
    unsigned int _step_delay_us = 750; // time between half a step
    bool _ref_direction = true; // decides the standard direction of movement
    bool _prev_direction = false;

    int _stepper_pos = 0;  // position on stepper based on amount of steps
    bool _step_state = false; // stores the state of the step pin
    float _offset_pos = 0; // offset if calibration is off

    int _sensor_top_pos = -1; // initial value out of range
    int _sensor_bottom_pos = 1; // same here
    unsigned long _sensor_count = 0; // measurs the sensor multiple
    const unsigned long MAX_COUNT = 100000; // increase this if sensor gets activated without being near magnet

    // step = angle * k
    float _angle_to_step_const = 99.05055907 / 1.80;     // gear ratio /(degree/step) = k = 55.0281
    float _step_to_angle_const = 1.0 / _angle_to_step_const;



  public:

    // constructor
    WingStepper(int step_pin, int dir_pin, int sensor_pin) {
      _step_pin = step_pin;
      _dir_pin = dir_pin;
      _sensor_pin = sensor_pin;
    }

    // run in setup
    void begin() {
      pinMode(_step_pin, OUTPUT);
      pinMode(_dir_pin, OUTPUT);
      pinMode(_sensor_pin, INPUT);
      digitalWrite(_dir_pin, LOW);
    }


    int get_pos() {
      return _stepper_pos;
    }


    float get_angle() {
      return _stepper_pos * _step_to_angle_const;
    }


    void set_angle_to_step_const (float K) {
      _angle_to_step_const = K;
      _step_to_angle_const = 1 / K;
    }


    float get_angle_to_step_const() {
      return _angle_to_step_const;
    }


    // flip direction if stepper rotates the wrong way
    void flip_ref_direction() {
      _ref_direction = false;
    }


    // set new delay between steps
    void set_step_delay_us(int us) {
      _step_delay_us = us;
    }


    // set a offset angle if caliblation is off center
    void set_offset_angle(float offset_angle) {
      int offset_pos = offset_angle * _angle_to_step_const; // convert from angle to step

      if (_offset_pos == 0) {
        _offset_pos = offset_pos;
      }
      else {
        _stepper_pos = _stepper_pos + offset_pos;
      }
    }


    // rotates one step towards the desired position
    //  every other time it is run
    // @return true when finished
    bool rotate_step(int desired_pos) {
      
      // return if position is already correct
      if (_stepper_pos == desired_pos) {
        return true;
      }
      
      // chooses correct direction
      bool dir = _ref_direction;
      if (_stepper_pos > desired_pos) {
        dir = !_ref_direction;
      }

      // change direction on pin
      if (dir != _prev_direction) {
        delayMicroseconds(1);
        digitalWrite(_dir_pin, dir);
        _prev_direction = dir;
        delayMicroseconds(1); //driver need min 650ns of delay before step.
      }

      //change step pin after a delay
      if (micros() - _timer1 > _step_delay_us) {
        _step_state = !_step_state;
        digitalWrite(_step_pin, _step_state);
        _timer1 = micros();

        //updates position after new step
        if (_step_state) {
          if (dir == _ref_direction) {
            _stepper_pos ++;
          }
          else {
            _stepper_pos --;
          }
        }
      }

      //return true if position is correct
      if (_stepper_pos == desired_pos) {
        return true;
      }
      else {
        return false;
      }
    }


    // rotates one step towards the desired angle
    //  every other time it is run
    // @return true when finished
    bool rotate (float desired_angle) {
      int desired_pos = desired_angle * _angle_to_step_const;
      return rotate_step(desired_pos);
    }


    // @return true when finished
    bool calibrate() {
      bool finished = false;

      // rotate up to sensor
      if (_sensor_top_pos == -1) {
        bool noMagnet = rotate(360);

        if (noMagnet) { //stepper has rotated 360 deg without finding a magnet
          Serial.println("<STEPPER_ERROR: No magnet found. Calibration failed>");
          finished = true;
        }
        if (digitalRead(_sensor_pin)) {
          //sensor must be read many times to increase data confidence
          _sensor_count ++;
          if (_sensor_count > MAX_COUNT) {
            _sensor_top_pos = _stepper_pos;  // capture position of magnet
            _sensor_count = 0;
          }
        }
      }

      // rotate down to other sensor
      else if (_sensor_bottom_pos == 1) {
        rotate(-360);

        if (digitalRead(_sensor_pin)) {
          //sensor must be read many times to increase data confidence
          _sensor_count ++;
          if (_sensor_count > MAX_COUNT) {
            _sensor_count = 0;

            // only add new sensor if stepper has moved more than 25 deg
            float sensor_top_angle = _sensor_top_pos * _step_to_angle_const;
            if (sensor_top_angle - get_angle() > 25) {
              _sensor_bottom_pos = _stepper_pos;
            }
          }
        }
      }

      //calculate new stepper pos
      else {
        float gap_angle = (_sensor_top_pos - _sensor_bottom_pos) * _step_to_angle_const;
        Serial.print("gap_ angle: "); Serial.println(gap_angle);
        if (gap_angle < 180) {
          _stepper_pos = (_sensor_bottom_pos - _sensor_top_pos) / 2;
        }
        // stepper is on wrong side of sensor
        else {
          float real_gap = 360 - gap_angle;
          _stepper_pos = (real_gap * _angle_to_step_const) / 2;
        }
        finished = true;
      }

      if (finished) {
        //reset pos in case a new calibration is started
        _sensor_top_pos = -1;
        _sensor_bottom_pos = 1;
        _stepper_pos = _stepper_pos + _offset_pos;
        return true;
      }
      else {
        return false;
      }
    }


    void print_debug() {
      Serial.println("STEPPER DEBUG:");
      Serial.print("stepper_pos: "); Serial.println (_stepper_pos);
      Serial.print("sensor state: "); Serial.println (digitalRead(_sensor_pin));
      Serial.print("sensor top: "); Serial.println(_sensor_top_pos);
      Serial.print("sensor bottom pos: "); Serial.println(_sensor_bottom_pos);
      delay(10);
    }

};

#endif

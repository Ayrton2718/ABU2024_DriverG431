/**
 * @file Tri_Func_Filter.hpp
 * @brief 三角関数を用いた、ステップ入力をなめらかにするフィルタ
 * @version 0.1
 * @date 2024-03-23
 * 
 * 
 */

#pragma once

#include <math.h>

namespace tut_mec_filter{

class Cos_Filter{

private:
    double _reach_time;
    double _integral_rad;
    double _const_cycle_time;
    double _cycle_time;
    double _hold_data;
    double _target_data;
    double _bef_target_data;
    double _data_delta;
    double _addition_data;
    double _output;
    bool _is_init = false;


public:
    Cos_Filter(){ }

    void init(double REACH_TIME, double CONST_CYCLE_TIME){
        _reach_time = REACH_TIME;
        _const_cycle_time = CONST_CYCLE_TIME;
        _cycle_time = _const_cycle_time;
        _is_init = true;

        this->Reset();
    }

    void Set_Base(double BASE){
        this->Reset();
        _target_data = BASE;
        _bef_target_data = BASE;
        _hold_data = BASE;
        _output = BASE;
    }

    void Set_Reach_Time(double TIME){
        _reach_time = TIME;
    }

    void Set_Data(double TARGET){
        _target_data = TARGET;
    }

    void Update(double CYCLE_TIME = -1){
        if(!_is_init){
            return;
        }

        if(CYCLE_TIME < 0){
            _cycle_time = _const_cycle_time;
        }
        else{
            _cycle_time = CYCLE_TIME;
        }

        if(_bef_target_data != _target_data){
            _hold_data = _hold_data + _addition_data;
            _data_delta = _target_data - _hold_data;
            _integral_rad = 0;
            _bef_target_data = _target_data;
        }

        if(_integral_rad < M_PI){
            _addition_data = _data_delta * ((1-cos(_integral_rad)) / 2.0);
            _output = _hold_data + _addition_data;
            if(_reach_time <= 0){
                _integral_rad = M_PI;
            }
            else{
                _integral_rad += (M_PI / _reach_time) * _cycle_time;
            }
        }
        else{
            _addition_data = _data_delta;
            _output = _hold_data + _addition_data;
        }
    }

    double Output(){ return _output; }

    double Cal_Data(double TARGET, double CYCLE_TIME = -1){
        this->Set_Data(TARGET);
        this->Update(CYCLE_TIME);
        return this->Output();
    }

    void Reset(){
        _target_data = 0;
        _bef_target_data = 0;
        _hold_data = 0;
        _data_delta = 0;
        _addition_data = 0;
        _integral_rad = 0;
        _output = 0;
    }



};


}

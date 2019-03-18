#ifndef VRA_VOLTAGE_H
#define VRA_VOLTAGE_H

#include "mbed.h"
#include "ble/BLE.h"
#include "ble/Gap.h"
#include "BQ25896.h"
#include "MAX17055.h"
#include "BLEChar.h"

class VRAVoltage
{
public:
    enum Characteristics {
        TTE,
        TTF,
        CYCLES,
        CURRENT,
        AV_CURRENT,
        CAPACITY,
        FULL_CAPACITY,
        V_CELL,
        OTG,

        COUNT
    };

    VRAVoltage(Serial *_pc, PinName p_sda, PinName p_scl, PinName p_interrupt);

    GattCharacteristic** getCharacteristics(GattCharacteristic** c);
    uint8_t getCharacteristcsCount();
    uint8_t *values;

    BLEChar **characs;

    void init();

    uint8_t ChargeValue;

    BQ25896 *voltageCtl;
    MAX17055 *batteryChk;
    int get_charge();

    void get_tte(void);
    void get_ttf(void);
    void get_cycles(void);
    void get_current(void);
    void get_average_current(void);
    void get_capacity(void);
    void get_full_capacity(void);
    void get_v_cell(void);

    void get_otg(void);
    void set_otg(void);

    int readMax17055(MAX17055::reg_t reg);
    int max17055TomAh(int val);
    int max17055ToPerc(int val);
    int max17055ToMinutes(int val);
    int max17055ToCelsius(int val);
    int max17055ToOhm(int val);
    signed int max17055TouA(int val);
    int max17055TomV(int val);
    
    void voltageIntISR(void);

    void checkChargingState(void);
    // void *chargerDetectedCb(void);
    // void *chargerRemovedCb(void);

    uint8_t chargeState;
    volatile bool voltInt;
    Serial *pc;

    I2C *i2c;
    DigitalOut *select;
    InterruptIn *interruptPin;

    void init_voltageCtl();
    void init_batteryChk();
};

#endif
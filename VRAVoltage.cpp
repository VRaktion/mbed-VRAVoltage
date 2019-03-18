#include "VRAVoltage.h"

const uint16_t BATTERY_OTG_CHARACTERISTIC = 0xff0f;

VRAVoltage::VRAVoltage(Serial *_pc, PinName p_sda, PinName p_scl, PinName p_interrupt)
{
    this->chargeState = 0;
    this->pc = _pc;
    this->pc->printf("generate voltage module\n\r");
    i2c = new I2C(p_sda, p_scl);
    
    batteryChk = new MAX17055(*i2c);
    voltageCtl = new BQ25896(p_sda, p_scl);
    //voltageInt = new InterruptIn(p_interrupt);

    this->characs = (BLEChar**) malloc(COUNT * sizeof(BLEChar*));

    this->characs[TTE] = new BLEChar(
        0xff01,
        GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ,
        4);
    this->characs[TTE]->readCb->attach(this, &VRAVoltage::get_tte);

    this->characs[TTF] = new BLEChar(
        0xff02,
        GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ,
        4);
    this->characs[TTF]->readCb->attach(this, &VRAVoltage::get_ttf);

    this->characs[CYCLES] = new BLEChar(
        0xff03,
        GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ,
        4);
    this->characs[CYCLES]->readCb->attach(this, &VRAVoltage::get_cycles);

    this->characs[CURRENT] = new BLEChar(
        0xff04,
        GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ,
        4);
    this->characs[CURRENT]->readCb->attach(this, &VRAVoltage::get_current);

    this->characs[AV_CURRENT] = new BLEChar(
        0xff05,
        GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ,
        4);
    this->characs[AV_CURRENT]->readCb->attach(this, &VRAVoltage::get_average_current);

    this->characs[CAPACITY] = new BLEChar(
        0xff06,
        GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ,
        4);
    this->characs[CAPACITY]->readCb->attach(this, &VRAVoltage::get_capacity);

    this->characs[FULL_CAPACITY] = new BLEChar(
        0xff07,
        GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ,
        4);
    this->characs[FULL_CAPACITY]->readCb->attach(this, &VRAVoltage::get_full_capacity);

    this->characs[V_CELL] = new BLEChar(
        0xff08,
        GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ,
        4);
    this->characs[V_CELL]->readCb->attach(this, &VRAVoltage::get_v_cell);

    this->characs[OTG] = new BLEChar(
        0xff0f,
        GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE,
        1);
    this->characs[OTG]->readCb->attach(this, &VRAVoltage::get_otg);
    this->characs[OTG]->writeCb->attach(this, &VRAVoltage::set_otg);
}

GattCharacteristic** VRAVoltage::getCharacteristics(GattCharacteristic** c){
    for(uint8_t i=0; i < (uint8_t) COUNT; i++){
        *(c++) = this->characs[i]->charac;
    }
    return c;
}

uint8_t VRAVoltage::getCharacteristcsCount(){
    return (uint8_t) COUNT;
}

// volatile bool voltInt = false;
// void voltageIntISR() {
//     voltInt = true;
// }

void VRAVoltage::init(){
    init_voltageCtl();
    init_batteryChk();

    //voltageInt->fall(&voltageIntISR);
}

volatile bool voltInt = false;
void VRAVoltage::voltageIntISR(void) {
    voltInt = true;
}

void VRAVoltage::init_voltageCtl() {
    pc->printf("init voltageCtl\n\r");
    voltageCtl->reset();
    voltageCtl->disableWATCHDOG();//disable Watchdog
    voltageCtl->disableILIMPin();
    voltageCtl->setIINLIM(3250);//mA
    voltageCtl->enableOTG();
    voltageCtl->setBOOSTV(5000);//
    voltageCtl->setICHG(1280);//Fast Charge Current Limit 1280
    voltageCtl->setBOOST_LIM(6);//BOOST_LIM 2.15A
    //voltageCtl->enableFORCE_VINDPM();//absolute VINDPM
    //voltageCtl->setVINDPM(4000);
    //pc->printf("VINDPM: %d\n\r", voltageCtl->getVINDPM());

    this->characs[OTG]->value[0] = 0x01; //OTG enabled
}

void VRAVoltage::init_batteryChk(){
    pc->printf("init batteryChk\n\r");
    batteryChk->init(0);
    batteryChk->writeReg(MAX17055::DESIGN_CAP, (uint16_t) 0x0A28);//nominal capacity in mAh * 2
    batteryChk->writeReg(MAX17055::FULL_CAP_REP, (uint16_t) 0x0A28);
    batteryChk->writeReg(MAX17055::V_EMPTY, (uint16_t) 0xABE0);//empty voltage in mV * 16 (2,75V)
    batteryChk->writeReg(MAX17055::I_CHG_TERM, (uint16_t) 0x0668);//charge termination current in mA * 6,4
    //256mA default by BQ25896 REG05 ITERM
}

int VRAVoltage::get_charge(){
    return max17055ToPerc(readMax17055(MAX17055::REP_SOC));
}

void VRAVoltage::get_tte(){
    this->characs[TTE]->setIntVal(
        max17055ToMinutes(
            readMax17055(MAX17055::TTE)));
}

void VRAVoltage::get_ttf(){
    this->characs[TTF]->setIntVal(
        max17055ToMinutes(
            readMax17055(MAX17055::TTF)));
}

void VRAVoltage::get_cycles(){
    this->characs[CYCLES]->setIntVal(
        readMax17055(MAX17055::CYCLES));
}

void VRAVoltage::get_current(){
    this->characs[CURRENT]->setIntVal(
        max17055TouA(
            readMax17055(MAX17055::CURRENT)));
}

void VRAVoltage::get_average_current(){
    this->characs[AV_CURRENT]->setIntVal(
        max17055TouA(
            readMax17055(MAX17055::AVG_CURRENT)));
}

void VRAVoltage::get_capacity(){
    this->characs[CAPACITY]->setIntVal(
        max17055TomAh(
            readMax17055(MAX17055::REP_CAP)));
}

void VRAVoltage::get_full_capacity(){
    this->characs[FULL_CAPACITY]->setIntVal(
        max17055TomAh(
            readMax17055(MAX17055::FULL_CAP_REP)));
}

void VRAVoltage::get_v_cell(){
    this->characs[V_CELL]->setIntVal(
        max17055TomV(
            readMax17055(MAX17055::V_CELL)));
}

void VRAVoltage::get_otg(){
    //this->characs[OTG]->value[0] = 0x23;
}

void VRAVoltage::set_otg(){
    //this->pc->printf("OTG: %x\n\r", this->characs[OTG]->value[0]);
    if(this->characs[OTG]->value[0]){
        voltageCtl->enableOTG();
    }else{
        voltageCtl->disableOTG();
    }
}

void VRAVoltage::checkChargingState(void){
    voltageCtl->oneShotADC();
    if(chargeState == 0){
        if (voltageCtl->getVBUSV() >= 4.99){
            pc->printf("charger detected\n\r");
            chargeState = 1;
            //if(*chargerDetectedCb) (*chargerDetectedCb)();
            //voltageCtl->disableOTG();
        }
    } else if (chargeState == 1){
        if (voltageCtl->getVBUSV() < 4.99){
            pc->printf("charger removed\n\r");
            chargeState = 0;
            //if(chargerRemovedCb) (*chargerRemovedCb)();
            //init_voltageCtl();
            //wait(3);
            // setBlueLed(0x00);
            // setWhiteLed(0x00);
            //initLEDDriver();
        }
    }
}

int VRAVoltage::readMax17055(MAX17055::reg_t reg)
{
    char buf[2];

    if (batteryChk->readReg(reg, buf, 2)) {
        return 0;
    }

    return (int)((buf[1] << 8) | buf[0]);
}

int VRAVoltage::max17055TomAh(int val){
    return val/2;//for 10mOhm Rsense
}

int VRAVoltage::max17055ToPerc(int val){
    return val/256;
}

int VRAVoltage::max17055ToMinutes(int val){
    int sec = val * 5625;
    return sec / 60000;
}

int VRAVoltage::max17055ToCelsius(int val){
    return val/256;
}

int VRAVoltage::max17055ToOhm(int val){
    return val/4096;
}

signed int VRAVoltage::max17055TouA(int val){
    signed int uA = (signed int) ((int16_t) (val)) * 15625;//for 10mOhm Rsense
    return uA / 100;
}

int VRAVoltage::max17055TomV(int val){
    int uA = val * 125;
    return uA / 1600;
}

#include <LedFader.h>

// constructor
LedFader::LedFader(const byte pin,
                   const byte pwmChannel,
                   const byte minValue,
                   const byte maxValue,
                   const unsigned long msPerCycle,
                   const bool active,
                   const bool stopWhenOn) : pin_(pin), pwmChannel_(pwmChannel), minValue_(minValue), maxValue_(maxValue), msPerCycle_(msPerCycle) {
    startTime_ = 0;
    active_ = active;
    stopWhenOn_ = stopWhenOn;
    forwards_ = true;
}  // end of  LedFader::LedFader

void LedFader::begin() {
    ledcAttachPin(pin_, pwmChannel_);  // assign  led pins to channels
                                       // Initialize channels
    // channels 0-15, resolution 1-16 bits, freq limits depend on resolution
    // ledcSetup(uint8_t channel, uint32_t freq, uint8_t resolution_bits);
    ledcSetup(pwmChannel_, 12000, 8);  // 12 kHz PWM, 8-bit resolution
    ledcWrite(pwmChannel_, 255);       // test high output of all leds in sequence
    delay(300);
    ledcWrite(pwmChannel_, 0);

    startTime_ = millis();
}  // end of LedFader::begin


void LedFader::fullOn() {
    ledcWrite(pwmChannel_, 255);  // full led illumination
}  // end of LedFader::begin

void LedFader::fullOff() {
    ledcWrite(pwmChannel_, 0);  // 0 led illumination
    active_ = false;

}  // end of LedFader::begin

// call from loop to set the LED brightness
void LedFader::update() {
    // do nothing if not active
    if (!active_)
        return;

    unsigned long now = millis();
    unsigned long howFarDone = now - startTime_;
    if (howFarDone >= msPerCycle_) {
        forwards_ = !forwards_;  // change direction
        if (forwards_)
            //analogWrite (pin_, minValue_);
            ledcWrite(pwmChannel_, minValue_);

        else
            //analogWrite (pin_, maxValue_);
            ledcWrite(pwmChannel_, maxValue_);
        startTime_ = now;

        // stop when at required brightness?
        if (stopWhenOn_)
            active_ = false;
    }  // end of overshot time for this cycle
    else {
        unsigned long newValue;
        byte valDifference = maxValue_ - minValue_;

        if (forwards_)
            newValue = (howFarDone * valDifference) / msPerCycle_;
        else
            newValue = ((msPerCycle_ - howFarDone) * valDifference) / msPerCycle_;

        //analogWrite (pin_, newValue + minValue_);
        ledcWrite(pwmChannel_, newValue + minValue_);
    }  // end of still in same cycle

}  // end of LedFader::update

// activate this LED
void LedFader::on() {
    active_ = true;
    startTime_ = millis();
    // forwards_ = true;
}  // end of LedFader::on

// deactivate this LED
void LedFader::off() {
    active_ = false;
    // digitalWrite(pin_, LOW);
}  // end of LedFader::off

// call from loop to flash the LED
// void setMsPerCycle();
void LedFader::setMsPerCycle(unsigned long msPerCycle) {
    msPerCycle_ = msPerCycle;

}  // end of LedFader::update

// is it active?
bool LedFader::isOn() const {
    return active_;
}  // end of LedFader::isOn

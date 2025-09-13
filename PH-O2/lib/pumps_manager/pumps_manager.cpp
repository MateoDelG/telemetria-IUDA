#include "pumps_manager.h"

PumpsManager::PumpsManager() {}

bool PumpsManager::begin(uint8_t pinKCL,
                         uint8_t pinH2O,
                         uint8_t pinSample,
                         uint8_t pinDrain,
                         uint8_t pinMixer,
                         bool activeHigh,
                         bool mixerInverted) {
  ch_[static_cast<uint8_t>(PumpId::KCL   )].pin = pinKCL;
  ch_[static_cast<uint8_t>(PumpId::H2O   )].pin = pinH2O;
  ch_[static_cast<uint8_t>(PumpId::SAMPLE)].pin = pinSample;
  ch_[static_cast<uint8_t>(PumpId::DRAIN )].pin = pinDrain;
  ch_[static_cast<uint8_t>(PumpId::MIXER )].pin = pinMixer;

  activeHigh_    = activeHigh;
  mixerInverted_ = mixerInverted;

  // Configura pines y asegura OFF seguro según lógica efectiva por canal
  for (uint8_t i = 0; i < static_cast<uint8_t>(PumpId::COUNT); ++i) {
    uint8_t pin = ch_[i].pin;
    if (pin == 0xFF) continue;
    pinMode(pin, OUTPUT);
    ch_[i].state = false; // OFF lógico
    bool ah = activeHighFor_(static_cast<PumpId>(i));
    // OFF físico
    digitalWrite(pin, ah ? LOW : HIGH);
  }

  inited_ = true;
  return true;
}

bool PumpsManager::activeHighFor_(PumpId id) const {
  if (id == PumpId::MIXER) {
    // Mixer invierte la lógica si mixerInverted_ == true
    return mixerInverted_ ? !activeHigh_ : activeHigh_;
  }
  return activeHigh_; // bombas usan lógica global
}

void PumpsManager::apply_(PumpId id) {
  if (!inited_) return;
  Chan& c = ch_[static_cast<uint8_t>(id)];
  if (c.pin == 0xFF) return;

  bool ah = activeHighFor_(id);
  // ON lógico -> HIGH si ah, LOW si !ah
  bool physHigh = (c.state == true) ? ah : !ah;
  digitalWrite(c.pin, physHigh ? HIGH : LOW);
}

void PumpsManager::on(PumpId id)  {
  ch_[static_cast<uint8_t>(id)].state = true;
  apply_(id);
}

void PumpsManager::off(PumpId id) {
  ch_[static_cast<uint8_t>(id)].state = false;
  apply_(id);
}

void PumpsManager::set(PumpId id, bool enable) {
  ch_[static_cast<uint8_t>(id)].state = enable;
  apply_(id);
}

bool PumpsManager::isOn(PumpId id) const {
  return ch_[static_cast<uint8_t>(id)].state;
}

void PumpsManager::allOff() {
  for (uint8_t i = 0; i < static_cast<uint8_t>(PumpId::COUNT); ++i) {
    ch_[i].state = false;
    uint8_t pin = ch_[i].pin;
    if (pin == 0xFF) continue;
    bool ah = activeHighFor_(static_cast<PumpId>(i));
    // OFF físico
    digitalWrite(pin, ah ? LOW : HIGH);
  }
}

void PumpsManager::setActiveHigh(bool activeHigh) {
  activeHigh_ = activeHigh;
  // Reaplica estados actuales a todos los canales (el mixer respeta mixerInverted_)
  for (uint8_t i = 0; i < static_cast<uint8_t>(PumpId::COUNT); ++i) {
    apply_(static_cast<PumpId>(i));
  }
}

void PumpsManager::setMixerInverted(bool inverted) {
  mixerInverted_ = inverted;
  // Reaplica estados actuales a todos por simplicidad (o solo al Mixer)
  for (uint8_t i = 0; i < static_cast<uint8_t>(PumpId::COUNT); ++i) {
    apply_(static_cast<PumpId>(i));
  }
}

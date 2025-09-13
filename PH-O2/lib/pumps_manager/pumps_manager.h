#pragma once
#include <Arduino.h>

// Identificadores de actuadores (conserva MIXER)
enum class PumpId : uint8_t {
  KCL = 0,
  H2O,
  SAMPLE,
  DRAIN,
  MIXER,
  COUNT
};

class PumpsManager {
public:
  PumpsManager();

  // Bombas/Mixer
  bool begin(uint8_t pinKCL,
             uint8_t pinH2O,
             uint8_t pinSample,
             uint8_t pinDrain,
             uint8_t pinMixer,
             bool activeHigh = true,
             bool mixerInverted = true);

  void on (PumpId id);
  void off(PumpId id);
  void set(PumpId id, bool enable);
  bool isOn(PumpId id) const;

  // Helpers
  inline void kclOn()    { on (PumpId::KCL);    }
  inline void kclOff()   { off(PumpId::KCL);    }
  inline void h2oOn()    { on (PumpId::H2O);    }
  inline void h2oOff()   { off(PumpId::H2O);    }
  inline void sampleOn() { on (PumpId::SAMPLE); }
  inline void sampleOff(){ off(PumpId::SAMPLE); }
  inline void drainOn()  { on (PumpId::DRAIN);  }
  inline void drainOff() { off(PumpId::DRAIN);  }
  inline void mixerOn()  { on (PumpId::MIXER);  }
  inline void mixerOff() { off(PumpId::MIXER);  }

  void allOff();
  void setActiveHigh(bool activeHigh);
  void setMixerInverted(bool inverted);

  // -----------------------------
  // Sensores de nivel (H2O / KCL)
  // -----------------------------
  // Nota: en ESP32 los pines 32-39 no tienen pull-up interno. Usa INPUT si no tienes resistencia externa.
  void beginLevels(uint8_t pinLevelH2O, uint8_t pinLevelKCL, bool usePullup = true);

  // Lecturas crudas (true = HIGH, false = LOW)
  bool levelH2O() const;  // digitalRead(pinH2O) == HIGH
  bool levelKCL() const;  // digitalRead(pinKCL) == HIGH

private:
  struct Chan {
    uint8_t pin   = 0xFF;
    bool    state = false;   // true=ON lógico
  };

  // Bombas/Mixer
  Chan  ch_[static_cast<uint8_t>(PumpId::COUNT)];
  bool  activeHigh_     = true;  // lógica bombas
  bool  mixerInverted_  = true;  // mixer usa !activeHigh_ si true
  bool  inited_         = false;

  // Sensores de nivel
  uint8_t levelPinH2O_  = 0xFF;
  uint8_t levelPinKCL_  = 0xFF;
  bool    levelsInited_ = false;
  bool    levelsUsePullup_ = true;

  bool activeHighFor_(PumpId id) const;
  void apply_(PumpId id);
};

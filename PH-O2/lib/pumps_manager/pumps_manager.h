#pragma once
#include <Arduino.h>

// Identificadores de los actuadores (evitar choque con macro MIXER)
enum class PumpId : uint8_t {
  KCL = 0,
  H2O,
  SAMPLE,
  DRAIN,
  MIXER,   // <- evita conflicto con #define MIXER
  COUNT
};

class PumpsManager {
public:
  PumpsManager();

  // 'activeHigh' define la lógica de las bombas (KCL/H2O/SAMPLE/DRAIN).
  // 'mixerInverted' (por defecto true) hace que el Mixer use lógica inversa a 'activeHigh'.
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

  // Helpers por comodidad
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

  // Cambia el nivel activo global de bombas y reaplica estados
  void setActiveHigh(bool activeHigh);

  // Configura si el Mixer está invertido respecto a 'activeHigh' y reaplica
  void setMixerInverted(bool inverted);

private:
  struct Chan {
    uint8_t pin   = 0xFF;
    bool    state = false;   // true=ON lógico
  };

  Chan  ch_[static_cast<uint8_t>(PumpId::COUNT)];
  bool  activeHigh_     = true;  // lógica de bombas
  bool  mixerInverted_  = true;  // mixer usa !activeHigh_ si true
  bool  inited_         = false;

  // Devuelve la lógica activa efectiva para un canal (considera inversión del mixer)
  bool activeHighFor_(PumpId id) const;

  void apply_(PumpId id);
};

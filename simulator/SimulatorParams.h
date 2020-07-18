#pragma once

namespace simulator {

// SimulatorParams contains all parameters for Simulator
struct SimulatorParams {
  float r;
  float m;
  float kLen;
  float kBend;
  float kGlobal;
  float kContact;
  float kDt;
  float kDn;
  float aSmall;
  float aLarge;
  float h;
  float cInit;

  static SimulatorParams Default() {
    return SimulatorParams
    {
      0.125f,
      0.006f,
      2000.f,
      5.f,
      1.5f,
      3250.f,
      0.003f,
      0.03f,
      0.3f,
      0.3f,
      1.f / 11800,
      0.935f
    };
  }
};

}; // namespace simulator

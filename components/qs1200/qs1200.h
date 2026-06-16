#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#ifdef USE_BUTTON
#include "esphome/components/button/button.h"
#endif

// FreeRTOS spinlock for ISR ↔ loop synchronisation (ESP32 only).
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>

namespace esphome {
namespace qs1200 {

// LED bit masks in the lower 16 bits of the decoded 32-bit frame.
// Verified against jingsno/intex-swg-pcb protocol documentation.
static const uint32_t LED_SERVICE       = (1UL << 0);   // 0x0001
static const uint32_t LED_HIGH_SALT     = (1UL << 1);   // 0x0002
static const uint32_t LED_LOW_SALT      = (1UL << 5);   // 0x0020
static const uint32_t LED_PUMP_LOW_FLOW = (1UL << 6);   // 0x0040
static const uint32_t LED_OZONE        = (1UL << 10);  // 0x0400
static const uint32_t LED_WORKING      = (1UL << 11);  // 0x0800
static const uint32_t LED_SLEEP        = (1UL << 12);  // 0x1000
static const uint32_t LED_BOOST        = (1UL << 15);  // 0x8000

// Button byte values (first byte; second byte is inverted complement).
static const uint8_t BUTTON_RELEASE    = 0x00;
static const uint8_t BUTTON_LOCK       = 0x01;
static const uint8_t BUTTON_TIMER      = 0x02;
static const uint8_t BUTTON_POWER      = 0x04;
static const uint8_t BUTTON_SELF_CLEAN = 0x08;
static const uint8_t BUTTON_BOOST      = 0x10;

// Pulse timing (µs).  Frame: LOW-200 start, then per bit: HIGH-(800|200) + LOW-200 spacer.
// Delta between consecutive falling edges: (HIGH + 200) µs.
// Bit-1 total: 800+200=1000 µs  →  window [750, 1350)
// Bit-0 total: 200+200= 400 µs  →  window [100,  650)
// Gap between windows avoids any overlap.
static const uint32_t BIT1_MIN = 750;
static const uint32_t BIT1_MAX = 1350;
static const uint32_t BIT0_MIN = 100;
static const uint32_t BIT0_MAX = 650;

// Debounce delay after a button press before sending RELEASE (ms).
static const uint32_t BUTTON_DEBOUNCE_MS = 50;

// 7-segment character codes (.gfedcba bit order).
static const uint8_t SEG_BLANK    = 0b00000000;
static const uint8_t SEG_0        = 0b00111111;
static const uint8_t SEG_1        = 0b00000110;
static const uint8_t SEG_2        = 0b01011011;
static const uint8_t SEG_3        = 0b01001111;
static const uint8_t SEG_4        = 0b01100110;
static const uint8_t SEG_5        = 0b01101101;
static const uint8_t SEG_6        = 0b01111101;
static const uint8_t SEG_7        = 0b00000111;
static const uint8_t SEG_8        = 0b01111111;
static const uint8_t SEG_9        = 0b01101111;
// Special self-clean display codes (with decimal-point bit set in the OTHER digit).
static const uint8_t SEG_CLEAN_6  = 0b01111101;  // matches DISP_1_CLEAN_06P in ref
static const uint8_t SEG_CLEAN_10 = 0b00111111;  // matches DISP_1_CLEAN_10P
static const uint8_t SEG_CLEAN_14 = 0b01100110;  // matches DISP_1_CLEAN_14P

class QS1200Component : public Component {
 public:
  // ── Pin setters (called from generated code) ──────────────────────────────
  void set_from_main_pin(InternalGPIOPin *pin) { from_main_pin_ = pin; }
  void set_to_disp_pin(InternalGPIOPin *pin)   { to_disp_pin_   = pin; }
  void set_from_disp_pin(InternalGPIOPin *pin) { from_disp_pin_ = pin; }
  void set_to_main_pin(InternalGPIOPin *pin)   { to_main_pin_   = pin; }

  // ── Sensor setters ────────────────────────────────────────────────────────
  void set_display_code_sensor(text_sensor::TextSensor *s)    { display_code_sensor_ = s; }
  void set_running_sensor(binary_sensor::BinarySensor *s)     { running_sensor_      = s; }
  void set_boost_sensor(binary_sensor::BinarySensor *s)       { boost_sensor_        = s; }
  void set_sleep_sensor(binary_sensor::BinarySensor *s)       { sleep_sensor_        = s; }
  void set_low_flow_sensor(binary_sensor::BinarySensor *s)    { low_flow_sensor_     = s; }
  void set_low_salt_sensor(binary_sensor::BinarySensor *s)    { low_salt_sensor_     = s; }
  void set_high_salt_sensor(binary_sensor::BinarySensor *s)   { high_salt_sensor_    = s; }
  void set_service_sensor(binary_sensor::BinarySensor *s)     { service_sensor_      = s; }

  // ── ESPHome lifecycle ─────────────────────────────────────────────────────
  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::IO; }
  void dump_config() override;

  // ── Button injection (fase 2) ─────────────────────────────────────────────
  void press_power();
  void press_timer();
  void press_boost();
  void press_self_clean();
  void press_lock();

  // ── ISR entry points (must be public for static dispatch) ─────────────────
  static void IRAM_ATTR isr_from_main(QS1200Component *arg);
  static void IRAM_ATTR isr_from_disp(QS1200Component *arg);

 protected:
  // Pins
  InternalGPIOPin *from_main_pin_{nullptr};
  InternalGPIOPin *to_disp_pin_{nullptr};
  InternalGPIOPin *from_disp_pin_{nullptr};
  InternalGPIOPin *to_main_pin_{nullptr};

  // ISR state — mainboard → display path (DIO decode)
  volatile uint32_t raw_frame_{0};
  volatile int      bit_index_{0};
  volatile uint32_t fall_time_us_{0};
  volatile uint32_t decoded_frame_{0};
  volatile bool     new_frame_{false};

  // Button injection — flag set in loop(), ISR honours it
  volatile bool     injecting_{false};

  portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;

  // Sensors
  text_sensor::TextSensor  *display_code_sensor_{nullptr};
  binary_sensor::BinarySensor *running_sensor_{nullptr};
  binary_sensor::BinarySensor *boost_sensor_{nullptr};
  binary_sensor::BinarySensor *sleep_sensor_{nullptr};
  binary_sensor::BinarySensor *low_flow_sensor_{nullptr};
  binary_sensor::BinarySensor *low_salt_sensor_{nullptr};
  binary_sensor::BinarySensor *high_salt_sensor_{nullptr};
  binary_sensor::BinarySensor *service_sensor_{nullptr};

  // Internal helpers
  void process_frame_(uint32_t frame);
  static char seg_to_char_(uint8_t seg);
  void send_button_(uint8_t button_code);
};

// ─────────────────────────────────────────────────────────────────────────────
// Fase 2 — button subklassen (alleen gecompileerd als USE_BUTTON actief is)
// ─────────────────────────────────────────────────────────────────────────────

#ifdef USE_BUTTON
class QS1200PowerButton : public button::Button {
 public:
  void set_parent(QS1200Component *p) { parent_ = p; }
 protected:
  void press_action() override { parent_->press_power(); }
  QS1200Component *parent_{nullptr};
};

class QS1200TimerButton : public button::Button {
 public:
  void set_parent(QS1200Component *p) { parent_ = p; }
 protected:
  void press_action() override { parent_->press_timer(); }
  QS1200Component *parent_{nullptr};
};

class QS1200BoostButton : public button::Button {
 public:
  void set_parent(QS1200Component *p) { parent_ = p; }
 protected:
  void press_action() override { parent_->press_boost(); }
  QS1200Component *parent_{nullptr};
};

class QS1200SelfCleanButton : public button::Button {
 public:
  void set_parent(QS1200Component *p) { parent_ = p; }
 protected:
  void press_action() override { parent_->press_self_clean(); }
  QS1200Component *parent_{nullptr};
};

class QS1200LockButton : public button::Button {
 public:
  void set_parent(QS1200Component *p) { parent_ = p; }
 protected:
  void press_action() override { parent_->press_lock(); }
  QS1200Component *parent_{nullptr};
};
#endif  // USE_BUTTON

}  // namespace qs1200
}  // namespace esphome

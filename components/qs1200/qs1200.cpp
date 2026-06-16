#include "qs1200.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
// delayMicroseconds() / delay() / micros() available via esphome/core/hal.h
// (already pulled in transitively through qs1200.h → esphome/core/hal.h).

namespace esphome {
namespace qs1200 {

static const char *const TAG = "qs1200";

// ─────────────────────────────────────────────────────────────────────────────
// Setup
// ─────────────────────────────────────────────────────────────────────────────

void QS1200Component::setup() {
  from_main_pin_->setup();
  from_disp_pin_->setup();
  to_disp_pin_->setup();
  to_main_pin_->setup();

  // Both output lines idle HIGH — this is the fail-safe pass-through state.
  to_disp_pin_->digital_write(true);
  to_main_pin_->digital_write(true);

  from_main_pin_->attach_interrupt(QS1200Component::isr_from_main, this, gpio::INTERRUPT_ANY_EDGE);
  from_disp_pin_->attach_interrupt(QS1200Component::isr_from_disp, this, gpio::INTERRUPT_ANY_EDGE);

  ESP_LOGD(TAG, "QS1200 setup complete (FROM_MAIN=%d TO_DISP=%d FROM_DISP=%d TO_MAIN=%d)",
           from_main_pin_->get_pin(), to_disp_pin_->get_pin(),
           from_disp_pin_->get_pin(), to_main_pin_->get_pin());
}

void QS1200Component::dump_config() {
  ESP_LOGCONFIG(TAG, "QS1200 SWG component");
  LOG_PIN("  FROM_MAIN: ", from_main_pin_);
  LOG_PIN("  TO_DISP:   ", to_disp_pin_);
  LOG_PIN("  FROM_DISP: ", from_disp_pin_);
  LOG_PIN("  TO_MAIN:   ", to_main_pin_);
}

// ─────────────────────────────────────────────────────────────────────────────
// Main loop — consume decoded frames from ISR
// ─────────────────────────────────────────────────────────────────────────────

void QS1200Component::loop() {
  if (!new_frame_)
    return;

  uint32_t frame;
  portENTER_CRITICAL(&mux_);
  frame = decoded_frame_;
  new_frame_ = false;
  portEXIT_CRITICAL(&mux_);

  process_frame_(frame);
}

// ─────────────────────────────────────────────────────────────────────────────
// ISR — mainboard → display (DIO, 32-bit frame decode + immediate pass-through)
// ─────────────────────────────────────────────────────────────────────────────

void IRAM_ATTR QS1200Component::isr_from_main(QS1200Component *arg) {
  uint32_t now = micros();

  if (!arg->from_main_pin_->digital_read()) {
    // ── Falling edge ──────────────────────────────────────────────────────
    arg->to_disp_pin_->digital_write(false);  // pass-through immediately

    uint32_t delta = now - arg->fall_time_us_;
    arg->fall_time_us_ = micros();  // timestamp after the write

    if (delta >= BIT1_MIN && delta < BIT1_MAX) {
      arg->raw_frame_ = (arg->raw_frame_ << 1) | 1u;
      arg->bit_index_++;
    } else if (delta >= BIT0_MIN && delta < BIT0_MAX) {
      arg->raw_frame_ = (arg->raw_frame_ << 1);
      arg->bit_index_++;
    } else {
      // Out-of-range delta → treat as frame start/sync, reset accumulator.
      arg->bit_index_ = 0;
      arg->raw_frame_ = 0;
      return;
    }

    if (arg->bit_index_ >= 32) {
      portENTER_CRITICAL_ISR(&arg->mux_);
      arg->decoded_frame_ = arg->raw_frame_;
      arg->new_frame_ = true;
      portEXIT_CRITICAL_ISR(&arg->mux_);
      arg->bit_index_ = 0;
      arg->raw_frame_ = 0;
    }
  } else {
    // ── Rising edge — pass-through only ───────────────────────────────────
    arg->to_disp_pin_->digital_write(true);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// ISR — display board → mainboard (CLK/button line, pure pass-through)
// Suppressed while loop() is injecting a synthetic button press.
// ─────────────────────────────────────────────────────────────────────────────

void IRAM_ATTR QS1200Component::isr_from_disp(QS1200Component *arg) {
  if (arg->injecting_)
    return;

  arg->to_main_pin_->digital_write(!arg->from_disp_pin_->digital_read());
}

// ─────────────────────────────────────────────────────────────────────────────
// Frame processing (runs in main loop task)
// ─────────────────────────────────────────────────────────────────────────────

void QS1200Component::process_frame_(uint32_t frame) {
  // Bits 31-24: left 7-seg digit  (bit 31 = decimal point)
  // Bits 23-16: right 7-seg digit (bit 23 = decimal point)
  // Bits 15- 0: LED status flags
  uint8_t left_seg  = (frame >> 24) & 0x7F;
  uint8_t right_seg = (frame >> 16) & 0x7F;
  char left_ch  = seg_to_char_(left_seg);
  char right_ch = seg_to_char_(right_seg);

  if (display_code_sensor_ != nullptr) {
    char buf[3] = {left_ch, right_ch, '\0'};
    if (display_code_sensor_->state != std::string(buf))
      display_code_sensor_->publish_state(buf);
  }

  auto pub = [](binary_sensor::BinarySensor *s, bool v) {
    if (s != nullptr && s->state != v)
      s->publish_state(v);
  };

  pub(running_sensor_,   (frame & LED_WORKING)       != 0);
  pub(boost_sensor_,     (frame & LED_BOOST)          != 0);
  pub(sleep_sensor_,     (frame & LED_SLEEP)          != 0);
  pub(low_flow_sensor_,  (frame & LED_PUMP_LOW_FLOW)  != 0);
  pub(low_salt_sensor_,  (frame & LED_LOW_SALT)       != 0);
  pub(high_salt_sensor_, (frame & LED_HIGH_SALT)      != 0);
  pub(service_sensor_,   (frame & LED_SERVICE)        != 0);

  ESP_LOGV(TAG, "frame=0x%08X disp='%c%c' W=%d B=%d Sl=%d LF=%d LS=%d HS=%d SVC=%d",
           frame, left_ch, right_ch,
           !!(frame & LED_WORKING), !!(frame & LED_BOOST), !!(frame & LED_SLEEP),
           !!(frame & LED_PUMP_LOW_FLOW), !!(frame & LED_LOW_SALT),
           !!(frame & LED_HIGH_SALT), !!(frame & LED_SERVICE));
}

// ─────────────────────────────────────────────────────────────────────────────
// 7-segment lookup
// ─────────────────────────────────────────────────────────────────────────────

char QS1200Component::seg_to_char_(uint8_t seg) {
  switch (seg & 0x7F) {
    case SEG_0:    return '0';
    case SEG_1:    return '1';
    case SEG_2:    return '2';
    case SEG_3:    return '3';
    case SEG_4:    return '4';
    case SEG_5:    return '5';
    case SEG_6:    return '6';
    case SEG_7:    return '7';
    case SEG_8:    return '8';
    case SEG_9:    return '9';
    case SEG_BLANK: return ' ';
    default:       return '?';
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Button injection (fase 2)
// Transmits a 16-bit PWM frame on TO_MAIN, followed automatically by RELEASE.
// Blocks the main loop for ~80 ms — acceptable for a one-shot button event.
// ─────────────────────────────────────────────────────────────────────────────

void QS1200Component::send_button_(uint8_t button_code) {
  injecting_ = true;

  // Frame: [inverted_byte | button_byte]
  uint16_t word = ((uint16_t)(button_code ^ 0xFF) << 8) | button_code;

  // Start pulse: LOW 200 µs
  to_main_pin_->digital_write(false);
  delayMicroseconds(200);

  // 16 data bits, MSB first: HIGH-(800|200) µs + LOW-200 µs spacer
  for (int i = 15; i >= 0; i--) {
    to_main_pin_->digital_write(true);
    delayMicroseconds((word >> i) & 1u ? 800 : 200);
    to_main_pin_->digital_write(false);
    delayMicroseconds(200);
  }

  to_main_pin_->digital_write(true);

  if (button_code != BUTTON_RELEASE) {
    delay(BUTTON_DEBOUNCE_MS);
    send_button_(BUTTON_RELEASE);
  }

  injecting_ = false;
}

void QS1200Component::press_power()      { send_button_(BUTTON_POWER);      }
void QS1200Component::press_timer()      { send_button_(BUTTON_TIMER);      }
void QS1200Component::press_boost()      { send_button_(BUTTON_BOOST_CMD);  }
void QS1200Component::press_self_clean() { send_button_(BUTTON_SELF_CLEAN); }
void QS1200Component::press_lock()       { send_button_(BUTTON_LOCK);       }

}  // namespace qs1200
}  // namespace esphome

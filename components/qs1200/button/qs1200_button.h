#pragma once

#include "esphome/components/button/button.h"
#include "../qs1200.h"

namespace esphome {
namespace qs1200 {

// One small subclass per physical key; each calls the matching press_*() on
// the parent component.  The parent drives the actual serial injection.

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

}  // namespace qs1200
}  // namespace esphome

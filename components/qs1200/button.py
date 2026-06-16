"""Fase 2 — button injection naar het hoofdbord."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from esphome.const import CONF_ID
from . import qs1200_ns, QS1200Component, CONF_QS1200_ID

DEPENDENCIES = ["qs1200"]

CONF_POWER      = "power"
CONF_TIMER      = "timer"
CONF_BOOST      = "boost"
CONF_SELF_CLEAN = "self_clean"
CONF_LOCK       = "lock"

# One Button subclass per physical key so each maps to its own press_*() call.
QS1200PowerButton     = qs1200_ns.class_("QS1200PowerButton",     button.Button)
QS1200TimerButton     = qs1200_ns.class_("QS1200TimerButton",     button.Button)
QS1200BoostButton     = qs1200_ns.class_("QS1200BoostButton",     button.Button)
QS1200SelfCleanButton = qs1200_ns.class_("QS1200SelfCleanButton", button.Button)
QS1200LockButton      = qs1200_ns.class_("QS1200LockButton",      button.Button)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_QS1200_ID): cv.use_id(QS1200Component),
        cv.Optional(CONF_POWER):      button.button_schema(QS1200PowerButton),
        cv.Optional(CONF_TIMER):      button.button_schema(QS1200TimerButton),
        cv.Optional(CONF_BOOST):      button.button_schema(QS1200BoostButton),
        cv.Optional(CONF_SELF_CLEAN): button.button_schema(QS1200SelfCleanButton),
        cv.Optional(CONF_LOCK):       button.button_schema(QS1200LockButton),
    }
)

_BUTTON_MAP = [
    (CONF_POWER,      QS1200PowerButton,     "set_parent"),
    (CONF_TIMER,      QS1200TimerButton,     "set_parent"),
    (CONF_BOOST,      QS1200BoostButton,     "set_parent"),
    (CONF_SELF_CLEAN, QS1200SelfCleanButton, "set_parent"),
    (CONF_LOCK,       QS1200LockButton,      "set_parent"),
]


async def to_code(config):
    hub = await cg.get_variable(config[CONF_QS1200_ID])

    for key, cls, _ in _BUTTON_MAP:
        if key in config:
            btn = await button.new_button(config[key])
            cg.add(btn.set_parent(hub))

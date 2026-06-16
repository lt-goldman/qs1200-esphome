import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from . import qs1200_ns, QS1200Component, CONF_QS1200_ID

DEPENDENCIES = ["qs1200"]

CONF_POWER = "power"
CONF_TIMER = "timer"
CONF_BOOST = "boost"
CONF_SELF_CLEAN = "self_clean"
CONF_LOCK = "lock"

QS1200PowerButton = qs1200_ns.class_("QS1200PowerButton", button.Button)
QS1200TimerButton = qs1200_ns.class_("QS1200TimerButton", button.Button)
QS1200BoostButton = qs1200_ns.class_("QS1200BoostButton", button.Button)
QS1200SelfCleanButton = qs1200_ns.class_("QS1200SelfCleanButton", button.Button)
QS1200LockButton = qs1200_ns.class_("QS1200LockButton", button.Button)

_BUTTONS = [
    (CONF_POWER, QS1200PowerButton),
    (CONF_TIMER, QS1200TimerButton),
    (CONF_BOOST, QS1200BoostButton),
    (CONF_SELF_CLEAN, QS1200SelfCleanButton),
    (CONF_LOCK, QS1200LockButton),
]

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_QS1200_ID): cv.use_id(QS1200Component),
        cv.Optional(CONF_POWER): button.button_schema(QS1200PowerButton),
        cv.Optional(CONF_TIMER): button.button_schema(QS1200TimerButton),
        cv.Optional(CONF_BOOST): button.button_schema(QS1200BoostButton),
        cv.Optional(CONF_SELF_CLEAN): button.button_schema(QS1200SelfCleanButton),
        cv.Optional(CONF_LOCK): button.button_schema(QS1200LockButton),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_QS1200_ID])
    for key, _ in _BUTTONS:
        if key not in config:
            continue
        btn = await button.new_button(config[key])
        cg.add(btn.set_parent(hub))

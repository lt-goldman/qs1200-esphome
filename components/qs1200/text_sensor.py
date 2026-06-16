import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from . import QS1200Component, CONF_QS1200_ID

DEPENDENCIES = ["qs1200"]

CONF_DISPLAY_CODE = "display_code"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_QS1200_ID): cv.use_id(QS1200Component),
        cv.Optional(CONF_DISPLAY_CODE): text_sensor.text_sensor_schema(),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_QS1200_ID])

    if CONF_DISPLAY_CODE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_DISPLAY_CODE])
        cg.add(hub.set_display_code_sensor(sens))

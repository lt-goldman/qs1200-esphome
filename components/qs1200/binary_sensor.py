import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from . import QS1200Component, CONF_QS1200_ID

DEPENDENCIES = ["qs1200"]

CONF_RUNNING    = "running"
CONF_BOOST      = "boost"
CONF_SLEEP      = "sleep"
CONF_LOW_FLOW   = "low_flow"
CONF_LOW_SALT   = "low_salt"
CONF_HIGH_SALT  = "high_salt"
CONF_SERVICE    = "service"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_QS1200_ID): cv.use_id(QS1200Component),
        cv.Optional(CONF_RUNNING):   binary_sensor.binary_sensor_schema(device_class="running"),
        cv.Optional(CONF_BOOST):     binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_SLEEP):     binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_LOW_FLOW):  binary_sensor.binary_sensor_schema(device_class="problem"),
        cv.Optional(CONF_LOW_SALT):  binary_sensor.binary_sensor_schema(device_class="problem"),
        cv.Optional(CONF_HIGH_SALT): binary_sensor.binary_sensor_schema(device_class="problem"),
        cv.Optional(CONF_SERVICE):   binary_sensor.binary_sensor_schema(device_class="problem"),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_QS1200_ID])

    for key, setter in [
        (CONF_RUNNING,   "set_running_sensor"),
        (CONF_BOOST,     "set_boost_sensor"),
        (CONF_SLEEP,     "set_sleep_sensor"),
        (CONF_LOW_FLOW,  "set_low_flow_sensor"),
        (CONF_LOW_SALT,  "set_low_salt_sensor"),
        (CONF_HIGH_SALT, "set_high_salt_sensor"),
        (CONF_SERVICE,   "set_service_sensor"),
    ]:
        if key in config:
            sens = await binary_sensor.new_binary_sensor(config[key])
            cg.add(getattr(hub, setter)(sens))

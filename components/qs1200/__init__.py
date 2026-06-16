import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.const import CONF_ID

CODEOWNERS = ["@local"]
MULTI_CONF = False

qs1200_ns = cg.esphome_ns.namespace("qs1200")
QS1200Component = qs1200_ns.class_("QS1200Component", cg.Component)

# Export so other platforms (binary_sensor, text_sensor, button) can import.
CONF_QS1200_ID = "qs1200_id"

CONF_FROM_MAIN_PIN = "from_main_pin"
CONF_TO_DISP_PIN   = "to_disp_pin"
CONF_FROM_DISP_PIN = "from_disp_pin"
CONF_TO_MAIN_PIN   = "to_main_pin"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(QS1200Component),
        cv.Required(CONF_FROM_MAIN_PIN): pins.internal_gpio_input_pin_schema,
        cv.Required(CONF_TO_DISP_PIN):   pins.internal_gpio_output_pin_schema,
        cv.Required(CONF_FROM_DISP_PIN): pins.internal_gpio_input_pin_schema,
        cv.Required(CONF_TO_MAIN_PIN):   pins.internal_gpio_output_pin_schema,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    from_main = await cg.gpio_pin_expression(config[CONF_FROM_MAIN_PIN])
    cg.add(var.set_from_main_pin(from_main))

    to_disp = await cg.gpio_pin_expression(config[CONF_TO_DISP_PIN])
    cg.add(var.set_to_disp_pin(to_disp))

    from_disp = await cg.gpio_pin_expression(config[CONF_FROM_DISP_PIN])
    cg.add(var.set_from_disp_pin(from_disp))

    to_main = await cg.gpio_pin_expression(config[CONF_TO_MAIN_PIN])
    cg.add(var.set_to_main_pin(to_main))

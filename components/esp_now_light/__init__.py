import esphome.codegen as cg
from esphome.components import light
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_LIGHT_ID
from esphome.core import HexInt

DOMAIN = "esp_now_light"
CONF_ESPNOW_ID = "espnow_id"
CONF_LIGHT_NUMBER = "light_number"
CONF_LIGHT_TYPE = "light_type"
CONF_RECEIVERS = "receivers"
CONF_WHITE_LIGHT_ID = "white_light_id"
CONF_SOURCE_ADDRESS = "source_address"

esp_now_light_ns = cg.esphome_ns.namespace(DOMAIN)
ESPNowLightReceiver = esp_now_light_ns.class_("ESPNowLightReceiver", cg.Component)
LightType = esp_now_light_ns.enum("LightType", is_class=True)
LIGHT_TYPES = {
    "binary": LightType.BINARY,
    "monochromatic": LightType.MONOCHROMATIC,
    "rgb": LightType.RGB,
    "rgbw": LightType.RGBW,
    "rgbww": LightType.RGBWW,
}

# Validate the version before importing the native ESP-NOW component.
REQUIREMENTS = cv.All(
    cv.require_esphome_version(2026, 11, 0), cv.only_on(["esp32", "esp8266"])
)


def mac_array(address):
    return cg.ArrayInitializer(*(HexInt(part) for part in address.parts))


def validate_receivers(config):
    numbers = set()
    for index, receiver in enumerate(config[CONF_RECEIVERS]):
        number = receiver[CONF_LIGHT_NUMBER]
        if number in numbers:
            raise cv.Invalid(
                "Light numbers must be unique",
                path=[CONF_RECEIVERS, index, CONF_LIGHT_NUMBER],
            )
        numbers.add(number)
        if CONF_WHITE_LIGHT_ID in receiver and receiver[CONF_LIGHT_TYPE] != "rgbw":
            raise cv.Invalid(
                "A separate white light requires light_type: rgbw",
                path=[CONF_RECEIVERS, index, CONF_WHITE_LIGHT_ID],
            )
    return config


def receiver_schema(config):
    from esphome.components.espnow import ESPNowComponent

    return cv.All(
        cv.Schema(
            {
                cv.GenerateID(): cv.declare_id(ESPNowLightReceiver),
                cv.GenerateID(CONF_ESPNOW_ID): cv.use_id(ESPNowComponent),
                cv.Optional(CONF_SOURCE_ADDRESS): cv.mac_address,
                cv.Required(CONF_RECEIVERS): cv.All(
                    cv.ensure_list(
                        cv.Schema(
                            {
                                cv.Required(CONF_LIGHT_ID): cv.use_id(light.LightState),
                                cv.Optional(CONF_LIGHT_NUMBER, default=1): cv.int_range(min=1, max=9),
                                cv.Required(CONF_LIGHT_TYPE): cv.enum(LIGHT_TYPES, lower=True),
                                cv.Optional(CONF_WHITE_LIGHT_ID): cv.use_id(light.LightState),
                            }
                        )
                    ),
                    cv.Length(min=1),
                ),
            }
        ).extend(cv.COMPONENT_SCHEMA),
        validate_receivers,
    )(config)


CONFIG_SCHEMA = cv.All(REQUIREMENTS, receiver_schema)


async def to_code(config):
    cg.add_define("USE_ESP_NOW_LIGHT_RECEIVER")
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await cg.add_variable(var.set_transport, config[CONF_ESPNOW_ID])
    if CONF_SOURCE_ADDRESS in config:
        cg.add(var.set_source_address(mac_array(config[CONF_SOURCE_ADDRESS])))
    for receiver in config[CONF_RECEIVERS]:
        target = await cg.get_variable(receiver[CONF_LIGHT_ID])
        white = (
            await cg.get_variable(receiver[CONF_WHITE_LIGHT_ID])
            if CONF_WHITE_LIGHT_ID in receiver
            else cg.nullptr
        )
        cg.add(
            var.add_light(
                receiver[CONF_LIGHT_NUMBER], receiver[CONF_LIGHT_TYPE], target, white
            )
        )

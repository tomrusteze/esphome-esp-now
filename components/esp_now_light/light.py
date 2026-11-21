import esphome.codegen as cg
from esphome.components import light, power_supply, uart
import esphome.config_validation as cv
from esphome.const import CONF_MAC_ADDRESS, CONF_POWER_SUPPLY, CONF_UART_ID

from . import (
    CONF_ESPNOW_ID,
    CONF_LIGHT_NUMBER,
    CONF_LIGHT_TYPE,
    LIGHT_TYPES,
    REQUIREMENTS,
    esp_now_light_ns,
    mac_array,
)

ESPNowLightOutput = esp_now_light_ns.class_("ESPNowLightOutput", light.LightOutput)


def output_schema(config):
    from esphome.components.espnow import ESPNowComponent

    return cv.All(
        light.light_schema(ESPNowLightOutput, light.LightType.RGB).extend(
            {
                cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
                cv.Optional(CONF_LIGHT_NUMBER, default=1): cv.int_range(min=1, max=9),
                cv.Required(CONF_LIGHT_TYPE): cv.enum(LIGHT_TYPES, lower=True),
                cv.Optional(CONF_ESPNOW_ID): cv.use_id(ESPNowComponent),
                cv.Optional(CONF_UART_ID): cv.use_id(uart.UARTComponent),
                cv.Optional(CONF_POWER_SUPPLY): cv.use_id(power_supply.PowerSupply),
            }
        ),
        cv.has_exactly_one_key(CONF_ESPNOW_ID, CONF_UART_ID),
    )(config)


CONFIG_SCHEMA = cv.All(REQUIREMENTS, output_schema)

UART_SCHEMA = uart.final_validate_device_schema(
    "esp_now_light", baud_rate=115200, data_bits=8, parity="NONE", stop_bits=1
)


def FINAL_VALIDATE_SCHEMA(config):
    if CONF_UART_ID in config:
        UART_SCHEMA(config)
    return config


async def to_code(config):
    cg.add_define("USE_ESP_NOW_LIGHT_OUTPUT")
    var = await light.new_light(
        config,
        mac_array(config[CONF_MAC_ADDRESS]),
        config[CONF_LIGHT_NUMBER],
        config[CONF_LIGHT_TYPE],
    )
    if CONF_ESPNOW_ID in config:
        await cg.add_variable(var.set_transport, config[CONF_ESPNOW_ID])
    if CONF_UART_ID in config:
        cg.add_define("USE_ESP_NOW_LIGHT_UART")
        await cg.add_variable(var.set_uart, config[CONF_UART_ID])
    if CONF_POWER_SUPPLY in config:
        await cg.add_variable(var.set_power_supply, config[CONF_POWER_SUPPLY])

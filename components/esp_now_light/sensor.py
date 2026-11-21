import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, uart
from esphome.const import ICON_COUNTER, STATE_CLASS_TOTAL_INCREASING

from . import REQUIREMENTS, esp_now_light_ns

DEPENDENCIES = ["uart"]
UARTErrorSensor = esp_now_light_ns.class_(
    "UARTErrorSensor", sensor.Sensor, cg.Component, uart.UARTDevice
)

CONFIG_SCHEMA = cv.All(
    REQUIREMENTS,
    sensor.sensor_schema(
        UARTErrorSensor,
        icon=ICON_COUNTER,
        accuracy_decimals=0,
        state_class=STATE_CLASS_TOTAL_INCREASING,
    )
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA),
)


FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "esp_now_light", baud_rate=115200, require_rx=True, data_bits=8, parity="NONE", stop_bits=1
)


async def to_code(config):
    cg.add_define("USE_ESP_NOW_LIGHT_UART_SENSOR")
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

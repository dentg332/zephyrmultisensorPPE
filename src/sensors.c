#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include "messages.h"
#include "gps_sensor.h"

#define I2C_NODE DT_NODELABEL(i2c2)
#if !DT_NODE_HAS_STATUS_OKAY(I2C_NODE)
#error "Unsupported board: i2c2 devicetree node label is not defined"
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
    ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

#define BUTTON_EVENT    1
#define SOS_EVENT    2
#define FF_EVENT    3
#define SEN_EVENT    4

#define SENSOR_THREAD_PRIOR 5
#define MPU6050_ADDR 0x68
#define REG_CONFIG 0x1A
#define REG_ACCEL_CONFIG 0x1C
#define REG_GYRO_CONFIG 0x1B
#define REG_PWR_MGMT_1 0x6B
#define REG_ACCEL_XOUT_H 0x3B
#define REG_TEMP_OUT_H 0x41
#define REG_GYRO_XOUT_H 0x43

#define MMA8451_ADDR 0x1D // Dirección I2C del MMA8451 (7 bits)
#define MMA8451_REG_WHO_AM_I 0x0D
#define MMA8451_WHO_AM_I_VAL 0x1A // Valor esperado
#define REG_CTRL_REG_1 0x2A
#define REG_CTRL_REG_2 0x2B
#define XYZ_DATA_CFG_REG 0x0E
#define REG_OUT_X_MSB 0x01 // 0x01 para X MSB
#define SENSITIVITY_8G 0x02
#define SENSITIVITY_8G_VALUE 1024
#define ACCELEROMETER_F_SETUP 0x09
#define PL_CFG_REG 0x11
#define SYSMOD_REGISTER 0x0B

// Nuevos define
#define PULSE_CFG 0x21
#define PULSE_THSX 0x23
#define PULSE_THSY 0x24
#define PULSE_THSZ 0x25
#define PULSE_TMLT 0x26
#define PULSE_LTCY 0x27

#define FF_MT_CFG 0x15
#define FF_MT_SRC 0x16
#define FF_MT_THS 0x17
#define FF_MT_COUNT 0x18

#define INT_SOURCE 0x0C

#define CTRL_REG4 0x2D
#define CTRL_REG5 0x2E

// Sensibilidad 8g
#define SENSITIVITY_8G 0x02

typedef enum {NORMAL_MODE, LP_MODE, SOS_MODE} t_state;

LOG_MODULE_REGISTER(sensors_module, LOG_LEVEL_INF);

static const struct device *i2c_dev = DEVICE_DT_GET(I2C_NODE);
static struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0});
static struct gpio_dt_spec sos_led = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(blue_led_1), gpios, {0});

static uint8_t generate_payload = 0;
t_state state = NORMAL_MODE;
static uint16_t bpm_value = 0;

//Get Specs from DT
static const struct adc_dt_spec adc_channels[] = {
    DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)
};

static int write_register(const struct device *i2c_dev,uint8_t addr, uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {reg, value};
    return i2c_write(i2c_dev, buf, 2, addr);
}

static int read_register(const struct device *i2c_dev, uint8_t addr, uint8_t reg, uint8_t *data, size_t len) {
    return i2c_write_read(i2c_dev, addr, &reg, 1, data, len);
}

static int8_t mpu6050_init(const struct device *i2c_dev) {
    if (!device_is_ready(i2c_dev)) {
        return -ENODEV;
    }
    write_register(i2c_dev, MPU6050_ADDR, REG_CONFIG, 0x03);
    write_register(i2c_dev, MPU6050_ADDR, REG_ACCEL_CONFIG, 0x08);
    write_register(i2c_dev, MPU6050_ADDR, REG_GYRO_CONFIG, 0x08);
    write_register(i2c_dev, MPU6050_ADDR, REG_PWR_MGMT_1, 0x00);
    return 0;
}

static int8_t mpu6050_read_accel(const struct device *i2c_dev, int16_t *ax, int16_t *ay, int16_t *az) {
    uint8_t buf[6];
    if (read_register(i2c_dev, MPU6050_ADDR, REG_ACCEL_XOUT_H, buf, 6) != 0) return -EIO;

    *ax = (int16_t)((buf[0] << 8) | buf[1]);
    *ay = (int16_t)((buf[2] << 8) | buf[3]);
    *az = (int16_t)((buf[4] << 8) | buf[5]);
    return 0;
}

static int8_t mpu6050_read_gyro(const struct device *i2c_dev, int16_t *gx, int16_t *gy, int16_t *gz) {
    uint8_t buf[6];
    if (read_register(i2c_dev, MPU6050_ADDR, REG_GYRO_XOUT_H, buf, 6) != 0) return -EIO;

    *gx = (int16_t)((buf[0] << 8) | buf[1]);
    *gy = (int16_t)((buf[2] << 8) | buf[3]);
    *gz = (int16_t)((buf[4] << 8) | buf[5]);
    return 0;
}

static int8_t mpu6050_read_temperature(const struct device *i2c_dev, int16_t *temp_c) {
    uint8_t buf[2];
    if (read_register(i2c_dev, MPU6050_ADDR, REG_TEMP_OUT_H, buf, 2) != 0) return -EIO;

    *temp_c = (int16_t)((buf[0] << 8) | buf[1]);
    return 0;
}


static int8_t mma8451_init(const struct device *i2c_dev) {
    if (!device_is_ready(i2c_dev)) {
        return -ENODEV;
    }
    //Reset
    uint8_t resetStatus = write_register(i2c_dev,MMA8451_ADDR, REG_CTRL_REG_2, 0x40);
    k_sleep(K_MSEC(5));// Espera para el reset
    //Inactive
    write_register(i2c_dev,MMA8451_ADDR, REG_CTRL_REG_1, 0x20);
    //Activamos flag FF Detection
    write_register(i2c_dev,MMA8451_ADDR, FF_MT_CFG, 0xB8);
    //Definimos THreshold
    write_register(i2c_dev,MMA8451_ADDR, FF_MT_THS, 0x05); //0x0F g
    //Definimos Tiempos de debounce
    write_register(i2c_dev,MMA8451_ADDR, FF_MT_COUNT, 0x02);

    //Definimos latencia para la lectura del bit que se activa
    write_register(i2c_dev,MMA8451_ADDR, PULSE_LTCY, 0x09);
    //Habilitamos Interrupciones FF
    write_register(i2c_dev,MMA8451_ADDR, CTRL_REG4, 0x04);//Para solo FF 0x04 para FF y TAPS 0x0C
    //ruteamos a INT1
    write_register(i2c_dev,MMA8451_ADDR, CTRL_REG5, 0x0C);

    //Establecemos 8G
    write_register(i2c_dev,MMA8451_ADDR, XYZ_DATA_CFG_REG, 0x02);
    //Set Active
    uint8_t ctrl1Value = 0;
    read_register(i2c_dev, MMA8451_ADDR, REG_CTRL_REG_1, &ctrl1Value, 1);
    ctrl1Value|=0x01;
    write_register(i2c_dev,MMA8451_ADDR, REG_CTRL_REG_1, ctrl1Value);
    
    return 0;
}

static int8_t mma8451_read_accel(const struct device *i2c_dev, float *acc_x, float *acc_y, float *acc_z) {

    uint8_t ibuf[6] = {0, 0, 0, 0, 0, 0};
    uint8_t baseReg = REG_OUT_X_MSB;

    read_register(i2c_dev, MMA8451_ADDR, REG_OUT_X_MSB, &ibuf, 6);
    int16_t x_int = ((int16_t)(ibuf[0]<<8 | ibuf[1]))>>2;
    int16_t y_int = ((int16_t)(ibuf[2]<<8 | ibuf[3]))>>2;
    int16_t z_int = ((int16_t)(ibuf[4]<<8 | ibuf[5]))>>2;

    *acc_x = (float) x_int / SENSITIVITY_8G_VALUE * 9.81;
    *acc_y = (float) y_int / SENSITIVITY_8G_VALUE * 9.81;
    *acc_z = (float) (z_int) / SENSITIVITY_8G_VALUE * 9.81;
}
static uint8_t mma8451_read_freefall(const struct device *i2c_dev) {
    uint8_t intBufValue;
    read_register(i2c_dev, MMA8451_ADDR, INT_SOURCE, &intBufValue, 1);
    if (intBufValue & 0x04) {
        uint8_t ffBufValue;
        /*Lectura necesaria para hacer clear a bit EA y reiniciar deteccion*/
        read_register(i2c_dev, MMA8451_ADDR, FF_MT_SRC, &ffBufValue, 1);
        return 1;
    } else {
        return 0;
    }
    
}

uint8_t leds_init (void)  {
    if (!gpio_is_ready_dt(&led)) {
		LOG_ERR("Error LED device %s is not ready; ignoring it",led.port->name);
        return 0;
	}
    if (gpio_pin_configure_dt(&led, GPIO_OUTPUT)) {
        LOG_ERR("Error failed to configure LED device %s pin %d\n", led.port->name, led.pin);
        return 0;
    }
    if (!gpio_is_ready_dt(&sos_led)) {
		LOG_INF("Error SOS LED device %s is not ready; ignoring it\n", sos_led.port->name);
		return 0;
	}
    if (gpio_pin_configure_dt(&sos_led, GPIO_OUTPUT)) {
        LOG_INF("Error failed to configure SOS LED device %s pin %d\n", sos_led.port->name, sos_led.pin);
        return 0;
    }
    gpio_pin_set_dt(&sos_led, 1);
    gpio_pin_set_dt(&led, 0);
    LOG_INF("Leds ready");
    return 1;
}

static uint16_t read_adc_u16(uint16_t *raw_value) {
    if (!adc_is_ready_dt(&adc_channels[0])) {
        LOG_ERR("ADC no está listo");
        return -ENODEV;
    }

    int err = adc_channel_setup_dt(&adc_channels[0]);
    if (err < 0) {
        LOG_ERR("Fallo en la configuración del canal ADC (%d)", err);
        return err;
    }

    uint16_t buf;
    struct adc_sequence sequence = {
        .buffer = &buf,
        .buffer_size = sizeof(buf),
    };

    (void)adc_sequence_init_dt(&adc_channels[0], &sequence);

    err = adc_read_dt(&adc_channels[0], &sequence);
    if (err < 0) {
        LOG_ERR("Error en lectura ADC (%d)", err);
        return err;
    }

    *raw_value = buf;
    //Para obtener el valor en mv
    //int32_t val = adc_channels[0].channel_cfg.differential ? (int16_t)buf : buf;

    return 0;
}

ZBUS_CHAN_DEFINE(events_chan,          /* Name */
                 uint8_t,              /* Message type */
                 NULL,                 /* Validator */
                 NULL,                 /* User data */
                 ZBUS_OBSERVERS(sensor_sub), /* observers */
                 ZBUS_MSG_INIT(0)      /* Initial value 0*/
);

ZBUS_CHAN_DEFINE(sensor_data_chan,          /* Name */
                 struct sensor_msg,              /* Message type */
                 NULL,                 /* Validator */
                 NULL,                 /* User data */
                 ZBUS_OBSERVERS(parser_sub), /* observers */
                 ZBUS_MSG_INIT(0)      /* Initial value 0*/
);

ZBUS_SUBSCRIBER_DEFINE(sensor_sub, 8);
ZBUS_OBS_DECLARE(parser_sub);


void sensors_th(void) {
    LOG_INF("Getting sensors ready...");
    if (mpu6050_init(i2c_dev))
        LOG_ERR("MPU6050 device NOT configured");
    if (mma8451_init(i2c_dev))
        LOG_ERR("MMA8451 device NOT configured");
    leds_init();

    LOG_INF("");

	const struct zbus_channel *chan;
    t_state prev_state = NORMAL_MODE;
    uint8_t sos_counter = 0;
    struct sensor_msg sm = {0};

	while (!zbus_sub_wait(&sensor_sub, &chan, K_FOREVER)) {
        uint8_t event_msg;
        zbus_chan_read(chan, &event_msg, K_MSEC(250));
        switch (event_msg) {
        case BUTTON_EVENT:
            LOG_INF("BUTTON_EVENT");
                if (state == NORMAL_MODE) {
                    gpio_pin_set_dt(&sos_led, 0);
                    gpio_pin_set_dt(&led, 1);
                    state = LP_MODE;
                } else {
                    gpio_pin_set_dt(&sos_led, 1);
                    gpio_pin_set_dt(&led, 0);
                    state = NORMAL_MODE;
                } 
            break;
        case SOS_EVENT:
            LOG_INF("SOS_EVENT");
                if (state != SOS_MODE)
                    prev_state = state;
                state = SOS_MODE;
                sos_counter = 0;
                generate_payload = 1;
                sm.sos_status = 1;
                sm.sos_counter = sos_counter;
            break;
        case FF_EVENT:
            LOG_INF("FF_EVENT");
            if (mma8451_read_freefall(i2c_dev)){
                generate_payload = 1;
                sm.ff_status = 1;
            }
                
            break;
        case SEN_EVENT:
            LOG_INF("SEN_EVENT");
            switch (state) {
            case NORMAL_MODE:
                generate_payload = 1;
                mpu6050_read_accel(i2c_dev, &sm.ax, &sm.ay, &sm.az);
                mpu6050_read_gyro(i2c_dev, &sm.gx, &sm.gy, &sm.gz);
                mpu6050_read_temperature(i2c_dev, &sm.temp);
                sm.pulsiMeterValue = bpm_value;
                sm.latitude = readLatitude();
                sm.latDir = readLatitudeDirection();
                sm.longitude = readLongitude();
                sm.lonDir = readLLongitudeDirection();
                sm.fix_indicator = readFixIndicator();
                sm.altitude = readAltitude();
                sm.ff_status = 0;
                sm.sos_status = 0;
                break;
            case LP_MODE:
                generate_payload = 1;
                sm.ax, sm.ay, sm.az = 0;
                sm.gx, sm.gy, sm.gz = 0;
                sm.temp = 0;
                sm.pulsiMeterValue = 0;
                sm.latitude = readLatitude();
                sm.latDir = readLatitudeDirection();
                sm.longitude = readLongitude();
                sm.lonDir = readLLongitudeDirection();
                sm.fix_indicator = readFixIndicator();
                sm.altitude = readAltitude();
                sm.ff_status = 0;
                sm.sos_status = 0;
                break;
            case SOS_MODE:
                sos_counter ++;    
                if (sos_counter <= 3) {
                    generate_payload = 1;
                    sm.sos_counter = sos_counter;
                } else {
                    state = prev_state;
                }
                break;
            
            default:
                break;
            }
            break;
        default:
            LOG_INF("DEFAULT");
            break;
        }
        if (generate_payload) {
            generate_payload = 0;
            zbus_chan_pub(&sensor_data_chan, &sm, K_MSEC(250));
        }
		
		
	}
}

K_THREAD_DEFINE(sensors_th_id, 2024, sensors_th, NULL, NULL, NULL, 3, 0, 0);

void bpm_detection_thread(void) {
    uint32_t now;
    uint8_t bpm_measurement_counter = 0;
    uint8_t beats_counter = 0;
    while (1) {
        if (state == NORMAL_MODE) {
            uint16_t adc_val;
            if (read_adc_u16(&adc_val) == 0) {
                if (adc_val > 3500){
                    beats_counter++;
                    //LOG_INF("Heartbeat Detected - %d",adc_val);
                }
            }
            bpm_measurement_counter++;
            if (bpm_measurement_counter == 120) {
                bpm_value = beats_counter * 2;
                bpm_measurement_counter = 0;
                beats_counter = 0;
                //LOG_INF("BPM: %d", bpm_value);
            }
        }
        k_sleep(K_MSEC(250));
    }
}

#define STACK_SIZE 1024
#define THREAD_PRIORITY 5

K_THREAD_DEFINE(bpm_tid, STACK_SIZE, bpm_detection_thread, NULL, NULL, NULL, THREAD_PRIORITY, 0, 0);
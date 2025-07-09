#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(gps_parser, LOG_LEVEL_INF); 

#define UART_BUFFER_SIZE 128
#define GPS_BUFFER_SIZE 80 

static const struct device *uart_dev;
static uint8_t recv_buffer[UART_BUFFER_SIZE];
static uint8_t gps_buffer[GPS_BUFFER_SIZE];
static size_t recv_index = 0;
static bool capturing = false;

static const char gpgga_prefix[] = "$GPGGA";
// Estructura para almacenar datos del GPS
typedef struct
{
    float latitude;
    char lat_dir; // N o S o M
    float longitude;
    char lon_dir; // E o O
    uint8_t fix_indicator; // 0 Fix no disponible 1 Fix 2 Diferential fix
    //int num_satellites;
    float altitude;
} gps_data_t;

static gps_data_t current_gps_data;

// Función para parsear sentencia GPGGA
void parse_gpgga(const char *nmea_sentence, gps_data_t *gps_data)
{
    char *token;
    char sentence_copy[UART_BUFFER_SIZE]; // Buffer copia auxiliar
    strncpy(sentence_copy, nmea_sentence, UART_BUFFER_SIZE - 1);
    sentence_copy[UART_BUFFER_SIZE - 1] = '\0'; // Fijamos valor final

    // Eliminar el checksum al final si está presente
    char *checksum_start = strchr(sentence_copy, '*');
    if (checksum_start)
    {
        *checksum_start = '\0';
    }

    // Omitir "$GPGGA" - Ignoramos
    token = strtok(sentence_copy, ","); // "$GPGGA"

    // 1. Time - Ignoramos
    token = strtok(NULL, ",");

    // 2. Latitud valor
    token = strtok(NULL, ",");
    if (token && strlen(token) > 0)
    {
        gps_data->latitude = atof(token);
    }
    else
    {
        gps_data->latitude = 0.0;
    }

    // 3. Latitud
    token = strtok(NULL, ",");
    if (token && strlen(token) > 0)
    {
        gps_data->lat_dir = token[0];
    }
    else
    {
        gps_data->lat_dir = ' ';
    }

    // 4. Longitud valor
    token = strtok(NULL, ",");
    if (token && strlen(token) > 0)
    {
        gps_data->longitude = atof(token);
    }
    else
    {
        gps_data->longitude = 0.0;
    }

    // 5. Longitud
    token = strtok(NULL, ",");
    if (token && strlen(token) > 0)
    {
        gps_data->lon_dir = token[0];
    }
    else
    {
        gps_data->lon_dir = ' ';
    }

    // 6. Fix Indicator
    token = strtok(NULL, ",");
    if (token && strlen(token) > 0)
    {
        gps_data->fix_indicator = atoi(token);
    }
    else
    {
        gps_data->fix_indicator = 0;
    }

    // 7. Number of Satellites - Ignoramos
    token = strtok(NULL, ",");
    

    // 8. HDOP - Ignoramos
    token = strtok(NULL, ",");

    // 9. Altitud
    token = strtok(NULL, ",");
    if (token && strlen(token) > 0)
    {
        gps_data->altitude = atof(token);
    }
    else
    {
        gps_data->altitude = 0.0;
    }
}

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(user_data);

    switch (evt->type)
    {
        case UART_RX_RDY:
        for (int i = 0; i < evt->data.rx.len; i++)
        {
            char c = evt->data.rx.buf[evt->data.rx.offset + i];

            // Buscamos Inicio Sentencia %GPGGA
            if (!capturing)
            {
                // Desplazamiento de ventana para comparar con "$GPGGA"
                // Un pequeño buffer estático para detectar el prefijo
                static char window[6] = {0};
                for (int j = 0; j < 5; j++)
                {
                    window[j] = window[j + 1];
                }
                window[5] = c;

                if (strncmp(window, gpgga_prefix, 6) == 0)
                {
                    capturing = true;
                    recv_index = 0;
                    memcpy(gps_buffer, gpgga_prefix, 6);
                    recv_index = 6;
                }
            }
            else
            {
                // Captura $GPGGA hasta \n
                if (recv_index < UART_BUFFER_SIZE - 1)
                {
                    gps_buffer[recv_index++] = c;

                    if (c == '\n')
                    {
                        gps_buffer[recv_index] = '\0';

                        parse_gpgga((char *)gps_buffer, &current_gps_data);
                        capturing = false;
                        recv_index = 0;
                    }
                }
                else
                {
                    capturing = false;
                    recv_index = 0;
                }
            }
        }
        break;

    case UART_RX_DISABLED:
        memset(gps_buffer,0,GPS_BUFFER_SIZE);
        break;

    case UART_RX_BUF_REQUEST:
        uart_rx_buf_rsp(dev, gps_buffer, GPS_BUFFER_SIZE);
        break;
    default:
        break;
    }
}

int usart1_capture(const struct device *uart_dev)
{

    if (!device_is_ready(uart_dev))
    {
        LOG_ERR("UART device not ready");
        return -ENODEV;
    }

    uart_callback_set(uart_dev, uart_cb, NULL);

    int err = uart_rx_enable(uart_dev, recv_buffer, UART_BUFFER_SIZE, SYS_FOREVER_US);
    if (err)
    {
        LOG_ERR("Failed to enable UART RX: %d", err);
        return err;
    }

    LOG_INF("USART GPS listener succesfully started");

    current_gps_data.latitude = 0.0f;
    current_gps_data.lat_dir = " ";
    current_gps_data.longitude = 0.0f;
    current_gps_data.lon_dir = " ";
    current_gps_data.fix_indicator = 0;
    current_gps_data.altitude = 0.0f;
    return 0;
}

float readLatitude(void){ return current_gps_data.latitude; }
char readLatitudeDirection(void){ return current_gps_data.lat_dir; }
float readLongitude(void){ return current_gps_data.longitude; }
float readLLongitudeDirection(void){ return current_gps_data.lon_dir; }
uint8_t readFixIndicator(void){ return current_gps_data.fix_indicator; }
float readAltitude (void) { return current_gps_data.altitude; }
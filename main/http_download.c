#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_system.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "hardware.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "pax_codecs.h"
#include "pax_gfx.h"
#include "soc/rtc.h"
#include "soc/rtc_cntl_reg.h"
#include "wifi_connect.h"
#include "wifi_connection.h"
#include "http_download.h"

static const char* TAG = "HTTP download";

typedef struct {
    FILE*     fd;                // For downloading directly to file on filesystem
    uint8_t** buffer;            // Dynamically allocated buffer for downloading to RAM (malloced in event handler, used if fd is not set)
    size_t    size;              // File size as indicated by content-length header (set in event handler)
    size_t    received;          // Amount of data received (set in event handler)
    bool      error;             // Indication that an error event happened (set in event handler)
    bool      connected;         // Indication that the HTTP client has connected to the server (set in event handler)
    bool      finished;          // Indication that the operation has completed (set in event handler)
    bool      disconnected;      // Indication that the HTTP client has disconnected from the server (set in event handler)
    bool      out_of_memory;     // Indication that malloc failed
    bool      out_of_allocated;  // Indication that the server sent more data than indicated with the content-length header
} http_download_info_t;

static esp_err_t _event_handler(esp_http_client_event_t* evt) {
    http_download_info_t* info = (http_download_info_t*) evt->user_data;
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            info->error = true;
            break;
        case HTTP_EVENT_ON_CONNECTED:
            info->connected = true;
            break;
        case HTTP_EVENT_HEADERS_SENT:
            break;
        case HTTP_EVENT_ON_HEADER:
            {
                const char content_length_key[] = "Content-Length";
                if ((strlen(evt->header_key) == strlen(content_length_key)) &&
                    (strncasecmp(content_length_key, evt->header_key, strlen(content_length_key)) == 0)) {
                    // Header value is content length
                    info->size = atoi(evt->header_value);
                    printf("SIZE KNOWN: %u bytes\r\n", info->size);
                    if ((info->size > 0) && (info->buffer != NULL)) {  // Buffer poiner is set, buffer pointer points to NULL
                        *info->buffer = malloc(info->size);
                        printf("BUFFER MALLOC'ED (%u bytes)\r\n", info->size);
                        if (*info->buffer == NULL) {
                            info->out_of_memory = true;
                            return ESP_ERR_NO_MEM;
                        }
                    }
                } else {
                    // printf("HTTP_EVENT_ON_HEADER, key=%s, value=%s\r\n", evt->header_key, evt->header_value);
                }
                break;
            }
        case HTTP_EVENT_ON_DATA:
            if (info->fd != NULL) {  // Write directly to file on filesystem
                printf("Writing to FILE @ %p (%u bytes): %u of %u bytes.\r\n", info->fd, evt->data_len, info->received + evt->data_len, info->size);
                fwrite(evt->data, 1, evt->data_len, info->fd);
            } else if (info->buffer != NULL && *info->buffer != NULL) {
                if (info->received + evt->data_len <= info->size) {
                    uint8_t* dest = &((*info->buffer)[info->received]);
                    printf("Writing to RAM @ %p (%u bytes): %u of %u bytes.\r\n", dest, evt->data_len, info->received + evt->data_len, info->size);
                    memcpy(dest, evt->data, evt->data_len);
                } else {
                    printf("Downloaded too much? %u with %u in content-length header\r\n", info->received + evt->data_len, info->size);
                    info->out_of_allocated = true;
                    return ESP_ERR_NO_MEM;
                }
            } else {
                return ESP_FAIL;
            }
            info->received += evt->data_len;
            break;
        case HTTP_EVENT_ON_FINISH:
            info->finished = true;
            break;
        case HTTP_EVENT_DISCONNECTED:
            info->disconnected = true;
            break;
    }
    return ESP_OK;
}

static bool download_success(esp_err_t err, http_download_info_t* info) {
    return (err == ESP_OK) && (!(info->error || info->out_of_allocated || info->out_of_memory)) && info->finished && (info->received == info->size);
}

static bool _download_file(const char* url, const char* path) {
    FILE* fd = fopen(path, "w");
    if (fd == NULL) {
        ESP_LOGE(TAG, "Failed to open file");
        return false;
    }

    http_download_info_t info = {0};
    info.fd                   = fd;

    esp_http_client_config_t config = {
        .url = url, .use_global_ca_store = true, .keep_alive_enable = true, .timeout_ms = 10000, .user_data = (void*) &info, .event_handler = _event_handler};
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t                err    = esp_http_client_perform(client);
    fclose(fd);
    esp_http_client_cleanup(client);
    return download_success(err, &info);
}

bool download_file(const char* url, const char* path) {
    return download_file_retries(url, path, 3);
}

bool download_file_retries(const char* url, const char* path, int retries) {
    int retry = retries;
    while (retry--) {
        if (_download_file(url, path)) return true;
        printf("DL waiting to retry ...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    return false;
}

static bool _download_ram(const char* url, uint8_t** ptr, size_t* size) {
    http_download_info_t info       = {0};
    info.buffer                     = ptr;
    esp_http_client_config_t config = {
        .url = url, .use_global_ca_store = true, .keep_alive_enable = true, .timeout_ms = 10000, .user_data = (void*) &info, .event_handler = _event_handler};
    esp_http_client_handle_t client  = esp_http_client_init(&config);
    esp_err_t                err     = esp_http_client_perform(client);
    bool                     success = download_success(err, &info);
    if (success && (size != NULL)) *size = info.size;
    printf("Buffer: %p -> %p\r\n", ptr, *ptr);
    esp_http_client_cleanup(client);
    return success;
}

bool download_ram(const char* url, uint8_t** ptr, size_t* size) {
    return download_ram_retries(url, ptr, size, 3);
}

bool download_ram_retries(const char* url, uint8_t** ptr, size_t* size, int reties) {
    int retry = reties;
    while (retry--) {
        if (_download_ram(url, ptr, size)) return true;
        printf("DL waiting to retry ...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    return false;
}

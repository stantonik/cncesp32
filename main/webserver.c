/**
 * @author      : stanleyarn (stanleyarn@$HOSTNAME)
 * @file        : webserver
 * @created     : Monday Aug 26, 2024 19:32:53 CEST
 */

/******************************/
/*         Includes           */
/******************************/
#include "webserver.h"

#include "esp_wifi_types_generic.h"

#define JSMN_HEADER
#include "jsmn.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_check.h"
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "mdns.h"
#include "sd.h"
#include "config.h"

/******************************/
/*     Global Variables       */
/******************************/

/******************************/
/*     Static Variables       */
/******************************/
#define TAG "webserver"

static int retry_num = 0;
static char *html_str;
static void (*post_callback)(char *key, char *val);
static bool initialized = false;

/******************************/
/*    Function Prototypes     */
/******************************/
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

static esp_err_t root_get_handler(httpd_req_t *req);
static esp_err_t root_post_handler(httpd_req_t *req);

/******************************/
/*   Function Definitions     */
/******************************/

esp_err_t webserver_init()
{
  /* NVS init */
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)  
  {
    nvs_flash_erase();
    nvs_flash_init();
  }

  /* Wifi station initialization */
  esp_netif_init();
  esp_event_loop_create_default();
  esp_netif_create_default_wifi_sta();
  wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&wifi_initiation); //     
  esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
  wifi_config_t wifi_cfg = {  };
  config_get_setting(config_wifi_ssid, &wifi_cfg.sta.ssid, CONFIG_STRING);
  config_get_setting(config_wifi_password, &wifi_cfg.sta.password, CONFIG_STRING);
  char temp[32];
  config_get_setting(config_wifi_auth_mode, temp, CONFIG_STRING);
  if (strcmp(temp, "wpa/wpa2") == 0)
  {
    wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK;
  }
  else if (strcmp(temp, "wpa2/wpa3") == 0) 
  {
    wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_WPA3_PSK;
  }
  else
  {
    ESP_LOGE(TAG, "'%s' invilid wifi auth mode", temp);
    return -1;
  }

  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_config((wifi_interface_t)ESP_IF_WIFI_STA, &wifi_cfg);
  esp_wifi_start();
  esp_wifi_connect();

  /* Start mDNS service */
  config_get_setting(config_hostname, temp, CONFIG_STRING);
  ESP_RETURN_ON_ERROR(mdns_init(), TAG, "mDNS service init error.");
  mdns_hostname_set(temp);
  mdns_instance_name_set(temp);

  /* Get HTML file */
  struct stat st = {0};

  if (stat(SD_MOUNT_POINT"/system/web", &st) == -1) 
  {
    mkdir(SD_MOUNT_POINT"/system/web", 0700);
    html_str = "html file not found on the sd card";
    ESP_LOGW(TAG, "index not found");
  }
  else
  {
    ESP_LOGI(TAG, "reading index file on sd card..");
    FILE *index = fopen(SD_MOUNT_POINT"/system/web/index.html", "r");
    if (index == NULL)
    {
      html_str = "html file not found on the sd card";
      ESP_LOGW(TAG, "index not found");
    }
    else
    {
      // Seek to the end of the file to determine its size
      fseek(index, 0, SEEK_END);
      long file_size = ftell(index);
      rewind(index);

      // Allocate memory for the file content (+1 for the null terminator)
      html_str = (char *)malloc((file_size + 1) * sizeof(char));
      if (html_str == NULL) ESP_LOGW(TAG, "no memory left");

      // Read the entire file into the buffer
      fread(html_str, sizeof(char), file_size, index);

      html_str[file_size] = '\0';
      fclose(index);
      ESP_LOGI(TAG, "reading complete");
    }
  }

  /* Start server */
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  config_get_setting(config_port, &config.server_port, CONFIG_UINT);
  config.uri_match_fn = httpd_uri_match_wildcard;

  ESP_LOGI(TAG, "Starting HTTP Server on port: '%d'", config.server_port);
  if (httpd_start(&server, &config) != ESP_OK) 
  {
    ESP_LOGI(TAG, "Failed to start file server!");
    return -1;
  }

  httpd_uri_t _root_get_handler = 
  {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_get_handler,
    .user_ctx = NULL
  };
  httpd_register_uri_handler(server, &_root_get_handler);

  httpd_uri_t _root_post_handler = 
  {
    .uri = "/data",
    .method = HTTP_POST,
    .handler = root_post_handler,
    .user_ctx = NULL
  };
  httpd_register_uri_handler(server, &_root_post_handler);

  initialized = true;

  return ESP_OK;
}

esp_err_t webserver_set_post_callback(void (*callback)(char *, char *))
{
  ESP_RETURN_ON_FALSE(initialized, -1, TAG, "server not initialized");
  ESP_RETURN_ON_FALSE(callback != NULL, -1, TAG, "post callback is null");
  post_callback = callback;

  return ESP_OK;
}

esp_err_t root_get_handler(httpd_req_t *req)
{
  httpd_resp_sendstr_chunk(req, html_str);

  return ESP_OK;
}

esp_err_t root_post_handler(httpd_req_t *req)
{
  ESP_LOGI(TAG, "root_post_handler req->uri=[%s]", req->uri);
  ESP_LOGI(TAG, "root_post_handler content length %i", (int)req->content_len);

  char *buf = malloc(req->content_len + 1);
  size_t off = 0;
  while (off < req->content_len) 
  {
    /* Read data received in the request */
    int ret = httpd_req_recv(req, buf + off, req->content_len - off);
    if (ret <= 0)
    {
      if (ret == HTTPD_SOCK_ERR_TIMEOUT)
      {
        httpd_resp_send_408(req);
      }
      free (buf);
      return ESP_FAIL;
    }
    off += ret;
  }
  buf[off] = '\0';
  ESP_LOGI(TAG, "root_post_handler buf=[%s]", buf);

  jsmn_parser p;
  jsmn_init(&p);
  jsmntok_t t[3];
  size_t r = jsmn_parse(&p, buf, strlen(buf), t, 3);

  if (r > 0 && t[0].type == JSMN_OBJECT) 
  {
    char *key = strndup(buf + t[1].start, t[1].end - t[1].start);
    char *value = strndup(buf + t[2].start, t[2].end - t[2].start);

    post_callback(key, value);

    free(key);
    free(value);
  }

  free(buf);

  // Redirect to the root page
  httpd_resp_set_status(req, "303 See Other");
  httpd_resp_set_hdr(req, "Location", "/");
  httpd_resp_sendstr(req, "Redirecting to /");

  return ESP_OK;
}

void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
  if(event_id == WIFI_EVENT_STA_START)
  {
    printf("WIFI CONNECTING....\n");
  }
  else if (event_id == WIFI_EVENT_STA_CONNECTED)
  {
    printf("WiFi CONNECTED\n");
  }
  else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
  {
    printf("WiFi lost connection\n");
    if(retry_num < 5)
    {
      esp_wifi_connect();
      retry_num++;
      printf("Retrying to Connect...\n");
    }
  }
}

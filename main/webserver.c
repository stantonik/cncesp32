/**
 * @author      : stanleyarn (stanleyarn@$HOSTNAME)
 * @file        : webserver
 * @created     : Monday Aug 26, 2024 19:32:53 CEST
 */

/******************************/
/*         Includes           */
/******************************/
#include "webserver.h"

#include "jsmn.h"

#include <string.h>

#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_check.h"
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "mdns.h"

/******************************/
/*     Global Variables       */
/******************************/

/******************************/
/*     Static Variables       */
/******************************/
#define TAG "webserver"

static int retry_num = 0;
static const char *html_str;
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
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)   {
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
  wifi_config_t wifi_configuration = {
    .sta = {
      .ssid = WIFI_SSID,
      .password = WIFI_PASSWORD,
      .threshold.authmode = WIFI_AUTH_MODE,
    }
  };
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_config((wifi_interface_t)ESP_IF_WIFI_STA, &wifi_configuration);
  esp_wifi_start();
  esp_wifi_connect();

  /* Start mDNS service */
  ESP_RETURN_ON_ERROR(mdns_init(), TAG, "mDNS service init error.");
  mdns_hostname_set("cncesp");
  mdns_instance_name_set("Stanley's CNC");

  /* Get HTML file */
  html_str = 
"<!DOCTYPE html>\n"
"<html lang=\"en\">\n"
"\n"
"  <head>\n"
"    <meta charset=\"UTF-8\">\n"
"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
"    <title>CNC Control Interface</title>\n"
"    <link href=\"https://cdn.jsdelivr.net/npm/tailwindcss@2.2.19/dist/tailwind.min.css\" rel=\"stylesheet\">\n"
"  </head>\n"
"\n"
"  <body class=\"bg-gray-900 text-white min-h-screen p-5\">\n"
"    <!-- Navigation bar -->\n"
"    <div>\n"
"    </div>\n"
"\n"
"    <div class=\"grid grid-cols-1 md:grid-cols-7 gap-6\">\n"
"      <!-- Components go here -->\n"
"\n"
"      <!-- Informations -->\n"
"      <div class=\"col-span-2 bg-gray-800 p-5 rounded-lg\">\n"
"        <h2 class=\"text-xl font-semibold mb-4\">Status</h2>\n"
"        <ul>\n"
"          <li>Status : </li>\n"
"          <li>Temperature : </li>\n"
"        </ul>\n"
"      </div>\n"
"\n"
"      <!-- Console -->\n"
"      <div class=\"col-span-3 bg-gray-800 p-5 rounded-lg\">\n"
"        <h2 class=\"text-xl font-semibold mb-4\">&gt Console</h2>\n"
"        <textarea readonly id=\"console-log\" rows=\"8\" class=\"block resize-none p-2.5 w-full text-sm rounded-lg border bg-gray-700 border-gray-600 text-white focus:outline-none\"></textarea><br>\n"
"        <form id=\"console-chat-form\">\n"
"          <input autocomplete=\"off\" id=\"console-chat\" class=\"uppercase p-2 w-full text-sm rounded-lg border bg-gray-700 border-gray-600 placeholder-gray-400 text-white focus:ring-blue-500 focus:border-blue-500\" placeholder=\"Type a command...\">\n"
"        </form>\n"
"      </div>\n"
"\n"
"      <!-- Print -->\n"
"      <div class=\"col-span-2 bg-gray-800 p-5 rounded-lg\">\n"
"        <h2 class=\"text-xl font-semibold mb-4\">Print</h2>\n"
"        <form enctype=\"multipart/form-data\" id=\"print-form\">\n"
"          <!-- File deposit -->\n"
"          <div class=\"flex items-center justify-center w-full\">\n"
"            <label for=\"dropzone-file\" class=\"flex flex-col items-center justify-center w-full h-40 border-2 border-dashed rounded-lg cursor-pointer hover:bg-gray-800 bg-gray-700 border-gray-600 hover:border-gray-500 hover:bg-gray-600\">\n"
"              <div class=\"flex flex-col items-center justify-center pt-5 pb-6\">\n"
"                <svg class=\"w-8 h-8 mb-4 text-gray-500 dark:text-gray-400\" aria-hidden=\"true\" xmlns=\"http://www.w3.org/2000/svg\" fill=\"none\" viewBox=\"0 0 20 16\">\n"
"                  <path stroke=\"currentColor\" stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M13 13h3a3 3 0 0 0 0-6h-.025A5.56 5.56 0 0 0 16 6.5 5.5 5.5 0 0 0 5.207 5.021C5.137 5.017 5.071 5 5 5a4 4 0 0 0 0 8h2.167M10 15V6m0 0L8 8m2-2 2 2\"/>\n"
"                </svg>\n"
"                <p class=\"mb-2 text-sm text-gray-400 text-center\"><span class=\"font-semibold\">Click to upload</span> G-Code or drag and drop<p>\n"
"                <div id=\"dropzone-filename-div\"></div>\n"
"              </div>\n"
"              <input id=\"dropzone-file\" type=\"file\" class=\"hidden\" />\n"
"            </label>\n"
"          </div><br>\n"
"\n"
"          <button type=\"submit\" class=\"p-1 bg-gray-600 hover:bg-gray-700 rounded-lg w-full\">Upload</button><br><br>\n"
"        </form>\n"
"      </div>\n"
"\n"
"      <!-- Mouvement control -->\n"
"      <div class=\"col-start-1 col-span-7 bg-gray-800 p-5 rounded-lg\">\n"
"        <h2 class=\"text-xl font-semibold mb-4\">Mouvement control</h2>\n"
"        <div class=\"grid grid-cols-1 md:grid-cols-7 gap-x-6\">\n"
"          <!-- Live controls -->\n"
"          <div class=\"col-span-2 p-4\">\n"
"            <label for=\"speed-range\" class=\"block mb-2 text-sm font-medium\">Speed factor</label>\n"
"            <input id=\"speed-range\" type=\"range\" min=\"0\" max=\"10\" value=\"5\" class=\"w-full h-2 rounded-lg appearance-none cursor-pointer bg-gray-700\"><br><br>\n"
"            <label for=\"extrusion-range\" class=\"block mb-2 text-sm font-medium\">Extrusion factor</label>\n"
"            <input id=\"extrusion-range\" type=\"range\" min=\"0\" max=\"10\" value=\"5\" class=\"w-full h-2 rounded-lg appearance-none cursor-pointer bg-gray-700\">\n"
"          </div>\n"
"\n"
"          <!-- Axes control -->\n"
"          <div class=\"col-span-3\">\n"
"            <div id=\"axes-control-div\" class=\"grid grid-cols-5 gap-x-6\">\n"
"              <!-- Utilities -->\n"
"              <div>\n"
"                <button class=\"p-3 w-full bg-gray-600 hover:bg-gray-700 rounded-lg text-center\">Homing</button><br>\n"
"              </div>\n"
"              <!-- X -->\n"
"              <div class=\"p-2 border-2 border-dashed border-blue-400 rounded-lg\">\n"
"                <button id=\"Xadd-control-button\" class=\"axe-ctrl-but-add p-3 w-full bg-gray-600 hover:bg-gray-700 rounded-lg text-center\">X+</button><br>\n"
"                <p id=\"X-control-label\" class=\"text-center mt-2 mb-2\">0</p>\n"
"                <button id=\"Xsub-control-button\" class=\"axe-ctrl-but-sub p-3 w-full bg-gray-600 hover:bg-gray-700 rounded-lg text-center\">X-</button>\n"
"              </div>\n"
"            </div>\n"
"\n"
"            <br><div class=\"grid grid-cols-4 px-2 py-2 gap-x-6 border-2 border-dashed border-gray-600 rounded-lg\">\n"
"              <label for=\"\" class=\"p-1\">Step :</label>\n"
"              <button onclick=\"step = 0.1;\" class=\"p-1 w-full bg-gray-600 hover:bg-gray-700 rounded-lg text-center\">0.1</button>\n"
"              <button onclick=\"step = 1;\" class=\"p-1 w-full bg-gray-600 hover:bg-gray-700 rounded-lg text-center\">1</button>\n"
"              <button onclick=\"step = 10;\" class=\"p-1 w-full bg-gray-600 hover:bg-gray-700 rounded-lg text-center\">10</button>\n"
"            </div>\n"
"          </div>\n"
"        </div>\n"
"      </div>\n"
"    </div>\n"
"\n"
"    <script>\n"
"      var step = 0.1;\n"
"\n"
"      function send_post_request(obj)\n"
"      {\n"
"        fetch(\"data\", {\n"
"          method: \"POST\",\n"
"          body: JSON.stringify(obj),\n"
"          headers: {\n"
"            \"Content-type\": \"application/json; charset=UTF-8\"\n"
"          }\n"
"        })\n"
"          .then(response => response.json())\n"
"          .then(json => console.log(json)) \n"
"          .catch(err => console.error('Error:', err)); \n"
"      }\n"
"\n"
"      function handle_file_select(e) \n"
"      {\n"
"        let fl_files = document.getElementById('dropzone-file').files;\n"
"        if (fl_files.length === 0) {\n"
"          console.warn(\"No file selected.\");\n"
"          return;\n"
"        }\n"
"\n"
"        let fl_file = fl_files[0]\n"
"\n"
"        let reader = new FileReader();\n"
"        reader.onload = function(e) {\n"
"          send_post_request({ \"gcode-file\": e.target.result });\n"
"        };\n"
"\n"
"        reader.readAsText(fl_file);\n"
"\n"
"        e.preventDefault();\n"
"        return false;\n"
"      }\n"
"\n"
"      function console_chat_send(e)\n"
"      {\n"
"        let log = document.getElementById(\"console-log\");\n"
"        let input = document.getElementById(\"console-chat\");\n"
"\n"
"        if (input.value.length > 0)\n"
"        {\n"
"          // Send the command\n"
"          send_post_request({ \"gcode-cmd\": input.value.toUpperCase() });\n"
"          // Display command on the logs\n"
"          var val = log.value.replace(/^\\n+/,\"\");\n"
"          val += (new Date()).toLocaleTimeString() + \" > \" + input.value.toUpperCase() + \"\\n\";\n"
"          log.value = val;\n"
"\n"
"          var padding = [];\n"
"          while (log.clientHeight >= log.scrollHeight) {\n"
"            padding.push(\"\\n\");\n"
"            log.value = \"\\n\" + log.value;\n"
"          }\n"
"          padding.pop();\n"
"          log.value = padding.join(\"\") + val;\n"
"          log.scrollTop = log.scrollHeight;\n"
"          input.value = \"\";\n"
"        }\n"
"\n"
"        e.preventDefault();\n"
"        return false;\n"
"      }\n"
"\n"
"      function change_dropzone(e)\n"
"      {\n"
"        if (e.target.files.length == 0) return;\n"
"        document.getElementById(\"dropzone-filename-div\").innerHTML = \"<p>\" + e.target.files[0].name + \"</p>\"\n"
"      }\n"
"\n"
"      function populate_axes_control_div()\n"
"      {\n"
"        let div = document.getElementById(\"axes-control-div\");\n"
"        let template = div.children[1].outerHTML;\n"
"        let axe_names = [\"X\", \"Y\", \"Z\", \"E\"];\n"
"        for (let i = 0; i < 3; i++)\n"
"        {\n"
"          div.innerHTML += template.replace(/X/g, axe_names[i + 1]);\n"
"        }\n"
"\n"
"        div.childNodes.forEach(x => x.addEventListener(\"click\", control_axe));\n"
"      }\n"
"\n"
"      function control_axe(e)\n"
"      {\n"
"        let axe_name = e.target.id[0];\n"
"        let dir = 1;\n"
"\n"
"        if (e.target.className.includes(\"axe-ctrl-but-add\"))\n"
"        {\n"
"          dir = 1;\n"
"        }\n"
"        else\n"
"        {\n"
"          dir = -1;\n"
"        }\n"
"\n"
"        let pos = parseFloat(document.getElementById(axe_name + \"-control-label\").innerHTML);\n"
"        document.getElementById(axe_name + \"-control-label\").innerHTML = String((pos + dir * step).toFixed(1));\n"
"\n"
"        // Send the command\n"
"        let body = {  };\n"
"        body[axe_name.concat(\"_move_from\")] = (dir * step).toString();\n"
"        send_post_request(body);\n"
"      }\n"
"\n"
"      populate_axes_control_div();\n"
"\n"
"      document.getElementById(\"print-form\").addEventListener(\"submit\", handle_file_select);\n"
"\n"
"      document.getElementById(\"dropzone-file\").addEventListener(\"change\", change_dropzone);\n"
"\n"
"      document.getElementById(\"console-chat-form\").addEventListener(\"submit\", console_chat_send, false);\n"
"\n"
"    </script>\n"
"  </body>\n"
"\n"
"</html>";

  /* Start server */
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = SERVER_PORT;

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

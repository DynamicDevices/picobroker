
#ifndef __WIFI_H_
#define __WIFI_H_

#include "board_defs.h"

#ifdef CONFIG_SUPPORT_WIFI_AP
#undef SUPPORT_WIFI_AP
#define SUPPORT_WIFI_AP CONFIG_SUPPORT_WIFI_AP
#endif

#ifdef SUPPORT_WIFI_AP

#define ESP_AP_SSID "esp-ap"
#define ESP_AP_PASS "decafbad00"
#define ESP_AP_AUTH_MODE WIFI_AUTH_WPA2_PSK
#define ESP_AP_MAX_CONN 4
#define ESP_AP_CHANNEL 11

#ifdef CONFIG_ESP_AP_SSID
#undef ESP_AP_SSID
#define ESP_AP_SSID CONFIG_ESP_AP_SSID
#endif

#ifdef CONFIG_ESP_AP_PASS
#undef ESP_AP_PASS
#define ESP_AP_PASS CONFIG_ESP_AP_PASS
#endif

#endif // SUPPORT_WIFI_AP

#ifdef CONFIG_SUPPORT_WIFI_STA
#undef SUPPORT_WIFI_STA
#define SUPPORT_WIFI_STA CONFIG_SUPPORT_WIFI_STA
#endif

#ifdef SUPPORT_WIFI_STA

/* The examples use WiFi configuration that you can set via project configuration menu
   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define ESP_STA_SSID                "router"
#define ESP_STA_PASS                "decafbad00"
#define ESP_STA_MAXIMUM_RETRY       5

#ifdef CONFIG_ESP_STA_SSID
#undef ESP_STA_SSID
#define ESP_STA_SSID CONFIG_ESP_STA_SSID
#endif

#ifdef CONFIG_ESP_STA_PASS
#undef ESP_STA_PASS
#define ESP_STA_PASS CONFIG_ESP_STA_PASS
#endif

#ifdef CONFIG_ESP_STA_MAXIMUM_RETRY
#undef ESP_STA_MAXIMUM_RETRY
#define ESP_STA_MAXIMUM_RETRY CONFIG_ESP_STA_MAXIMUM_RETRY
#endif

#endif // SUPPORT_WIFI_STA

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

// Prototypes

void wifi_init(void);

#endif // __WIFI_H_

//
// Created by Andrew Simmons on 2021-03-04.
//

#include <esp_wifi.h>
#include <esp_now.h>
#include <WiFiType.h>
#include "ErrorUtil.h"

String ErrorUtil::getDescriptionFromWLStatus(wl_status_t error)
{
    switch(error)
    {
        case WL_IDLE_STATUS: return "Idle status";
        case WL_NO_SSID_AVAIL: return "No SSID available";
        case WL_SCAN_COMPLETED: return "Scan completed";
        case WL_CONNECTED: return "Connected";
        case WL_CONNECT_FAILED: return "Connection failed";
        case WL_CONNECTION_LOST:return "Connection lost";
        case WL_DISCONNECTED:return "Disconnected";
        default: return "Unknown";
    }
}


String ErrorUtil::getDescriptionFromESPError(esp_err_t error)
{
	switch(error)
	{

		case ESP_ERR_WIFI_NOT_INIT: return "WiFi driver was not installed by esp_wifi_init";
		case ESP_ERR_WIFI_NOT_STARTED: return "WiFi driver was not started by esp_wifi_start";
		case ESP_ERR_WIFI_NOT_STOPPED: return "WiFi driver was not stopped by esp_wifi_stop";
		case ESP_ERR_WIFI_IF: return "WiFi interface error";
		case ESP_ERR_WIFI_MODE: return "WiFi mode error";
		case ESP_ERR_WIFI_STATE: return "WiFi internal state error";
		case ESP_ERR_WIFI_CONN: return "WiFi internal control block of station or soft-AP error";
		case ESP_ERR_WIFI_NVS: return "WiFi internal NVS module error";
		case ESP_ERR_WIFI_MAC: return "MAC address is invalid";
		case ESP_ERR_WIFI_SSID: return "SSID is invalid";
		case ESP_ERR_WIFI_PASSWORD: return "Password is invalid";
		case ESP_ERR_WIFI_TIMEOUT: return "Timeout error";
		case ESP_ERR_WIFI_WAKE_FAIL: return "WiFi is in sleep state(RF closed) and wakeup fail";
		case ESP_ERR_WIFI_WOULD_BLOCK: return "The caller would block";
		case ESP_ERR_WIFI_NOT_CONNECT: return "Station still in disconnect status";
		case ESP_ERR_WIFI_POST: return "Failed to post the event to WiFi task";
		case ESP_ERR_WIFI_INIT_STATE: return "Invalod WiFi state when init/deinit is called";
		case ESP_ERR_WIFI_STOP_STATE: return "Returned when WiFi is stopping";


		case ESP_OK: return "OK!";
		case ESP_FAIL: return "General failure";
		case ESP_ERR_NO_MEM: return "Out of memory";
		case ESP_ERR_INVALID_ARG: return "Invalid argument";
		case ESP_ERR_INVALID_STATE: return "Invalid state";
		case ESP_ERR_INVALID_SIZE: return "Invalid size";
		case ESP_ERR_NOT_FOUND: return "Requested resource not found";
		case ESP_ERR_NOT_SUPPORTED: return "Operation or feature not supported";
		case ESP_ERR_TIMEOUT: return "Operation timed out";
		case ESP_ERR_INVALID_RESPONSE: return "Received response was invalid";
		case ESP_ERR_INVALID_CRC: return "CRC or checksum was invalid";
		case ESP_ERR_INVALID_VERSION: return "Version was invalid";
		case ESP_ERR_INVALID_MAC: return "MAC address was invalid";
		
		case ESP_ERR_ESPNOW_BASE         : return "ESPNOW error number base.";
		case ESP_ERR_ESPNOW_NOT_INIT     : return "ESPNOW is not initialized.";
		case ESP_ERR_ESPNOW_ARG          : return "Invalid argument";
		case ESP_ERR_ESPNOW_NO_MEM       : return "Out of memory";
		case ESP_ERR_ESPNOW_FULL         : return "ESPNOW peer list is full";
		case ESP_ERR_ESPNOW_NOT_FOUND    : return "ESPNOW peer is not found";
		case ESP_ERR_ESPNOW_INTERNAL     : return "Internal error";
		case ESP_ERR_ESPNOW_EXIST        : return "ESPNOW peer has existed";
		case ESP_ERR_ESPNOW_IF           : return "Interface error";
		case ESP_NOW_ETH_ALEN            : return "Length of ESPNOW peer MAC address";
		case ESP_NOW_KEY_LEN             : return "Length of ESPNOW peer local master key";
		case ESP_NOW_MAX_TOTAL_PEER_NUM  : return "Maximum number of ESPNOW total peers";
		case ESP_NOW_MAX_DATA_LEN        : return "Maximum length of ESPNOW data which is sent very time";
		
		default: return "Unknown";
	}
}

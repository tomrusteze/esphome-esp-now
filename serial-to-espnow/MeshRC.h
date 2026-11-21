//
// Author: Phong Vu
// Modified by Tom Rust for use with ESPhome
// This version is slightly modified for use with Arduino
//
#ifndef __MESH_RC_H__
#define __MESH_RC_H__

#include <ESP8266WiFi.h>

extern "C" {
#include <c_types.h>
#include <espnow.h>
#include <user_interface.h>
}

namespace MeshRC
{

	typedef std::function<void(void)> esp_rc_callback_t;
	typedef std::function<void(uint8_t *, uint8_t)> esp_rc_data_callback_t;

	struct esp_rc_event_t
	{
		String prefix;
		esp_rc_callback_t callback;
		esp_rc_data_callback_t callback2;
	} events[250];

	uint8_t buffer[250];
	uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	uint8_t events_num = 0;
	volatile uint32_t sent_error = 0;
	uint32_t rejected = 0;
	uint32_t received;
	uint32_t ignored;
	uint8_t *sender;
	uint8_t *master = NULL;
	bool esp_now_is_init = false;
	uint8_t psk[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 'a', 'b', 'c', 'd', 'e', 'f'};

	void setupwifi(int channel)
	{
		WiFi.mode(WIFI_STA);
		wifi_set_channel(channel);
	}

	void send(uint8_t *dest, const String &data)
	{
		if (!esp_now_is_init || data.length() == 0 || data.length() > sizeof(buffer))
		{
			rejected++;
			return;
		}
		if (!esp_now_is_peer_exist(dest) &&
		    esp_now_add_peer(dest, ESP_NOW_ROLE_COMBO, wifi_get_channel(), nullptr, 0) != 0)
		{
			rejected++;
			return;
		}
		memcpy(buffer, data.c_str(), data.length());
		if (esp_now_send(dest, buffer, data.length()) != 0)
			rejected++;
	}

	void getValue(String d)
	{
		d = "";
		for (auto i = 0; i < 250; i++)
			d.concat((const char)buffer[i]);
	}
	void on(String prefix, esp_rc_callback_t callback)
	{
		events[events_num++] = (esp_rc_event_t){prefix, callback, NULL};
	}
	void on(String prefix, esp_rc_data_callback_t callback)
	{
		events[events_num++] = (esp_rc_event_t){prefix, NULL, callback};
	}
	void delayMs(uint32_t time)
	{
		uint32_t delayUntil = millis() + time;
		while (millis() < delayUntil)
			yield();
	}
	bool equals(const uint8_t *a, const uint8_t *b, uint8_t size, uint8_t offset = 0)
	{
		for (auto i = offset; i < offset + size; i++)
			if (a[i] != b[i])
				return false;
		return true;
	}
	bool equals(uint8_t *a, uint8_t *b, uint8_t size, uint8_t offset = 0)
	{
		for (auto i = offset; i < offset + size; i++)
			if (a[i] != b[i])
				return false;
		return true;
	}

	esp_now_send_cb_t sendHandler = [](uint8_t *addr, uint8_t sendStatus)
	{
		if (sendStatus == 1)
		{
			sent_error = sent_error + 1;
		}
	};

	esp_now_recv_cb_t recvHandler = [](uint8_t *addr, uint8_t *data, uint8_t size)
	{
		static uint8_t offset, i;
		// Only receives from master if set
		if (addr == NULL || master == NULL || equals(addr, master, 6))
		{
			received++;
			sender = addr;
			for (i = 0; i < events_num; i++)
			{
				offset = events[i].prefix.length();
				if (equals(data, (uint8_t *)events[i].prefix.c_str(), offset))
				{
					if (events[i].callback)
						events[i].callback();
					if (events[i].callback2)
						events[i].callback2(&data[offset], size - offset);
				}
			}
		}
		else
		{
			ignored++;
		}
	};

	void begin()
	{
		if (esp_now_init() == OK)
		{
			esp_now_is_init = true;
			if (esp_now_is_peer_exist(broadcast))
			{
				esp_now_del_peer(broadcast);
			}
			esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
			esp_now_add_peer(broadcast, ESP_NOW_ROLE_COMBO, wifi_get_channel(), nullptr, 0);
			esp_now_register_send_cb(sendHandler);
			esp_now_register_recv_cb(recvHandler);
		}
	}

} // namespace MeshRC

#endif //__MESH_RC_H__

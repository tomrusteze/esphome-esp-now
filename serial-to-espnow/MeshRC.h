//
// Author: Phong Vu
// Modified by Tom Rust for use with ESPhome
// This version is slightly modified for use with Arduino
//
#ifndef __MESH_RC_H__
#define __MESH_RC_H__

#include <espnow.h>
#include <ESP8266WiFi.h>

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
	uint32_t sent_error = 0;
	uint32_t received;
	uint32_t ignored;
	uint8_t *sender;
	uint32_t sendTime;
	uint16_t duration;
	uint8_t *master = NULL;
	bool sending;
	bool esp_now_is_init = false;
	uint8_t psk[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 'a', 'b', 'c', 'd', 'e', 'f'};

	void setupwifi(int channel)
	{
		// Use this function when you want the node to only communicate over esp-now.
		WiFi.mode(WIFI_AP_STA);
		WiFi.softAP("temp", "temppass", channel, true, 0);
		WiFi.mode(WIFI_STA);
		WiFi.channel(channel);
	}

	void send(uint8_t *data, uint8_t size, uint8_t *dest)
	{
		// Send to a specific node
		sending = true;
		sendTime = micros();
		if (esp_now_is_init)
		{
			esp_now_send(dest ? dest : broadcast, data, size);
		}
	}

	void send(uint8_t *dest, String data)
	{
		memcpy(&buffer[0], (uint8_t *)data.c_str(), data.length());
		send(buffer, data.length(), dest);
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
	void wait()
	{
		while (MeshRC::sending)
			yield(); // Wait while sending
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

	void setAddr(uint8_t *addr)
	{
		if (esp_now_is_peer_exist(addr))
			esp_now_del_peer(addr);
		esp_now_add_peer(addr, ESP_NOW_ROLE_COMBO, 1, 0, 0);
	}

	esp_now_send_cb_t sendHandler = [](uint8_t *addr, uint8_t sendStatus)
	{
		if (sendStatus == 1)
		{
			sent_error++;
		}
		sending = false;
		duration = micros() - sendTime;
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
			esp_now_add_peer(broadcast, ESP_NOW_ROLE_COMBO, 1, 0, 0);
			esp_now_register_send_cb(sendHandler);
			esp_now_register_recv_cb(recvHandler);
		}
	}

} // namespace MeshRC

#endif //__MESH_RC_H__

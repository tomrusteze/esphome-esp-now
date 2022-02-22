//
// Author: Phong Vu
// Modified by Tom Rust
//
#ifndef __MESH_RC_H__
#define __MESH_RC_H__

#ifdef USE_ESP32
#include <esp_now.h>
#include <WiFi.h>
#endif
#ifdef USE_ESP8266
#include <espnow.h>
#endif


namespace MeshRC {

typedef std::function<void(void)> esp_rc_callback_t;
#ifdef USE_ESP32
typedef std::function<void(const uint8_t *, int)> esp_rc_data_callback_t;
#endif
#ifdef USE_ESP8266
typedef std::function<void(uint8_t *, uint8_t)> esp_rc_data_callback_t;
#endif

struct esp_rc_event_t {
	String prefix;
	esp_rc_callback_t callback;
	esp_rc_data_callback_t callback2;
} events[250];

uint8_t buffer[250];
uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t events_num = 0;
uint32_t received;
uint32_t ignored;
uint8_t *sender;
uint32_t sendTime;
uint16_t duration;
uint8_t *master = NULL;
bool sending;
uint8_t psk[] = {'1','2','3','4','5','6','7','8','9','0','a','b','c','d','e','f'};
/*
void setMaster(uint8_t *addr) {
	if (esp_now_is_peer_exist(master))
		esp_now_del_peer(master);
	master = addr;
	esp_now_add_peer(master, ESP_NOW_ROLE_COMBO, 1, psk, sizeof(psk));
}
*/
void send(uint8_t *data, uint8_t size) {
	sending = true;
	sendTime = micros();
	esp_now_send(master ? master : broadcast, data, size);
}
void send(String data) {
	send((uint8_t *)data.c_str(), data.length());
}
void send(String data, std::string value) {
	send(String(data + value.c_str()));
}
void send(uint8_t *data, uint8_t size, String type) {
	memcpy(&buffer[0], data, size);
	memcpy(&buffer[size], (uint8_t *)type.c_str(), type.length());
	send(buffer, type.length() + size);
}
void send(String type, uint8_t *data, uint8_t size) {
	memcpy(&buffer[0], (uint8_t *)type.c_str(), type.length());
	memcpy(&buffer[type.length()], data, size);
	send(buffer, type.length() + size);
}
void send(uint8_t *data, uint8_t size, std::string value) {
	String type = String(value.c_str());
	send(data,size,type);
}
void getValue(String d) {
		d = "";
		for (auto i=0; i<250; i++) d.concat((const char)buffer[i]);
	}
void on(String prefix, esp_rc_callback_t callback) {
	events[events_num++] = (esp_rc_event_t){prefix, callback, NULL};
}
void on(String prefix, esp_rc_data_callback_t callback) {
	events[events_num++] = (esp_rc_event_t){prefix, NULL, callback};
}
void wait() {
	while (MeshRC::sending) yield();  // Wait while sending
}
void delayMs(uint32_t time) {
	uint32_t delayUntil = millis() + time;
	while (millis() < delayUntil) yield();
}
bool equals(const uint8_t *a, const uint8_t *b, uint8_t size, uint8_t offset = 0) {
	for (auto i = offset; i < offset + size; i++)
		if (a[i] != b[i])
			return false;
	return true;
}
bool equals(uint8_t *a, uint8_t *b, uint8_t size, uint8_t offset = 0) {
	for (auto i = offset; i < offset + size; i++)
		if (a[i] != b[i])
			return false;
	return true;
}

#ifdef USE_ESP32
void sendHandler(const uint8_t *addr, esp_now_send_status_t err) {
	sending = false;
	duration = micros() - sendTime;
}

void recvHandler(const uint8_t *addr, const uint8_t *data, int size) {
	static uint8_t offset, i;
	// Only receives from master if set
	if (addr == NULL || master == NULL || equals(addr, master, 6)) {
		received++;
		sender = (uint8_t*) addr;
		for (i = 0; i < events_num; i++) {
			offset = events[i].prefix.length();
			if (equals(data, (uint8_t *)events[i].prefix.c_str(), offset)) {
				if (events[i].callback) events[i].callback();
				if (events[i].callback2) events[i].callback2(&data[offset],  size - offset);
			}
		}
	} else {
		ignored++;
	}
}

void begin() {
	if (esp_now_init() == ESP_OK) {
		if (esp_now_is_peer_exist(broadcast)) {
			esp_now_del_peer(broadcast);
		}
		esp_now_peer_info_t peerInfo;
		peerInfo.channel = 0;
		peerInfo.encrypt = false;
		memcpy(peerInfo.peer_addr, broadcast, 6);
		esp_now_add_peer(&peerInfo);
		esp_now_register_send_cb(sendHandler);
		esp_now_register_recv_cb(recvHandler);
	}
}
#endif
#ifdef USE_ESP8266
esp_now_send_cb_t sendHandler = [](uint8_t *addr, uint8_t err) {
	sending = false;
	duration = micros() - sendTime;
};
esp_now_recv_cb_t recvHandler = [](uint8_t *addr, uint8_t *data, uint8_t size) {
	static uint8_t offset, i;
	// Only receives from master if set
	if (addr == NULL || master == NULL || equals(addr, master, 6)) {
		received++;
		sender = addr;
		for (i = 0; i < events_num; i++) {
			offset = events[i].prefix.length();
			if (equals(data, (uint8_t *)events[i].prefix.c_str(), offset)) {
				if (events[i].callback) events[i].callback();
				if (events[i].callback2) events[i].callback2(&data[offset], size - offset);
			}
		}
	} else {
		ignored++;
	}
};
void begin() {
	if (esp_now_init() == OK) {
		if (esp_now_is_peer_exist(broadcast)) {
			esp_now_del_peer(broadcast);
		}
		esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
		esp_now_add_peer(broadcast, ESP_NOW_ROLE_COMBO, 1, 0, 0);
		esp_now_register_send_cb(sendHandler);
		esp_now_register_recv_cb(recvHandler);
	}
}
#endif
}  // namespace MeshRC

#endif	//__MESH_RC_H__

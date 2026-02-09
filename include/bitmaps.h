#ifndef BITMAPS_H
#define BITMAPS_H

#include <Arduino.h>

// Width and Height of Status Icons
#define ICON_WIDTH 10
#define ICON_HEIGHT 10

// ==========================================
// STATUS BAR ICONS (only bitmaps left)
// ==========================================

// WiFi Connected (Signal Waves) 10x10
const unsigned char wifi_connected_bits[] PROGMEM = {
  0x00, 0x00, 0x7C, 0x00, 0xFE, 0x00, 0x83, 0x01, 0x39, 0x01, 0x7C, 0x00, 0xC6, 0x00, 0x28, 0x00,
  0x38, 0x00, 0x10, 0x00 };

// WiFi Disconnected (Crossed Out X) 10x10
const unsigned char wifi_disconnected_bits[] PROGMEM = {
  0x01, 0x01, 0x82, 0x00, 0x44, 0x00, 0x28, 0x00, 0x10, 0x00, 0x28, 0x00, 0x44, 0x00, 0x82, 0x00,
  0x01, 0x01, 0x00, 0x00 };

// ==========================================
// Face types (drawn with primitives in main.cpp)
// ==========================================
#define FACE_HAPPY       0
#define FACE_THIRSTY     1
#define FACE_OVERWATERED 2
#define FACE_HOT         3
#define FACE_COLD        4
#define FACE_DARK        5
#define FACE_BRIGHT      6
#define FACE_HUMID       7
#define FACE_DRY_AIR     8

#endif

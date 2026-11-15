#pragma once

#include "machine/keyboard.hpp"

#include <array>

// Held state of every calculator key, indexed like Keyboard::KEYS, with the ON key at ON_KEY.
using KeyStates = std::array<bool, Keyboard::NUM_KEYS + 1>;
constexpr int ON_KEY = Keyboard::NUM_KEYS;

// Draws the keypad window and returns the held state of every key.
KeyStates drawKeypad(const Keyboard &kb);

// Sends changed key states to the emulated keyboard.
void applyKeyStates(Keyboard &kb, const KeyStates &held);

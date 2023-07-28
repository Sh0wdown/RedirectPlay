#pragma once

struct SSteamServerSettings;
struct HWND__;
using HWND = HWND__*;

HWND GetMainWindow();
bool ShowServerSettings(SSteamServerSettings& settings);
bool ShowPasswordRequest(char* szPassword, size_t maxLen);
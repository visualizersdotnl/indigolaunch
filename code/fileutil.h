
#pragma once

bool FileExists(const std::wstring &path);
uint8_t *ReadFile(const std::wstring &path, bool checkForTrailingZero = false);

#pragma once
#include <string>
#include <Windows.h>

namespace StringUtility {
// stringをwstringに変換
std::wstring ConvertString(const std::string &str);
// wstringをstringに変換
std::string ConvertString(const std::wstring &str);

} // namespace StringUtility

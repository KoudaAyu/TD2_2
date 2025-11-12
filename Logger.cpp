#include "Logger.h"
#include <Windows.h>
#include <iostream>

namespace Logger {
void Log(const std::string &message) {
  std::cout << message << std::endl;  
  OutputDebugStringA(message.c_str()); // 出力ウィンドウに文字を出力
}
} // namespace Logger
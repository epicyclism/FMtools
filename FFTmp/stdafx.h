// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <tchar.h>
#include <windows.h>

#define _USE_MATH_DEFINES
#define _SCL_SECURE_NO_WARNINGS
#define _SECURE_SCL 0

#include <iostream>
#include <memory>
#include <algorithm>
#include <functional>
#include <cmath>
#include <complex>
#include <vector>

#if defined (UNICODE)
#define COUT std::wcout
#define CERR std::wcerr
#else
#define COUT std::cout
#define CERR std::cerr
#endif
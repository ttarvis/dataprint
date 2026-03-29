// Copyright (c) 2026 Terence Tarvis
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <iostream>

#ifdef DEBUG
	#define DEBUG_LOG(fmt, ...) \
		std::cerr << "[DEBUG] " <<  __VA_ARGS__ << std::endl;
	#define PRINT_HEX64(a, b) \
		std::cerr << std::hex << std::setfill('0') \
		<< std::setw(16) << (a) \
		<< std::setw(16) << (b) \
		<< std::dec << std::endl
#else
  #define DEBUG_LOG(fmt, ...) ((void)0)
#endif


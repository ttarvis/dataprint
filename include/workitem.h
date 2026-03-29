// Copyright (c) 2026 Terence Tarvis
// SPDX-License-Identifier: Apache-2.0

#ifndef WORKITEM_H
#define WORKITEM_H

// work_item.h
#include <simdjson.h>
#include <vector>

struct WorkItem {
	simdjson::padded_string chunk;
};
#endif

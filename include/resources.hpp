#pragma once

#include <windows.h>
#include <cstdint>

extern "C"
{
#include "resource_def.h"
}

class Resource
{
private:
	int32_t m_tag = -1;
	void*	m_data = nullptr;
	int32_t m_size = -1;

public:	
	Resource(int32_t tag);
	bool	load();
	void*	data();
	int32_t size();
};

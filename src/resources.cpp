#include <cassert>
#include <windows.h>

#include "resources.hpp"
#include "globals.hpp"


Resource::Resource(int32_t tag)
{
	this->m_tag = tag;
}

bool Resource::load()
{
	assert(this->m_data == nullptr && "resource already loaded");
	assert(g_hInstance != nullptr && "uninitialized hInstance");

	HRSRC hRsrc = FindResource(g_hInstance,
		MAKEINTRESOURCE(this->m_tag),
		RT_RCDATA);
	if (!hRsrc) return false;

	HGLOBAL hGlob = ::LoadResource(g_hInstance, hRsrc);
	if (!hGlob) return false;

	this->m_data = ::LockResource(hGlob);
	this->m_size = ::SizeofResource(g_hInstance, hRsrc);
	
	return true;
}

void* Resource::data()
{
	assert(this->m_data != nullptr && "resource not loaded");
	return this->m_data;
}

int32_t Resource::size()
{
	assert(this->m_data != nullptr && "resource not loaded");
	return this->m_size;
}

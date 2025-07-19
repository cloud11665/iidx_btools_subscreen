#pragma once

#include <cstdint>
#include <vector>
#include <string>


namespace aic
{
	void init();
	void readerThread(size_t deviceIndex);
	std::vector<std::string> listCardIOPaths();

	bool icontains(const std::string& hay, const char* needle);
	bool isDesiredCardIO(const std::string& path);
	HANDLE openCardIOHandle(size_t deviceIndex, std::string& outPath);
}

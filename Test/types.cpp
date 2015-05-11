#include "types.h"
#include <boost/filesystem.hpp>

std::string GetNativeAppPath()
{
	char	path[MAX_PATH];
	WCHAR wpath[MAX_PATH];

	HMODULE hModule = GetModuleHandle(NULL);
	GetModuleFileName(hModule, wpath, MAX_PATH);

	boost::filesystem::path		xmlpath(wpath, boost::filesystem::native);

	xmlpath.remove_filename();
	std::wstring strWPath = xmlpath.native();

	wcstombs(path, strWPath.c_str(), MAX_PATH - 1);

	return path;
}
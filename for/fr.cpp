#include "fr.hpp"
#include <filesystem>
#include <stdexcept>
#include <string.h>

namespace fr {

size_t file_size(const char *path)
{
	return std::filesystem::file_size(path);
}

void throw_runtime_error(const char *str)
{
	throw exception(str);
}

exception::exception(const char *str)
{
	m_buf = strdup(str);
}
exception::~exception(void)
{
	std::free(m_buf);
}

const char* exception::what(void) const
{
	return m_buf;
}

}
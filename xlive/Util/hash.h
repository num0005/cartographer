#pragma once
#include <string>

namespace hashes
{
	std::string as_hex_string(const uint8_t *data, size_t data_len);

	template <typename T>
	inline std::string as_hex_string(const T *data, size_t count)
	{
		return as_hex_string(reinterpret_cast<const uint8_t*>(data), count * sizeof(T));
	}

	template <typename T, size_t count>
	inline std::string as_hex_string(const T (&data)[count])
	{
		return as_hex_string(data, count);
	}

	bool calc_file_md5(const std::string &filename, BYTE *checksum, size_t &checksum_len, long long count = 0);
	bool calc_file_md5(const std::string &filename, std::string &checksum_out, long long count = 0);

	bool calc_file_md5(const std::wstring &filename, BYTE *checksum, size_t &checksum_len, long long count = 0);
	bool calc_file_md5(const std::wstring &filename, std::string &checksum_out, long long count = 0);
};
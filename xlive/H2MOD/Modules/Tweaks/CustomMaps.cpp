#include "stdafx.h"
#include "CustomMaps.h"
#include "H2MOD.h"
#include <experimental/filesystem>
#include <chrono>

bool __cdecl get_path_by_id(int path_id, LPCWSTR sub_path, LPWSTR out_path, bool a4)
{
	typedef char __cdecl get_path_by_id(int path_id, LPCWSTR pMore, LPWSTR pszPath, bool a4);
	auto get_path_by_id_impl = reinterpret_cast<get_path_by_id*>(h2mod->GetAddress(0x8EF9E));
	return get_path_by_id_impl(path_id, sub_path, out_path, a4);
}

bool __cdecl get_map_hash(const wchar_t *file, uint8_t hash[32])
{
	typedef int __cdecl get_map_hash__warbird(const wchar_t *file, uint8_t *hash);
	auto get_map_hash__warbird__impl = h2mod->GetAddress<get_map_hash__warbird*>(0x8F9CB);
	return get_map_hash__warbird__impl(file, hash);
}

extern int __cdecl validate_and_add_custom_map(custom_map_info* data);

namespace fs = std::experimental::filesystem;
using time_clock = std::chrono::system_clock;
using sec = std::chrono::duration<double>;
void CustomMapSet::scan_maps_folder(std::atomic_flag *done, bool is_inital)
{
	const auto start = time_clock::now();
	wchar_t custom_maps_path[MAX_PATH];
	if (!LOG_CHECK(get_path_by_id(4, L"", custom_maps_path, 1)) ||
			!LOG_CHECK(PathFileExistsW(custom_maps_path)))
	{
		LOG_CRITICAL_FUNC("Failed to get custom maps path, aborting");
		scan_in_progress.store(false);
		return;
	}

	if (!is_inital) {
		/*
			We store the maps to update in a vector so we don't have to read the new data while inside a maps mutex
		*/
		typedef std::pair<wchar_t[256], uint8_t[0x20]> update_info;
		std::vector<update_info> to_update;
		{
			const std::lock_guard <std::recursive_mutex> map_lock(this->maps_mutex);
			LOG_INFO_FUNCW("Detecting removed and modified maps");
			for (auto& map : maps) {
				if (!fs::is_regular_file(map.map_file_name)) {
					LOG_INFO_FUNCW("Marked {} for removal", map.map_file_name);
					map.is_marked_for_removal = 1;
					continue;
				} else {
					uint8_t hash[0x20];
					if (!LOG_CHECK(get_map_hash(map.map_file_name, hash))) { // should never fail...
						continue;
					}
					if (map.hash != hash) {
						update_info info;
						memcpy_s(info.first, sizeof(info.first), map.map_file_name, sizeof(map.map_file_name));
						memcpy_s(info.second, sizeof(info.second), hash, sizeof(hash));
						to_update.push_back(info);
					}
				}
			}
		}
		for (const auto& entry : to_update)
		{
			custom_map_info new_map = {};
			memcpy_s(new_map.hash, sizeof(new_map.hash), entry.second, sizeof(entry.second));
			memcpy_s(new_map.map_file_name, sizeof(new_map.map_file_name), entry.first, sizeof(entry.first));
			if (!validate_and_add_custom_map(&new_map)) {
				LOG_INFO_FUNCW("Skipping map (Couldn't get map data) \"{}\"", new_map.map_file_name);
				continue;
			}
			{
				LOG_INFO_FUNCW("Queuing map data for update \"{}\"", new_map.map_file_name);
				const std::lock_guard <std::mutex> map_lock(this->update_mutex);
				this->maps_to_update.push_back(new_map);
			}
		}
	}

	for (auto& entry : fs::directory_iterator(custom_maps_path)) {
		auto path = entry.path();
		if (fs::is_regular_file(path) && path.extension() == ".map")
		{
			auto map_path_wide = path.generic_wstring();
			if (get_map_by_path(map_path_wide.c_str())) {
				LOG_DEBUG_FUNCW("Skipping existing map");
				continue;
			}
			uint8_t hash[0x20];
			if (!get_map_hash(map_path_wide.c_str(), hash)) {
				LOG_ERROR_FUNCW("Skipping map (Couldn't get hash for map) \"{}\"", map_path_wide);
				continue;
			}
			if (get_map_by_hash(hash)) {
				LOG_INFO_FUNCW("Skipping map (Duplicate) \"{}\"", map_path_wide);
				continue;
			}
			custom_map_info new_map = {};
			memcpy_s(new_map.hash, sizeof(new_map.hash), hash, sizeof(hash));
			wcsncpy_s(new_map.map_file_name, map_path_wide.c_str(), map_path_wide.length());
			if (!validate_and_add_custom_map(&new_map)) {
				LOG_DEBUG_FUNCW("Skipping map (Couldn't get map data) \"{}\"", map_path_wide);
				continue;
			}
			{
				LOG_DEBUG_FUNCW("Adding map \"{}\"", map_path_wide);
				const std::lock_guard <std::recursive_mutex> map_lock(this->maps_mutex);
				this->maps.push_back(new_map);
			}
		}
	}

	scan_in_progress.store(false);

	const sec duration = time_clock::now() - start;
	const size_t map_count = this->map_count();
	LOG_INFO_FUNC("Map scan complete! {} maps in {} seconds (or {} m/s)", map_count, duration.count(), map_count / duration.count());
	return;
}

void CustomMapSet::scan_maps_inital()
{
	if (!LOG_CHECK(scan_in_progress.load() == false)) {
		LOG_ERROR_FUNC("Init called more than once!!");
		return;
	}
	scan_in_progress.store(true);
	std::thread(&CustomMapSet::scan_maps_folder, this, nullptr, true).detach();
}

bool CustomMapSet::scan_maps_async(std::atomic_flag *done)
{
	if (scan_in_progress.load()) {
		return false;
	}
	scan_in_progress.store(true);
	std::thread(&CustomMapSet::scan_maps_folder, this, done, false).detach();
	return true;
}

#pragma once
#include "stdafx.h"

struct custom_map_info
{
	uint8_t hash[32];
	wchar_t map_name[32];
	wchar_t description[9][128];
	void *map_image;
	void *map_image_ui; // no idea why there are two...
	uint8_t max_teams[16];
	__int64 index;
	wchar_t map_file_name[256];
	FILETIME update_time;
	char is_marked_for_removal;
	char pad[0x3];
	int field_B8C;
};
static_assert(sizeof(custom_map_info) == 0xB90, "bad struct size");

struct map_info_ui
{
	wchar_t name[32];
	uint8_t hash[32];

	map_info_ui(wchar_t _name[32], uint8_t _hash[32])
	{
		memcpy(name, _name, sizeof(name));
		memcpy(hash, _hash, sizeof(hash));
	}
};

class CustomMapSet
{
	CustomMapSet() = default;
public:
	static CustomMapSet& GetSingleton()
	{
		static CustomMapSet singleton;
		return singleton;
	}
	/*
		Disable copy
	*/
	CustomMapSet(CustomMapSet const&) = delete;
	void operator=(CustomMapSet const&) = delete;
	/*
		Disable move
	*/
	CustomMapSet(CustomMapSet const&&) = delete;
	void operator=(CustomMapSet const&&) = delete;

	/*
		Start scanning the maps folder on a seperate thread
	*/
	bool scan_maps_async(std::atomic_flag *done = nullptr);

	/*
		Init and do initial scan
	*/
	void scan_maps_inital();

	inline custom_map_info *get_map_by_hash(const uint8_t hash[32])
	{
		const std::lock_guard <std::recursive_mutex> lock(this->maps_mutex);
		for (auto &map : maps)
		{
			if (memcmp(hash, map.hash, 32) == 0 && !map.is_marked_for_removal)
				return &map;
		}
		return nullptr;
	}

	inline custom_map_info *get_map_by_path(const wchar_t *path)
	{
		const std::lock_guard <std::recursive_mutex> lock(this->maps_mutex);
		for (auto& map : maps)
		{
			if (wcsncmp(map.map_file_name, path, 256) == 0 && !map.is_marked_for_removal)
				return &map;
		}
		return nullptr;
	}

	/* Remove map by path, note it is the callers responsiblity to ensure there aren't any dangling pointers to the map */
	inline void remove_map_by_path(const wchar_t *path)
	{
		const std::lock_guard <std::recursive_mutex> lock(this->maps_mutex);
		for (auto i = maps.begin(); i != maps.end();) {
			if (_wcsnicmp(i->map_file_name, path, 256) == 0)
				i = maps.erase(i);
			else
				++i;
		}
	}

	inline std::vector<map_info_ui> get_ui_map_info()
	{
		const std::lock_guard <std::recursive_mutex> lock(this->maps_mutex);

		std::vector<map_info_ui> ui_info;
		for (auto& map : maps)
			ui_info.push_back({map.map_name, map.hash});
		return ui_info;
	}

	inline size_t map_count()
	{
		return maps.size();
	}

	/* Call from the main loop */
	inline void update()
	{
		const std::lock_guard <std::recursive_mutex> map_lock(this->maps_mutex);
		for (auto i = maps.begin(); i != maps.end();) {
			if (i->is_marked_for_removal)
				i = maps.erase(i);
			else
				++i;
		}
		const std::lock_guard <std::mutex> update_lock(this->update_mutex);
		for (const auto& entry : maps_to_update) {
			LOG_INFO_FUNCW("Updating map data {}", entry.map_file_name);
			remove_map_by_path(entry.map_file_name);
			maps.push_back(entry);
		}
		maps_to_update.clear();
	}

private:
	/*
		Implementation function for map scanning
	*/
	void scan_maps_folder(std::atomic_flag *done, bool is_inital);

	std::list<custom_map_info> maps;
	std::recursive_mutex maps_mutex;

	std::atomic<bool> scan_in_progress;

	std::mutex update_mutex;
	std::vector<custom_map_info> maps_to_update;
};


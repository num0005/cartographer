#pragma once
//#pragma pack(1)

#include "BlamBaseTypes.h"

#define NONE -1

struct datum
{
	short index;
	short salt;
};
CHECK_STRUCT_SIZE(datum, 4);

struct tag_ref
{
	int tag_type;
	int tag_index;
};

struct tag_block_ref
{
	size_t size;
	size_t data;
};

template<typename T>
struct tag_block
{
	size_t size;
	size_t data;

	tag_block_ref* get_ref() {
		return reinterpret_cast<tag_block_ref*>(this);
	}
	tag_block_ref *operator&()
	{
		return get_ref();
	}

	T *begin()
	{
		if (this->data != NONE)
		{
			BYTE *tag_data_table = *reinterpret_cast<BYTE**>(0x882290);
			if (LOG_CHECK(tag_data_table))
				return reinterpret_cast<T*>(&tag_data_table[this->data]);
		}
		return nullptr;
	}

	T *end()
	{
		if (this->begin())
			return &this->begin()[this->size];
		else
			return nullptr;
	}

	T *operator[](size_t index)
	{
		if (index == NONE)
			return nullptr;
		if (index >= this->size)
			return nullptr;
		if (this->begin()) {
			T *data_array = this->begin();
			return &data_array[index];
		}
		return nullptr;
	}
};

struct byte_ref
{
	int size;
	void *address;
};
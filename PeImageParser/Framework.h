// ################################################################################################### //

#pragma once

// ################################################################################################### //

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ################################################################################################### //

typedef unsigned int uint32;

// ################################################################################################### //

#define ZERO_STRUCT(s)					memset(&(s), 0, sizeof(s))
#define COPY_STRUCT_BY_PTR(dst, src)	memcpy((dst), (src), sizeof(*(dst)))
#define FLAG(value, flag)				((value) & (flag))
#define ALIGN_UP(value, alignment)		(((value) + (alignment) - 1) & ~((alignment) - 1))
#define BEETWEEN_IE(value, low, high)	((value) >= (low) && (value) < (high))

// ################################################################################################### //

#define LOG(fmt, ...)					printf(fmt "\n", __VA_ARGS__)
#define INFO(fmt, ...)					printf("[INFO] " fmt "\n", __VA_ARGS__)
#define FAIL(fmt, ...)					printf("[FAIL] " fmt "\n", __VA_ARGS__)
#define UNEXPECTED(fmt, ...)			printf("[!] " fmt "\n", __VA_ARGS__)
#define FAIL_ARGS()						printf("[FAIL] Invalid arguments\n")
#define CENTERED(fmt, ...)				printf("\n========== " fmt " ==========\n", __VA_ARGS__)
#define TRACE()
#define HEX								"%p"

// ################################################################################################### //

inline void* operator new(size_t, void* p) noexcept { return p; }
inline void  operator delete(void*, void*) noexcept {}

template<typename T>
class DynamicArrayT {
	T*     _data;
	size_t _count;
	size_t _capacity;

	void _Grow() {
		size_t newCap = _capacity ? _capacity * 2 : 8;
		T* newData = (T*)malloc(newCap * sizeof(T));
		for (size_t i = 0; i < _count; i++) {
			new (newData + i) T(_data[i]);
			_data[i].~T();
		}
		if (_data) free(_data);
		_data = newData;
		_capacity = newCap;
	}

	void _DestroyElements() {
		for (size_t i = 0; i < _count; i++) {
			_data[i].~T();
		}
		_count = 0;
	}

public:
	DynamicArrayT() : _data(NULL), _count(0), _capacity(0) {}

	DynamicArrayT(const DynamicArrayT& other) : _data(NULL), _count(0), _capacity(0) {
		if (other._count) {
			_capacity = other._count;
			_data = (T*)malloc(_capacity * sizeof(T));
			for (size_t i = 0; i < other._count; i++) {
				new (_data + i) T(other._data[i]);
			}
			_count = other._count;
		}
	}

	DynamicArrayT& operator=(const DynamicArrayT& other) {
		if (this != &other) {
			_DestroyElements();
			if (_data) { free(_data); _data = NULL; _capacity = 0; }
			if (other._count) {
				_capacity = other._count;
				_data = (T*)malloc(_capacity * sizeof(T));
				for (size_t i = 0; i < other._count; i++) {
					new (_data + i) T(other._data[i]);
				}
				_count = other._count;
			}
		}
		return *this;
	}

	~DynamicArrayT() {
		_DestroyElements();
		if (_data) free(_data);
	}

	void Clear() { _DestroyElements(); }
	size_t Count() const { return _count; }

	void PushBack(const T& item) {
		if (_count >= _capacity) _Grow();
		new (_data + _count) T(item);
		_count++;
	}

	T* At(size_t index) {
		return index < _count ? &_data[index] : NULL;
	}

	T* GetElementPtrByIndex(size_t index) {
		return At(index);
	}
};

// ################################################################################################### //


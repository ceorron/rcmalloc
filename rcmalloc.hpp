/*----------------------------------------------------------------------------------*\
 |																					|
 | rcmalloc.hpp	 																	|
 |																					|
 | Copyright (c) 2019 Richard Cookman												|
 |																					|
 | Permission is hereby granted, free of charge, to any person obtaining a copy		|
 | of this software and associated documentation files (the "Software"), to deal	|
 | in the Software without restriction, including without limitation the rights		|
 | to use, copy, modify, merge, publish, distribute, sublicense, and/or sell		|
 | copies of the Software, and to permit persons to whom the Software is			|
 | furnished to do so, subject to the following conditions:							|
 |																					|
 | The above copyright notice and this permission notice shall be included in all	|
 | copies or substantial portions of the Software.									|
 |																					|
 | THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR		|
 | IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,			|
 | FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE		|
 | AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER			|
 | LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,	|
 | OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE	|
 | SOFTWARE.																		|
 |																					|
\*----------------------------------------------------------------------------------*/
#pragma once

#include <stdlib.h>
#include <string.h>
#include <memory>
#include <algorithm>
#include <mutex>

namespace rcmalloc {

const uint32_t ALLOC_PAGE_SIZE = 4096;

template<typename U>
inline ptrdiff_t dist(U* first, U* last) {
	return last - first;
}
inline int midpoint(unsigned imin, unsigned imax) {
	return (imin + imax) >> 1;
}
//basic binary search
template<typename Itr, typename T, typename Less>
bool binary_search(Itr beg, Itr end, const T& item,
				   Less comp, Itr& out) {
	//binary search return the insertion point, in both the found and not found case
	int sze = rcmalloc::dist(beg, end);
	if(sze == 0) {
		out = end;
		return false;
	}

	int imin = 0;
	int imax = sze - 1;
	int imid = 0;

	// continue searching while [imin,imax] is not empty
	while(imin <= imax) {
		// calculate the midpoint for roughly equal partition
		imid = midpoint(imin, imax);
		// determine which subarray to search
		if (comp(*(beg + imid), item))
			// change min index to search upper subarray
			imin = imid + 1;
		else if(comp(item, *(beg + imid)))
			// change max index to search lower subarray
			imax = imid - 1;
		else {
			out = (beg + imid);
			return true;
		}
	}
	// item was not found return the insertion point
	out = (beg + imin);
	return false;
}

//basic replacement for vector
struct basic_list {
	void* ptr = 0;
	uint32_t reserved = 0;
	uint32_t size = 0;
};

template<typename T>
inline T* basic_list_realloc(T* ptr, uint32_t newsize) {
	if(ptr == 0)
		return (T*)malloc(newsize);
	return (T*)realloc((void*)ptr, newsize);
}
template<typename T>
inline T* begin_basic_list(basic_list& ths) {
	return (T*)ths.ptr;
}
template<typename T>
inline T* end_basic_list(basic_list& ths) {
	return ((T*)ths.ptr) + ths.size;
}
template<typename T>
inline const T* begin_basic_list(const basic_list& ths) {
	return (const T*)ths.ptr;
}
template<typename T>
inline const T* end_basic_list(const basic_list& ths) {
	return ((const T*)ths.ptr) + ths.size;
}
template<typename T>
basic_list init_basic_list(uint32_t rsvr = 10) {
	if(rsvr == 0) rsvr = 10;
	basic_list rtn;
	rtn.ptr = calloc(rsvr, sizeof(T));
	rtn.reserved = rsvr;
	rtn.size = 0;
	return rtn;
}
template<typename T>
void dtor_basic_list(basic_list& ths) {
	for(auto it = begin_basic_list<T>(ths); it != end_basic_list<T>(ths); ++it)
		it->~T();
	free(ths.ptr);
	ths.ptr = 0;
	ths.reserved = 0;
	ths.size = 0;
}
template<typename T>
void clear_basic_list(basic_list& ths) {
	dtor_basic_list<T>(ths);
}
template<typename T>
T* insert_basic_list(basic_list& ths, T* insrt, T&& item) {
	if(ths.size == ths.reserved) {
		ths.reserved = (ths.reserved == 0 ? 10 : ths.reserved * 2);
		uint32_t pst = dist(begin_basic_list<T>(ths), insrt);
		ths.ptr = basic_list_realloc<T>((T*)ths.ptr, sizeof(T) * ths.reserved);
		insrt = (T*)ths.ptr + pst;
	}

	memmove((char*)(insrt + 1), (char*)insrt, sizeof(T) * dist(insrt, end_basic_list<T>(ths)));
	new (insrt) T(std::move(item));
	++ths.size;
	memset((char*)&item, 0, sizeof(T));
	return insrt;
}
template<typename T>
T* erase_basic_list(basic_list& ths, T* item) {
	//call destructor
	item->~T();
	memmove((char*)item, (char*)(item + 1), sizeof(T) * dist(item + 1, end_basic_list<T>(ths)));
	--ths.size;
	return item;
}
template<typename T>
inline T* push_back_basic_list(basic_list& ths, T&& item) {
	return insert_basic_list(ths, end_basic_list<T>(ths), std::move(item));
}
template<typename T>
inline T* pop_back_basic_list(basic_list& ths) {
	return erase_basic_list<T>(ths, end_basic_list<T>(ths) - 1);
}
template<typename T>
inline T& index_basic_list(basic_list& ths, uint32_t idx) {
	return *((T*)ths.ptr + idx);
}
template<typename T>
inline const T& index_basic_list(const basic_list& ths, uint32_t idx) {
	return *((const T*)ths.ptr + idx);
}
template<typename T>
inline uint32_t size_basic_list(const basic_list& ths) {
	return ths.size;
}

template<typename T>
void readLclInt(const uint8_t* bfr, uint32_t& bfrPos, T& val) {
	val = 0;
	for(uint32_t i = 0; i < sizeof(T); ++i) {
		val <<= 8;
		val |= bfr[bfrPos];
		++bfrPos;
	}
}

//prevent circular reference to new/delete
template<typename T>
T* malloc_new() {
	T* rtn = (T*)malloc(sizeof(T));
	new (rtn) T();
	return rtn;
}
template<typename T>
void delete_free(T* ptr) {
	ptr->~T();
	free(ptr);
}

struct object_data {
	uint32_t alignment;
	uint32_t size_of;
};

template<typename T>
struct object_move_generator {
	static void object_move(void* frm, void* to) {
		if constexpr (!std::is_trivially_copyable<T>::value)
			new (to) T(std::move(*(T*)frm));
	}
	static void object_intermediary_move(void* frm, void* to) {
		if constexpr (!std::is_trivially_copyable<T>::value) {
			//move to intermediary first
			T intermediary(std::move(*(T*)frm));
			new (to) T(std::move(intermediary));
		}
	}
};

typedef void (*object_move_func)(void* frm, void* to);

struct alloc_data {
	uint32_t size;
	uint32_t alignment;
	uint32_t size_of;
	uint32_t minalignment;
	uint32_t byterounding;
};

struct realloc_data {
	void* ptr;
	void* hint;
	uint32_t from_byte_size;
	uint32_t to_byte_size;
	uint32_t keep_byte_size_1;
	uint32_t keep_byte_size_2;
	int32_t keep_from_byte_offset_1;
	int32_t keep_from_byte_offset_2;
	int32_t keep_to_byte_offset_1;
	int32_t keep_to_byte_offset_2;
	uint32_t from_count_1;
	uint32_t from_count_2;
	uint32_t alignment;
	uint32_t size_of;
	uint32_t minalignment;
	uint32_t byterounding;
	object_move_func move_func;
	object_move_func intermediary_move_func;
	bool istrivial;
};

struct dealloc_data {
	void* ptr;
	uint32_t size;
	uint32_t alignment;
	uint32_t size_of;
	uint32_t minalignment;
	uint32_t byterounding;
};


template<typename T>
alloc_data init_alloc_data() {
	alloc_data rtn;
	memset((char*)&rtn, 0, sizeof(alloc_data));
	rtn.size = sizeof(T);
	rtn.alignment = std::alignment_of<T>();
	rtn.size_of = sizeof(T);
	rtn.minalignment = std::alignment_of<uintptr_t>();
	rtn.byterounding = sizeof(uintptr_t);
	return rtn;
}
alloc_data init_alloc_data_basic();

template<typename T>
realloc_data init_realloc_data() {
	realloc_data rtn;
	memset((char*)&rtn, 0, sizeof(realloc_data));
	rtn.to_byte_size = sizeof(T);
	rtn.alignment = std::alignment_of<T>();
	rtn.size_of = sizeof(T);
	if constexpr (!std::is_trivially_copyable<T>::value) {
		rtn.move_func = object_move_generator<T>::object_move;
		rtn.intermediary_move_func = object_move_generator<T>::object_intermediary_move;
	}
	rtn.istrivial = std::is_trivially_copyable<T>::value;
	rtn.minalignment = std::alignment_of<uintptr_t>();
	rtn.byterounding = sizeof(uintptr_t);
	return rtn;
}
realloc_data init_realloc_data_basic();
alloc_data to_alloc_data(const realloc_data* dat);

template<typename T>
dealloc_data init_dealloc_data() {
	dealloc_data rtn;
	memset((char*)&rtn, 0, sizeof(dealloc_data));
	rtn.size = sizeof(T);
	rtn.alignment = std::alignment_of<T>();
	rtn.size_of = sizeof(T);
	rtn.minalignment = std::alignment_of<uintptr_t>();
	rtn.byterounding = sizeof(uintptr_t);
	return rtn;
}
dealloc_data init_dealloc_data_basic();

struct vallocator;
typedef void (*stack_variable_cleanup)(vallocator* allocator, void* stkptr);

struct vgcsettings {
	//general virtual base class
	virtual const char* name() const = 0;
	virtual void ctorCopy(void* dat) const = 0;
	virtual void ctorMove(void* dat) = 0;
	virtual rcmalloc::object_data getDataDesc() const = 0;
	virtual ~vgcsettings();
};

struct vallocator {
	//general virtual base class
	virtual const char* name() const = 0;
	virtual void ctorCopy(void* dat) const = 0;
	virtual void ctorMove(void* dat) = 0;
	virtual rcmalloc::object_data getDataDesc() const = 0;
	virtual ~vallocator();
	//object allocation
	virtual void* do_malloc(const alloc_data* dat) = 0;
	virtual void* do_realloc(const realloc_data* dat) = 0;
	virtual void do_free(const dealloc_data* dat) = 0;
	//garbage collectors
	virtual void do_add_stack_variable(void* stkptr, stack_variable_cleanup fptr);
	virtual void do_remove_stack_variable_range(void* stkptr, uint32_t frame_size);
	virtual void do_cleanup(const vgcsettings& settings);
	virtual void do_test_cleanup(const vgcsettings& settings);
	virtual void* do_dereference(void* ptr);
	//get pointer to this
	vallocator& get_allocator();
	const vallocator& get_allocator() const;
};

struct memblock;

void roundAllocation(uint32_t minalignment, uint32_t byterounding,
					 uint32_t& size, uint32_t& alignment);
void roundAllocation(realloc_data& ldat);
void* align(uint32_t alignment, uint32_t size_of, void*& ptr);
char getMemOffset(void* ptr, uint32_t alignment, uint32_t size_of);
void* setAlignment(void* ptr, uint32_t alignment, uint32_t size_of);
void* getAlignment(void* ptr, uint32_t alignment, uint32_t& size, uint32_t& offset);

bool moveEndFirst(char* ptr1, int32_t keep_from_byte_offset,
				  char* ptr2, int32_t keep_to_byte_offset);
void* doMemMove(char* frmPtr, char* toPtr,
				const realloc_data& dat);
void addMemBlock(basic_list& blocklst, memblock* nMmBlck);
void findBlockForPointer(basic_list& blocklst, void* ptr,
						 memblock**& out);
void sortMemBlockDown(basic_list& blockfreespace,
					  memblock** itr);

struct bytesizes {
	uint32_t bytecount;
	char* ptr;
};
struct memblock {
	uint32_t bytetotal;
	uint32_t byteremain;
	char* ptr;
	//sorted by bytecount
	basic_list sizes;
	//sorted by ptr
	basic_list freelst;

	void init();
	~memblock();
	void* internal_malloc_at_hint(uint32_t size, bytesizes* pfrelst, void* hint);
	void* internal_malloc(uint32_t size);
	void* internal_realloc(
			const realloc_data* dat,
			char offset,
			bytesizes*& freeOut
	);
	void internal_free(void* ptr, uint32_t size, bytesizes*& freeOut);
};

template<unsigned AllocSize,
		 unsigned BlockID>
struct rc_allocator : public vallocator {
	//ordered by most recently allocated
	basic_list blockfreespace;
	basic_list blocklst;

	rc_allocator() {
		blockfreespace = init_basic_list<memblock*>(30);
		blocklst = init_basic_list<memblock*>(30);
	}
	~rc_allocator() {
		dtor_basic_list<memblock*>(blockfreespace);
		dtor_basic_list<memblock*>(blocklst);
	}
	void* malloc_new_block(uint32_t size) {
		uint32_t resz = ((size / AllocSize) + (size % AllocSize != 0 ? 1 : 0)) * AllocSize;

		void* nmem = malloc(resz);
		if(nmem == 0) return 0;

		memblock* nMmBlck = malloc_new<memblock>();
		memblock* tMmBlck = nMmBlck;
		nMmBlck->init();
		nMmBlck->bytetotal = resz;
		nMmBlck->byteremain = resz - size;
		nMmBlck->ptr = (char*)nmem;

		if((resz - size) > 0) {
			push_back_basic_list<bytesizes>(nMmBlck->sizes, bytesizes{resz - size, (char*)nmem + size});
			push_back_basic_list<bytesizes>(nMmBlck->freelst, bytesizes{resz - size, (char*)nmem + size});

			if(resz > AllocSize)
				insert_basic_list<memblock*>(blockfreespace, begin_basic_list<memblock*>(blockfreespace), std::move(nMmBlck));
			else
				push_back_basic_list<memblock*>(blockfreespace, std::move(nMmBlck));
		} else
			insert_basic_list<memblock*>(blockfreespace, begin_basic_list<memblock*>(blockfreespace), std::move(nMmBlck));

		addMemBlock(blocklst, tMmBlck);
		return nmem;
	}

	void* internal_malloc_i(uint32_t size) {
		//if allocation >= AllocSize do new allocSize
		if(size >= AllocSize) {
			void* nmem = malloc(size);
			if(nmem == 0) return 0;

			memblock* nMmBlck = malloc_new<memblock>();
			memblock* tMmBlck = nMmBlck;
			nMmBlck->init();
			nMmBlck->bytetotal = size;
			nMmBlck->byteremain = 0;
			nMmBlck->ptr = (char*)nmem;

			insert_basic_list<memblock*>(blockfreespace, begin_basic_list<memblock*>(blockfreespace), std::move(nMmBlck));
			addMemBlock(blocklst, tMmBlck);
			return nmem;
		}

		//ensure we don't take too long trying to allocate, only search the first 10 blocks!!
		uint32_t i = 0;
		for(auto it = end_basic_list<memblock*>(blockfreespace) - 1;
			it != begin_basic_list<memblock*>(blockfreespace) - 1 && i < 10;
			--it, ++i) {
			void* nmem = 0;
			if((nmem = (*it)->internal_malloc(size)) != 0)
				return nmem;
		}

		//add a new block to hold this
		return malloc_new_block(size);
	}
	void* internal_realloc_i(
			const realloc_data* dat,
			char offset
	) {
		realloc_data lclDat = *dat;
		uint32_t alignbytes = 0;
		if(lclDat.alignment >= 2)
			alignbytes = lclDat.alignment;
		lclDat.from_byte_size += alignbytes;
		lclDat.to_byte_size += alignbytes;
		lclDat.ptr = (char*)lclDat.ptr - offset;
		lclDat.hint = (char*)lclDat.hint - offset;

		//search
		memblock** out;
		findBlockForPointer(blocklst, lclDat.ptr, out);

		bytesizes* freeOut = 0;
		void* rtn = (*out)->internal_realloc(
						&lclDat,
						offset,
						freeOut
					);

		if(rtn == 0) {
			//allocate a new block to move this to
			void* rslt = malloc_new_block(lclDat.to_byte_size);
			if(rslt == 0) {
				//we have freed this, don't allow that, restore the old size block!!
				rtn = (*out)->internal_malloc_at_hint(lclDat.from_byte_size, freeOut, lclDat.ptr);
				return 0;
			}

			//do alignment
			if(lclDat.alignment >= 2) {
				void* rtn = rslt;
				rcmalloc::align(lclDat.alignment,
							 lclDat.size_of,
							 rtn);

				if(rtn == rslt)
					rtn = (char*)rtn + lclDat.alignment;

				//store the offset to the true block of this
				char offset = dist((char*)rslt, (char*)rtn);
				*((char*)rtn - 1) = offset;
				rslt = rtn;
			}

			//do memove - guaranteed to have no overlap
			doMemMove((char*)lclDat.ptr + offset, (char*)rslt, lclDat);

			//free this block
			sortMemBlockDown(blockfreespace, out);
			return rslt;
		}
		if(lclDat.to_byte_size < lclDat.from_byte_size)
			sortMemBlockDown(blockfreespace, out);
		return rtn;
	}
	void internal_free_i(void* ptr, uint32_t size) {
		if(ptr == 0) return;
		//search
		memblock** out;
		findBlockForPointer(blocklst, ptr, out);

		bytesizes* freeOut = 0;
		(*out)->internal_free(ptr, size, freeOut);

		if(size_basic_list<memblock*>(blocklst) > 1 && (*out)->byteremain == (*out)->bytetotal) {
			//do we want to free this block?
			//free/cleanup these
			memblock* crnt = (*out);
			free(crnt->ptr);
			erase_basic_list<memblock*>(blocklst, out);
			delete_free(crnt);

			auto it = std::find(begin_basic_list<memblock*>(blockfreespace),
								end_basic_list<memblock*>(blockfreespace),
								crnt);
			erase_basic_list<memblock*>(blockfreespace, it);
			return;
		}

		sortMemBlockDown(blockfreespace, out);
	}

	//virtual functions
	const char* name() const {
		return "rc_allocator";
	}
	void ctorCopy(void* dat) const {
		new (dat) rc_allocator(*this);
	}
	void ctorMove(void* dat) {
		new (dat) rc_allocator(std::move(*this));
	}
	rcmalloc::object_data getDataDesc() const {
		return rcmalloc::object_data{std::alignment_of<rc_allocator>(), sizeof(rc_allocator)};
	}

	void* do_malloc(const alloc_data* dat) {
		//handle alignment
		//always allocate atleast one byte!
		alloc_data ldat = *dat;
		roundAllocation(ldat.minalignment, ldat.byterounding, ldat.size, ldat.alignment);
		if(ldat.alignment < 2)
			return internal_malloc_i(ldat.size);

		uint32_t totalbytes = ldat.size + ldat.alignment;
		void* alc = internal_malloc_i(totalbytes);
		void* rtn = alc;

		rcmalloc::align(ldat.alignment,
					 ldat.size_of,
					 rtn);

		if(rtn == alc)
			rtn = (char*)rtn + ldat.alignment;

		//store the offset to the true block of this
		char offset = dist((char*)alc, (char*)rtn);
		*((char*)rtn - 1) = offset;
		return rtn;
	}
	void* do_realloc(const realloc_data* dat) {
		realloc_data lclDat = *dat;
		if(lclDat.ptr == 0) {
			alloc_data lclAllocDat = to_alloc_data(dat);
			return do_malloc(&lclAllocDat);
		}
		roundAllocation(lclDat);
		//always allocate atleast one byte, assume one byte was allocated last time!
		if(lclDat.from_byte_size == lclDat.to_byte_size)
			//just move the memory
			return doMemMove((char*)lclDat.ptr, (char*)lclDat.ptr, lclDat);

		//handle alignment - note alignment handled in internal_realloc_i
		char offset = 0;
		if(lclDat.alignment >= 2)
			offset = *((char*)lclDat.ptr - 1);
		//no need to store the alignment - simply realloc
		return internal_realloc_i(
				&lclDat,
				offset
			);
	}
	void do_free(const dealloc_data* dat) {
		//handle alignment
		if(dat->ptr == 0)
			return;
		dealloc_data ldat = *dat;
		roundAllocation(ldat.minalignment, ldat.byterounding, ldat.size, ldat.alignment);
		if(ldat.alignment < 2) {
			internal_free_i(ldat.ptr, ldat.size);
			return;
		}
		char offset = *((char*)ldat.ptr - 1);
		internal_free_i((char*)ldat.ptr - offset, ldat.size + ldat.alignment);
	}
};

template<unsigned AllocSize,
		 unsigned BlockID>
struct rc_internal_allocator {
	static rc_allocator<AllocSize, BlockID> fa;

	inline static void* do_malloc(const alloc_data* dat) {
		return fa.do_malloc(dat);
	}
	inline static void* do_realloc(const realloc_data* dat) {
		return fa.do_realloc(dat);
	}
	inline static void do_free(const dealloc_data* dat) {
		fa.do_free(dat);
	}
	inline static vallocator& get_allocator() {
		return fa.get_allocator();
	}
};

template<unsigned AllocSize,
		 unsigned BlockID>
rc_allocator<AllocSize, BlockID> rc_internal_allocator<AllocSize, BlockID>::fa;

template<typename Mtx = std::mutex,
		 unsigned AllocSize = ALLOC_PAGE_SIZE,
		 unsigned BlockID = 0>
struct rc_multi_threaded_internal_allocator {
	static Mtx mutex;
	static rc_allocator<AllocSize, BlockID> fia;

	static void* do_malloc(const alloc_data* dat) {
		std::lock_guard<Mtx> lg(mutex);
		return fia.do_malloc(dat);
	}
	static void* do_realloc(const realloc_data* dat) {
		//for performance - don't lock on no change
		if(dat->from_byte_size == dat->to_byte_size && dat->from_byte_size != 0)
			//just move the memory
			return doMemMove((char*)dat->ptr, (char*)dat->ptr, *dat);

		std::lock_guard<Mtx> lg(mutex);
		return fia.do_realloc(dat);
	}
	static void do_free(const dealloc_data* dat) {
		if(dat->ptr == 0) return;
		std::lock_guard<Mtx> lg(mutex);
		fia.do_free(dat);
	}
	inline static vallocator& get_allocator() {
		return fia.get_allocator();
	}
};

//static member construction
template<typename Mtx,
		 unsigned AllocSize,
		 unsigned BlockID>
Mtx rc_multi_threaded_internal_allocator<Mtx, AllocSize, BlockID>::mutex;
template<typename Mtx,
		 unsigned AllocSize,
		 unsigned BlockID>
rc_allocator<AllocSize, BlockID> rc_multi_threaded_internal_allocator<Mtx, AllocSize, BlockID>::fia;

template<typename T,
		 typename IAllocator = rc_multi_threaded_internal_allocator<std::mutex, ALLOC_PAGE_SIZE, 0>>
struct default_allocator {
	typedef T value_type;
	typedef T& reference;
	typedef T const& const_reference;
	typedef T* pointer;
	typedef T const* const_pointer;
	typedef ptrdiff_t difference_type;
	IAllocator allocator;

	inline void* allocate(const alloc_data* dat) {
		return allocator.do_malloc(dat);
	}
	inline void* reallocate(const realloc_data* dat) {
		return allocator.do_realloc(dat);
	}
	inline void deallocate(const dealloc_data* dat) {
		allocator.do_free(dat);
	}
	inline vallocator& get_allocator() {
		return allocator.get_allocator();
	}
	inline const vallocator& get_allocator() const {
		return allocator.get_allocator();
	}
};

template<typename T,
		 typename IAllocator = rc_multi_threaded_internal_allocator<std::mutex, ALLOC_PAGE_SIZE, 0>>
using allocator = default_allocator<T, IAllocator>;

template<typename T,
		 typename IAllocator = default_allocator<T>>
struct default_std_allocator {
	typedef T value_type;
	typedef T& reference;
	typedef T const& const_reference;
	typedef T* pointer;
	typedef T const* const_pointer;
	typedef std::size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef std::true_type propagate_on_container_move_assignment;
	typedef std::true_type is_always_equal;
	IAllocator allctr;

	pointer allocate(size_type n, const void* hint = 0) {
		alloc_data dat = init_alloc_data<value_type>();
		dat.size = n * sizeof(value_type);
		return (pointer)allctr.allocate(&dat);
	}

	void deallocate(T* p, std::size_t n) {
		dealloc_data dat = init_dealloc_data<value_type>();
		dat.ptr = p;
		dat.size = n * sizeof(value_type);
		allctr.deallocate(&dat);
	}

	inline size_type max_size() {
		return std::numeric_limits<size_type>::max();
	}

	inline void construct(pointer p, const_reference val) {
		new ((void*)p) T(val);
	}
	inline void destroy(pointer p) {
		p->~T();
	}
};

template<class T1, class T2, typename IAllocator>
inline bool operator==(const default_std_allocator<T1, IAllocator>& lhs, const default_std_allocator<T2, IAllocator>& rhs) noexcept {
	return true;
}
template<class T1, class T2, typename IAllocator>
inline bool operator!=(const default_std_allocator<T1, IAllocator>& lhs, const default_std_allocator<T2, IAllocator>& rhs) noexcept {
	return false;
}

template<typename Alloc>
typename Alloc::pointer allocate_init_count(uint32_t cnt) {
	//allocate using the allocator
	Alloc allctr;
	alloc_data dat = init_alloc_data<typename Alloc::value_type>();
	dat.size = sizeof(typename Alloc::value_type) * cnt;
	typename Alloc::pointer rtn = (typename Alloc::pointer)allctr.allocate(&dat);
	//do init
	typename Alloc::pointer tmp = rtn;
	for(uint32_t i = 0; i < cnt; ++i, ++tmp)
		new (tmp) typename Alloc::value_type;
	return rtn;
}
template<typename Alloc>
typename Alloc::pointer allocate_init_count(uint32_t cnt, const typename Alloc::value_type& val) {
	//allocate using the allocator
	Alloc allctr;
	alloc_data dat = init_alloc_data<typename Alloc::value_type>();
	dat.size = sizeof(typename Alloc::value_type) * cnt;
	typename Alloc::pointer rtn = (typename Alloc::pointer)allctr.allocate(&dat);
	//do init
	typename Alloc::pointer tmp = rtn;
	for(uint32_t i = 0; i < cnt; ++i, ++tmp)
		new (tmp) typename Alloc::value_type(val);
	return rtn;
}

template<typename Alloc>
inline typename Alloc::pointer allocate_init() {
	return allocate_init_count< Alloc >(1);
}

template<typename Alloc>
inline typename Alloc::pointer allocate_init(const typename Alloc::value_type& val) {
	//allocate using the allocator
	Alloc allctr;
	alloc_data dat = init_alloc_data<typename Alloc::value_type>();
	dat.size = sizeof(typename Alloc::value_type);
	typename Alloc::pointer rtn = (typename Alloc::pointer)allctr.allocate(&dat);
	new (rtn) typename Alloc::value_type(val);
	return rtn;
}

template<typename Alloc>
inline typename Alloc::pointer allocate_init(typename Alloc::value_type&& val) {
	//allocate using the allocator
	Alloc allctr;
	alloc_data dat = init_alloc_data<typename Alloc::value_type>();
	dat.size = sizeof(typename Alloc::value_type);
	typename Alloc::pointer rtn = (typename Alloc::pointer)allctr.allocate(&dat);
	new (rtn) typename Alloc::value_type(std::move(val));
	return rtn;
}

template<typename Alloc>
void destruct_deallocate_count(typename Alloc::pointer ptr, uint32_t cnt,
							   uint32_t alignment = std::alignment_of<typename Alloc::value_type>(),
							   uint32_t size_of = sizeof(typename Alloc::value_type)) {
	if(ptr == 0)
		return;
	//do destruct
	using X = typename Alloc::value_type;
	typename Alloc::pointer tmp = ptr;
	for(uint32_t i = 0; i < cnt; ++i, ++tmp)
		tmp[i].~X();
	//deallocate using the allocator
	Alloc allctr;
	dealloc_data dat = init_dealloc_data<typename Alloc::value_type>();
	dat.ptr = ptr;
	dat.size = sizeof(typename Alloc::value_type) * cnt;
	dat.alignment = alignment;
	dat.size_of = size_of;
	allctr.deallocate(&dat);
}

template<typename Alloc>
inline void destruct_deallocate(typename Alloc::pointer ptr,
								uint32_t alignment = std::alignment_of<typename Alloc::value_type>(),
								uint32_t size_of = sizeof(typename Alloc::value_type)) {
	destruct_deallocate_count< Alloc >(ptr, 1, alignment, size_of);
}

template<typename T>
inline T* new_T() {
	return allocate_init< default_allocator< T > >();
}
template<typename T>
inline T* new_T(const T& val) {
	return allocate_init< default_allocator< T > >(val);
}
template<typename T>
inline T* new_T(T&& val) {
	return allocate_init< default_allocator< T > >(std::move(val));
}
template<typename T>
inline T* new_T_array(uint32_t cnt) {
	return allocate_init_count< default_allocator< T > >(cnt);
}
template<typename T>
inline T* new_T_array(const T& val, uint32_t cnt) {
	return allocate_init_count< default_allocator< T > >(cnt, val);
}

template<typename T>
inline void delete_T(T* ptr) {
	destruct_deallocate_count< default_allocator< T > >(ptr, 1);
}
template<typename T>
inline void delete_T_array(T* ptr, uint32_t cnt) {
	destruct_deallocate_count< default_allocator< T > >(ptr, cnt);
}

}

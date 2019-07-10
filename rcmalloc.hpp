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

const unsigned ALLOC_PAGE_SIZE = 4096 * 4;

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
	size_t reserved = 0;
	size_t size = 0;
};

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
basic_list init_basic_list(size_t rsvr = 10) {
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
T* insert_basic_list(basic_list& ths, T* insrt, T&& item) {
	if(ths.size == ths.reserved) {
		ths.reserved = (ths.reserved == 0 ? 10 : ths.reserved * 2);
		size_t pst = dist(begin_basic_list<T>(ths), insrt);
		ths.ptr = realloc(ths.ptr, sizeof(T) * ths.reserved);
		insrt = (T*)ths.ptr + pst;
	}

	memmove(insrt + 1, insrt, sizeof(T) * dist(insrt, end_basic_list<T>(ths)));
	new (insrt) T(std::move(item));
	++ths.size;
	memset(&item, 0, sizeof(T));
	return insrt;
}
template<typename T>
T* erase_basic_list(basic_list& ths, T* item) {
	//call destructor
	item->~T();
	memmove(item, item + 1, sizeof(T) * dist(item + 1, end_basic_list<T>(ths)));
	--ths.size;
	return item;
}
template<typename T>
inline T* push_back_basic_list(basic_list& ths, T&& item) {
	return insert_basic_list(ths, end_basic_list<T>(ths), std::move(item));
}
template<typename T>
inline T* pop_back_basic_list(basic_list& ths) {
	return erase_basic_list<T>(ths, --end_basic_list<T>(ths));
}
template<typename T>
inline T& index_basic_list(basic_list& ths, size_t idx) {
	return *((T*)ths.ptr + idx);
}
template<typename T>
inline const T& index_basic_list(const basic_list& ths, size_t idx) {
	return *((const T*)ths.ptr + idx);
}

//prevent circular reference to new / delete
template<typename T>
T* malloc_new() {
	T* rtn = (T*)malloc(sizeof(T));
	new (rtn) T();
	return rtn;
}
template<typename T>
void free_delete(T* ptr) {
	ptr->~T();
	free(ptr);
}

struct object_data {
	size_t alignment;
	size_t size_of;
};

struct realloc_data {
	void* ptr;
	void* hint;
	size_t from_byte_size;
	size_t to_byte_size;
	size_t keep_byte_size_1;
	size_t keep_byte_size_2;
	ssize_t keep_from_byte_offset_1;
	ssize_t keep_from_byte_offset_2;
	ssize_t keep_to_byte_offset_1;
	ssize_t keep_to_byte_offset_2;
	size_t alignment;
	size_t size_of;
};

struct vallocator {
	virtual void* internal_malloc(size_t size, size_t alignment, size_t size_of) = 0;
	virtual void* internal_realloc(const realloc_data* dat) = 0;
	virtual void internal_free(void* ptr, size_t size, size_t alignment, size_t size_of) = 0;
	virtual void internal_cleanup() = 0;
	virtual void* internal_dereference(void* ptr);
	virtual vallocator& get_allocator();
};

struct memblock;

void* align(size_t alignment, size_t size_of, void*& ptr);
char getMemOffset(void* ptr, size_t alignment, size_t size_of);
void* setAlignment(void* ptr, size_t alignment, size_t size_of);
void* getAlignment(void* ptr, size_t alignment, size_t& size, size_t& offset);

bool moveEndFirst(char* ptr1, ssize_t keep_from_byte_offset,
				  char* ptr2, ssize_t keep_to_byte_offset);
void* doMemMove(char* frmPtr, char* toPtr,
				ssize_t keep_from_byte_offset_1,
				ssize_t keep_from_byte_offset_2,
				ssize_t keep_to_byte_offset_1,
				ssize_t keep_to_byte_offset_2,
				size_t keep_byte_size_1,
				size_t keep_byte_size_2);
void addMemBlock(basic_list& blocklst, memblock* nMmBlck);
void findBlockForPointer(basic_list& blocklst, void* ptr,
						 memblock**& out);
void sortMemBlockDown(basic_list& blockfreespace,
					  memblock** itr);

struct bytesizes {
	size_t bytecount;
	char* ptr;
};
struct memblock {
	size_t bytetotal;
	size_t byteremain;
	char* ptr;
	//sorted by bytecount
	basic_list sizes;
	//sorted by ptr
	basic_list freelst;

	void init();
	~memblock();
	void* internal_malloc_at_hint(size_t size, bytesizes* pfrelst, void* hint);
	void* internal_malloc(size_t size);
	void* internal_realloc(
			const realloc_data* dat,
			char offset,
			bytesizes*& freeOut
	);
	void internal_free(void* ptr, size_t size, bytesizes*& freeOut);
};

template< unsigned AllocSize,
		  unsigned blockID >
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
	void* malloc_new_block(size_t size) {
		size_t resz = ((size / AllocSize) + (size % AllocSize != 0 ? 1 : 0)) * AllocSize;

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

	void* internal_malloc_i(size_t size) {
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
		unsigned i = 0;
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
		size_t alignbytes = 0;
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
			//if we cannot realloc this then realloc using the C functions to a size big enough to hold the current size
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
			doMemMove((char*)lclDat.ptr + offset, (char*)rslt,
					  lclDat.keep_from_byte_offset_1,
					  lclDat.keep_from_byte_offset_2,
					  lclDat.keep_to_byte_offset_1,
					  lclDat.keep_to_byte_offset_2,
					  lclDat.keep_byte_size_1,
					  lclDat.keep_byte_size_2);

			//free this block
			sortMemBlockDown(blockfreespace, out);
			return rslt;
		}
		if(lclDat.to_byte_size < lclDat.from_byte_size)
			sortMemBlockDown(blockfreespace, out);
		return rtn;
	}
	void internal_free_i(void* ptr, size_t size) {
		if(ptr == 0) return;
		//search
		memblock** out;
		findBlockForPointer(blocklst, ptr, out);

		bytesizes* freeOut = 0;
		(*out)->internal_free(ptr, size, freeOut);

		if((*out)->byteremain == (*out)->bytetotal) {
			//do we want to free this block?
			size_t remainbytes = 0;
			unsigned i = 0;
			for(auto it = end_basic_list<memblock*>(blockfreespace) - 1;
				it != begin_basic_list<memblock*>(blockfreespace) - 1 && i < 10;
				--it, ++i) {
				if((*it)->ptr == (*out)->ptr) {
					--i;
					continue;
				}
				remainbytes += (*it)->byteremain;
			}
			if(remainbytes >= AllocSize) {
				//free/cleanup these
				memblock* crnt = (*out);
				free(crnt->ptr);
				erase_basic_list<memblock*>(blocklst, out);
				free_delete(crnt);

				auto it = std::find(begin_basic_list<memblock*>(blockfreespace),
									end_basic_list<memblock*>(blockfreespace),
									crnt);
				erase_basic_list<memblock*>(blockfreespace, it);
			}
			return;
		}
		sortMemBlockDown(blockfreespace, out);
	}

	void* internal_malloc(size_t size, size_t alignment, size_t size_of) {
		//handle alignment
		//always allocate atleast one byte!
		if(size == 0) size = 1;
		if(alignment < 2)
			return internal_malloc_i(size);

		size_t totalbytes = size + alignment;
		void* alc = internal_malloc_i(totalbytes);
		void* rtn = alc;

		rcmalloc::align(alignment,
					 size_of,
					 rtn);

		if(rtn == alc)
			rtn = (char*)rtn + alignment;

		//store the offset to the true block of this
		char offset = dist((char*)alc, (char*)rtn);
		*((char*)rtn - 1) = offset;
		return rtn;
	}
	void* internal_realloc(const realloc_data* dat) {
		realloc_data lclDat = *dat;
		if(lclDat.ptr == 0)
			return internal_malloc(lclDat.to_byte_size, lclDat.alignment, lclDat.size_of);
		//always allocate atleast one byte, assume one byte was allocated last time!
		if(lclDat.from_byte_size == 0) lclDat.from_byte_size = 1;
		if(lclDat.to_byte_size == 0) lclDat.to_byte_size = 1;
		if(lclDat.from_byte_size == lclDat.to_byte_size)
			//just move the memory
			return doMemMove((char*)lclDat.ptr, (char*)lclDat.ptr,
							 lclDat.keep_from_byte_offset_1,
							 lclDat.keep_from_byte_offset_2,
							 lclDat.keep_to_byte_offset_1,
							 lclDat.keep_to_byte_offset_2,
							 lclDat.keep_byte_size_1,
							 lclDat.keep_byte_size_2);

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
	void internal_free(void* ptr, size_t size, size_t alignment, size_t size_of) {
		//handle alignment
		if(ptr == 0)
			return;
		if(alignment < 2) {
			internal_free_i(ptr, size);
			return;
		}
		char offset = *((char*)ptr - 1);
		internal_free_i((char*)ptr - offset, size + alignment);
	}
	void internal_cleanup() {}
};

template< unsigned AllocSize,
		  unsigned blockID >
struct rc_internal_allocator {
	static rc_allocator<AllocSize, blockID> fa;

	inline static void* internal_malloc(size_t size, size_t alignment, size_t size_of) {
		return fa.internal_malloc(size, alignment, size_of);
	}
	inline static void* internal_realloc(const realloc_data* dat) {
		return fa.internal_realloc(dat);
	}
	inline static void internal_free(void* ptr, size_t size, size_t alignment, size_t size_of) {
		fa.internal_free(ptr, size, alignment, size_of);
	}
	inline static vallocator& get_allocator() {
		return fa.get_allocator();
	}
};

template< unsigned AllocSize,
		  unsigned blockID >
rc_allocator<AllocSize, blockID> rc_internal_allocator<AllocSize, blockID>::fa;

template< typename Mtx = std::mutex,
		  unsigned AllocSize = ALLOC_PAGE_SIZE,
		  unsigned blockID = 0 >
struct rc_multi_threaded_internal_allocator {
	static Mtx mutex;
	static rc_allocator<AllocSize, blockID> fia;

	static void* internal_malloc(size_t size, size_t alignment, size_t size_of) {
		std::lock_guard<Mtx> lg(mutex);
		return fia.internal_malloc(size, alignment, size_of);
	}
	static void* internal_realloc(const realloc_data* dat) {
		//for performance - don't lock on no change
		if(dat->from_byte_size == dat->to_byte_size && dat->from_byte_size != 0)
			//just move the memory
			return doMemMove((char*)dat->ptr, (char*)dat->ptr,
							 dat->keep_from_byte_offset_1,
							 dat->keep_from_byte_offset_2,
							 dat->keep_to_byte_offset_1,
							 dat->keep_to_byte_offset_2,
							 dat->keep_byte_size_1,
							 dat->keep_byte_size_2);

		std::lock_guard<Mtx> lg(mutex);
		return fia.internal_realloc(dat);
	}
	static void internal_free(void* ptr, size_t size, size_t alignment, size_t size_of) {
		if(ptr == 0) return;
		std::lock_guard<Mtx> lg(mutex);
		fia.internal_free(ptr, size, alignment, size_of);
	}
	inline static vallocator& get_allocator() {
		return fia.get_allocator();
	}
};

//static member construction
template< typename Mtx,
		  unsigned AllocSize,
		  unsigned blockID >
Mtx rc_multi_threaded_internal_allocator<Mtx, AllocSize, blockID>::mutex;
template< typename Mtx,
		  unsigned AllocSize,
		  unsigned blockID >
rc_allocator<AllocSize, blockID> rc_multi_threaded_internal_allocator<Mtx, AllocSize, blockID>::fia;

template< typename T,
		  typename IAllocator = rc_multi_threaded_internal_allocator<std::mutex, ALLOC_PAGE_SIZE, 0> >
struct default_allocator {
	typedef T value_type;
	typedef T& reference;
	typedef T const& const_reference;
	typedef T* pointer;
	typedef T const* const_pointer;
	typedef ptrdiff_t difference_type;
	IAllocator allocator;

	inline void* allocate(size_t byte_size,
						  size_t alignment = std::alignment_of<T>(),
						  size_t size_of = sizeof(T)) {
		return allocator.internal_malloc(byte_size, alignment, size_of);
	}
	inline void* reallocate(const realloc_data* dat) {
		return allocator.internal_realloc(dat);
	}
	inline void deallocate(void* ptr, size_t byte_size,
						   size_t alignment = std::alignment_of<T>(),
						   size_t size_of = sizeof(T)) {
		allocator.internal_free(ptr, byte_size, alignment, size_of);
	}
	inline vallocator& get_allocator() {
		return allocator.get_allocator();
	}
};

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
		return (pointer)allctr.allocate(n * sizeof(value_type), std::alignment_of<value_type>(), sizeof(value_type));
	}

	void deallocate(T* p, std::size_t n) {
		allctr.deallocate(p, n * sizeof(value_type), std::alignment_of<value_type>(), sizeof(value_type));
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
typename Alloc::pointer allocate_init_count(size_t cnt) {
	//allocate using the allocator
	Alloc allctr;
	typename Alloc::pointer rtn = (typename Alloc::pointer)allctr.allocate(sizeof(typename Alloc::value_type) * cnt);
	//do init
	typename Alloc::pointer tmp = rtn;
	for(size_t i = 0; i < cnt; ++i, ++tmp)
		new (tmp) typename Alloc::value_type;
	return rtn;
}
template<typename Alloc>
typename Alloc::pointer allocate_init_count(size_t cnt, const typename Alloc::value_type& val) {
	//allocate using the allocator
	Alloc allctr;
	typename Alloc::pointer rtn = (typename Alloc::pointer)allctr.allocate(sizeof(typename Alloc::value_type) * cnt);
	//do init
	typename Alloc::pointer tmp = rtn;
	for(size_t i = 0; i < cnt; ++i, ++tmp)
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
	typename Alloc::pointer rtn = (typename Alloc::pointer)allctr.allocate(sizeof(typename Alloc::value_type));
	new (rtn) typename Alloc::value_type(val);
	return rtn;
}

template<typename Alloc>
inline typename Alloc::pointer allocate_init(typename Alloc::value_type&& val) {
	//allocate using the allocator
	Alloc allctr;
	typename Alloc::pointer rtn = (typename Alloc::pointer)allctr.allocate(sizeof(typename Alloc::value_type));
	new (rtn) typename Alloc::value_type(std::move(val));
	return rtn;
}

template<typename Alloc>
void destruct_deallocate_count(typename Alloc::pointer ptr, size_t cnt,
							   size_t alignment = std::alignment_of<typename Alloc::value_type>(),
							   size_t size_of = sizeof(typename Alloc::value_type)) {
	if(ptr == 0)
		return;
	//do destruct
	using X = typename Alloc::value_type;
	typename Alloc::pointer tmp = ptr;
	for(size_t i = 0; i < cnt; ++i, ++tmp)
		tmp[i].~X();
	//deallocate using the allocator
	Alloc allctr;
	allctr.deallocate(ptr, sizeof(typename Alloc::value_type) * cnt, alignment, size_of);
}

template<typename Alloc>
inline void destruct_deallocate(typename Alloc::pointer ptr,
								size_t alignment = std::alignment_of<typename Alloc::value_type>(),
								size_t size_of = sizeof(typename Alloc::value_type)) {
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
inline T* new_T_array(size_t cnt) {
	return allocate_init_count< default_allocator< T > >(cnt);
}
template<typename T>
inline T* new_T_array(const T& val, size_t cnt) {
	return allocate_init_count< default_allocator< T > >(cnt, val);
}

template<typename T>
inline void delete_T(T* ptr) {
	destruct_deallocate_count< default_allocator< T > >(ptr, 1);
}
template<typename T>
inline void delete_T_array(T* ptr, size_t cnt) {
	destruct_deallocate_count< default_allocator< T > >(ptr, cnt);
}

}


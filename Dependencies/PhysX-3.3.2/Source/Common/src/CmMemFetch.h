// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2014 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#ifndef PX_PHYSICS_COMMON_MEMFETCH_H
#define PX_PHYSICS_COMMON_MEMFETCH_H

#include "PxMemory.h"

#ifdef __SPU__

#include "../ps3/CmPS3MemFetch.h"

#else

namespace physx
{
namespace Cm
{

#define pxPrintf printf

#ifndef PX_X64
	typedef unsigned int MemFetchPtr;
#else
	typedef PxU64 MemFetchPtr;
#endif

	struct MemFetchSmallBuffer {};
	struct MemFetchSmallBuffer16 {};
	template <typename T, int nb=1> struct MemFetchBufferA {}; // for aligned EA, pads to 16 bytes
	template <typename T, int nb=1> struct MemFetchBufferU {}; // for unaligned EA, adds 32 bytes of padding

	template <unsigned int T> class MemFetchGatherList
	{
	public:

		PX_FORCE_INLINE void init(){}
		PX_FORCE_INLINE void setSize(const unsigned int index, const unsigned int size){}
		PX_FORCE_INLINE unsigned int getSize(const unsigned int index) const {return 0;}
		PX_FORCE_INLINE void setEA(const unsigned int index, const unsigned int ea){}
		PX_FORCE_INLINE bool isValid(const unsigned int num) const  {return true;}
	};

	inline void memFetchWait(unsigned int) { }
	inline void memFetchWaitMask(unsigned int) { } 

	template<typename T> inline T	memFetch(MemFetchPtr ea, unsigned int /*channel*/)					{ return *(T*)ea; }

	template<typename T> inline T*	memFetchAsync(MemFetchPtr ea, unsigned int, MemFetchSmallBuffer&)	{ return (T*)ea; }
	template<typename T> inline T*	memFetchAsync(MemFetchPtr ea, unsigned int, MemFetchSmallBuffer16&)	{ return (T*)ea; }
	template<typename T> inline T*	memFetchAsync(void*, MemFetchPtr ea, unsigned int, unsigned int)	{ return (T*)ea; }

	template<typename T> inline T*	memFetchAsync(MemFetchPtr ea, unsigned int, MemFetchBufferA<T,1>&)	{ return (T*)ea; }
	template<typename T> inline T*	memFetchAsync(MemFetchPtr ea, unsigned int, MemFetchBufferU<T,1>&)	{ return (T*)ea; }
	template<typename T, int nb>
						 inline T*	memFetchAsync(MemFetchPtr ea, unsigned int, MemFetchBufferA<T,nb>&)	{ return (T*)ea; }
	template<typename T, int nb>
						 inline T*	memFetchAsync(MemFetchPtr ea, unsigned int, MemFetchBufferU<T,nb>&)	{ return (T*)ea; }

	inline void memFetchAlignedAsync(MemFetchPtr target, MemFetchPtr ea, unsigned int size, unsigned int)
	{
		PX_ASSERT((target & 0xF) == 0);
		PX_ASSERT((size & 0xF) == 0);
		PX_ASSERT((ea & 0xF) == 0);
		PxMemCopy((void*)target, (const void*)ea, size);
	}

	template<unsigned int N> inline void memFetchGatherListAsync(
		MemFetchPtr target, const MemFetchGatherList<N>& gatherList, unsigned int gatherListSize, unsigned int channel)
	{
	}

	template<typename T> inline void memStoreAsync(T* ls, MemFetchPtr ea, PxU32 channel, int count = 1)
	{
		PX_ASSERT((MemFetchPtr(ls) & 0xF) == 0);
		PX_ASSERT((sizeof(T) & 0xF) == 0);
		PX_ASSERT((ea & 0xF) == 0);
		PX_UNUSED(count);
		PX_UNUSED(channel);
		*((T*)ea) = *ls;
	}

	inline void memStoreWait(unsigned int channel) { memFetchWait(channel); }
}
} // namespace Cm
#endif

#endif

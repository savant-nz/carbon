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


#ifndef PX_PHYSICS_COMMON_RANGE
#define PX_PHYSICS_COMMON_RANGE

#include "CmPhysXCommon.h"

namespace physx
{
namespace Cm
{
	template<class T> 
	class Range 
	{
	private:
		T* mFirst;
		T* mLast; // past last element
	public:
		// c'tor
		Range();
		template <size_t N> 
		explicit Range(T (&array)[N]);
		Range(T* first, T* last);
		// mem functions
		PxU32 size() const;
		bool empty() const;
		void popFront();
		void popBack();
		T& front() const;
		T& back() const;
	};

	template <typename T, size_t N>
	Range<T> getRange(T (&array)[N])
	{
		return Range<T>(array);
	}

} // namespace Cm

template <typename T>
Cm::Range<T>::Range() : mFirst(0), mLast(0)
{}

template <typename T>
template <size_t N> 
Cm::Range<T>::Range(T (&array)[N]) : mFirst(array), mLast(array+N)
{}

template <typename T>
Cm::Range<T>::Range(T* first, T* last) : mFirst(first), mLast(last)
{
}

template <typename T>
PxU32 Cm::Range<T>::size() const
{
	return PxU32(mLast - mFirst); 
}

template <typename T>
bool Cm::Range<T>::empty() const
{
	return mFirst >= mLast; 
}

template <typename T>
void Cm::Range<T>::popFront()
{
	++mFirst;
}

template <typename T>
void Cm::Range<T>::popBack()
{
	--mLast;
}

template <typename T>
T& Cm::Range<T>::front() const
{
	return *mFirst;
}

template <typename T>
T& Cm::Range<T>::back() const
{
	return *(mLast-1);
}

}

#endif //PX_FRAMEWORK_PXITERATOR

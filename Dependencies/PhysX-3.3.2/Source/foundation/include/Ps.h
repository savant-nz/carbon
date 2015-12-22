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


#ifndef PX_FOUNDATION_PS_H
#define PX_FOUNDATION_PS_H

/*! \file top level include file for shared foundation */

#include "foundation/Px.h"

/**
Platform specific defines
*/
#if defined(PX_WINDOWS) || defined(PX_XBOXONE)
	#pragma intrinsic(memcmp)
	#pragma intrinsic(memcpy)
	#pragma intrinsic(memset)
	#pragma intrinsic(abs)
	#pragma intrinsic(labs)
#endif

#if defined(PX_VC) && !defined(PX_NO_WARNING_PRAGMAS) // get rid of browser info warnings
	// ensure this is reported even when we compile at level 4
	#pragma warning( disable : 4127 ) // conditional expression is constant
#endif
// An expression that should expand to nothing in non PX_DEBUG builds.  
// We currently use this only for tagging the purpose of containers for memory use tracking.
#if defined(PX_DEBUG)
#define PX_DEBUG_EXP(x) (x)
#define PX_DEBUG_EXP_C(x) x,
#else
#define PX_DEBUG_EXP(x)
#define PX_DEBUG_EXP_C(x)//notnv/epicgames/UE4/UE4_Epic_Base/
#endif

#define PX_SIGN_BITMASK		0x80000000

// Macro for avoiding default assignment and copy 
// because NoCopy inheritance can increase class size on some platforms.
#define PX_NOCOPY(Class) \
protected: \
	Class(const Class &); \
	Class &operator=(const Class &);

namespace physx
{
	namespace shdfnd 
	{
		// Int-as-bool type - has some uses for efficiency and with SIMD
		typedef int IntBool;
		static const IntBool IntFalse = 0;
		static const IntBool IntTrue = 1;

		template<class T, class Alloc> class Array;

		class ProfilerManager;
		class RenderOutput;
		class RenderBuffer;
	}

} // namespace physx


#endif

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

#ifndef GU_DISTANCE_SEGMENT_SEGMENT_H
#define GU_DISTANCE_SEGMENT_SEGMENT_H

#include "common/PxPhysXCommonConfig.h"
#include "GuSegment.h"

namespace physx
{
namespace Gu
{

	PX_PHYSX_COMMON_API PxReal distanceSegmentSegmentSquaredOLD(const PxVec3& seg0_origin, const PxVec3& seg0_extent,
																const PxVec3& seg1_origin, const PxVec3& seg1_extent,
																PxReal* s = NULL, PxReal* t = NULL);

	PX_INLINE PxReal distanceSegmentSegmentSquared(const Gu::Segment& seg0, const Gu::Segment& seg1, PxReal* s = NULL, PxReal* t = NULL)
	{
		return distanceSegmentSegmentSquaredOLD(seg0.p0, seg0.computeDirection(),
												seg1.p0, seg1.computeDirection(),
												s, t);
	}

	// This version fixes accuracy issues from the "OLD" one, but needs to do 2 square roots in order
	// to find the normalized direction and length of the segments, and then
	// a division in order to renormalize the output
	PX_PHYSX_COMMON_API PxReal distanceSegmentSegmentSquared2(	const Gu::Segment& segment0,
																const Gu::Segment& segment1,
																PxReal* param0=NULL, 
																PxReal* param1=NULL);

} // namespace Gu

}

#endif

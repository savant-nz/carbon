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


#ifndef PX_SPU_COMMON
#define PX_SPU_COMMON


// AP: Need to map from SPU_RAYCAST_MODULE to IS_SPU_RAYCAST and such because xpj doesn't support macros with values AFAIK
// These macros enable different query types in NpBatchQuery::execute; this in turn pulls in all the other necessary code.
// All unused code will then be stripped by the linker, so this is the only place where we differentiate between the 3 SPU modules.
#ifdef SPU_RAYCAST_MODULE
#define IS_SPU_RAYCAST 1
#else
#define IS_SPU_RAYCAST 0
#endif
#ifdef SPU_OVERLAP_MODULE
#define IS_SPU_OVERLAP 1
#else
#define IS_SPU_OVERLAP 0
#endif
#ifdef SPU_SWEEP_MODULE
#define IS_SPU_SWEEP 1
#else
#define IS_SPU_SWEEP 0
#endif


#endif


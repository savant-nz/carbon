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

#ifndef PX_FOUNDATION_PSERRORHANDLER_H
#define PX_FOUNDATION_PSERRORHANDLER_H

#include "foundation/PxErrorCallback.h"


namespace physx
{
namespace shdfnd
{

class PX_FOUNDATION_API ErrorHandler
{
public:
	static const PxU32 MAX_LISTENER_COUNT = 2;

	ErrorHandler();
	~ErrorHandler();

	/**
	\brief Reports an error code.
	\param code Error code, see #PxErrorCode
	\param message Message to display.
	\param file File error occured in.
	\param line Line number error occured on.
	*/
	void reportError(PxErrorCode::Enum code, const char* message, const char* file, int line);

	/**
	\brief Register the PxErrorCallback.
	\param callback Callback to register.
	\return The index of callback, return -1 if callbacks are more than #MAX_LISTENER_COUNT.
	*/
	PxI32 registerErrorCallback( PxErrorCallback& callback );

	/**
	\brief Un-Register the PxErrorCallback
	\param callback Callback to un-register.
	*/
	void unRegisterErrorCallback( PxErrorCallback& callback );

	/**
	\brief Un-Register the PxErrorCallback
	\param index The index of callback to un-register.
	*/
	void unRegisterErrorCallback( PxI32 index );

	/**
	\brief return the max number of error callback listeners
	*/
	PX_FORCE_INLINE PxU32 getMaxCallbackNum() const { return MAX_LISTENER_COUNT; }

	/**
	\brief return the number of error callback listeners
	*/
	PX_FORCE_INLINE PxU32 getCallbackCount() const { return (PxU32)mListenerCount; }
	
	/**
	\brief Return the PxErrorCallback
	\param idx The callback index.
	*/
	PX_FORCE_INLINE PxErrorCallback* getErrorCallback( PxI32 idx ) const { return mErrorCallbacks[idx]; }

private:
	PxErrorCallback*	mErrorCallbacks[MAX_LISTENER_COUNT];
	PxI32				mListenerCount;
	PxU32				mCallbackBitmap;
	
};

} // namespace shdfnd
} // namespace physx


/** @} */
#endif
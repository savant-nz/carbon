/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Exporters/Maya/MayaIncludeWrapper.h"
#include "CarbonEngine/Math/Color.h"
#include "CarbonEngine/Math/SimpleTransform.h"

namespace Carbon
{

namespace Maya
{

/**
 * If true then only selected meshes/objects will be exported.
 */
extern bool onlyExportSelected;

/**
 * Returns the client name for the Maya exporters to pass to Globals::initializeEngine() when starting an export.
 */
extern String getMayaClientName();

/**
 * Converts a Maya MPoint to a Vec3.
 */
extern Vec3 MPointToVec3(const MPoint& p);

/**
 * Converts a Maya MVector to a Vec3.
 */
extern Vec3 MVectorToVec3(const MVector& v);

/**
 * Converts a Maya MQuaternion to a Quaternion.
 */
extern Quaternion MQuaternionToQuaternion(const MQuaternion& mq);

/**
 * Converts a Maya MMatrix to a position and orientation, the original matrix is assumed to be an affine transform.
 */
extern SimpleTransform MMatrixToAffineTransform(const MMatrix& matrix);

/**
 * Converts a Maya MColor to a Color.
 */
extern Color MColorToColor(const MColor& color);

/**
 * Converts a Maya MString to a UnicodeString.
 */
extern UnicodeString MStringToString(const MString& mayaString);

/**
 * Converts a UnicodeString to an MString.
 */
extern MString toMString(const UnicodeString& string);

}

}

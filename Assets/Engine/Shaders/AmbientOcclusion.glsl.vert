/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef AMBIENT_OCCLUSION

attribute vec2 vsAmbientOcclusionTextureCoordinate;

varying vec2 tcAmbientOcclusionMap;

void ambientOcclusion()
{
    tcAmbientOcclusionMap = vsAmbientOcclusionTextureCoordinate;
}

#else

void ambientOcclusion()
{

}

#endif

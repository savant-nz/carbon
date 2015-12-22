/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef DECAL_MAPPING

attribute vec2 vsDecalTextureCoordinate;

varying vec2 tcDecalMap;

void decalMapping()
{
    tcDecalMap = vsDecalTextureCoordinate;
}

#else

void decalMapping()
{

}

#endif

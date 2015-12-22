/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

attribute vec3 vsPosition;

varying vec3 localVertexPosition;
varying vec2 tcNormalMap;

uniform sampler2D sHeightfield;

uniform vec3 clipmapValues;    // 1 / clipmapSize, 1 / blendRegionSize, clipmapSize / 2 - blendRegionSize - 1
uniform vec4 scales;           // finalScale, heightScale, textureScale, terrainScale
uniform vec3 clipmapOrigin;
uniform vec2 clipmapCameraPosition;

uniform mat4 modelViewProjection;

void main()
{
    // Sample the clipmap texture
    float height = texture2D(sHeightfield, vsPosition.xz * clipmapValues.x).r;

    // Calculate the height would be if it was being interpolated based on the data for the next clipmap. Vertices where x and z
    // are both even are not affected but if x and/or z are odd then the height would be interpolated. This second height is
    // used in the clipmap blend region around the outside in order to blend smoothly into the next clipmap level without seams
    // or 'pops' in the rendered geometry.

    vec2 coarserLayerOffsets = vec2(fract(vsPosition.x / 2.0) * 2.0, fract(vsPosition.z / 2.0) * 2.0);
    float heightInNextClipmap =
        texture2D(sHeightfield, (vsPosition.xz + vec2(coarserLayerOffsets.x, -coarserLayerOffsets.y)) * clipmapValues.x).r +
        texture2D(sHeightfield, (vsPosition.xz + vec2(-coarserLayerOffsets.x, coarserLayerOffsets.y)) * clipmapValues.x).r;
    heightInNextClipmap *= 0.5;

    // Calculate the blending amount, this will be one at the edges of the clipmap and goes linearly to zero at a rate dependent
    // on the size of the clipmap blend region.
    vec2 blendXY = abs(vsPosition.xz - clipmapCameraPosition + clipmapOrigin.xz / scales.x);
    blendXY -= clipmapValues.z;
    blendXY *= clipmapValues.y;
    float blendAmount = clamp(max(blendXY.x, blendXY.y), 0.0, 1.0);

    // Calculate the final vertex with its blended height
    vec3 layerVertex = vsPosition * scales.x;
    layerVertex.y = mix(height, heightInNextClipmap, blendAmount) * scales.y;

    gl_Position = modelViewProjection * vec4(layerVertex, 1.0);

    localVertexPosition = layerVertex + clipmapOrigin;
    tcNormalMap = vsPosition.xz * clipmapValues.x;
}

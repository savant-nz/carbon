/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Globals.h"

namespace Carbon
{

const String SceneExporterFileType = Globals::getEngineName() + " Scene";
const String SkeletalAnimationExporterFileType = Globals::getEngineName() + " Skeletal Animation";
const String SkeletalMeshExporterFileType = Globals::getEngineName() + " Skeletal Mesh";
const String StaticMeshExporterFileType = Globals::getEngineName() + " Static Mesh";

const String SceneExporterTitle = SceneExporterFileType + " Exporter";
const String SkeletalAnimationExporterTitle = SkeletalAnimationExporterFileType + " Exporter";
const String SkeletalMeshExporterTitle = SkeletalMeshExporterFileType + " Exporter";
const String StaticMeshExporterTitle = StaticMeshExporterFileType + " Exporter";

}

/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"

namespace Carbon
{

UnicodeString FileSystem::getCanonicalPath(const UnicodeString& path)
{
    return [[[NSURL fileURLWithPath:path.toNSString()] fileReferenceURL] path];
}

UnicodeString FileSystem::getUserLibraryDirectory()
{
    return NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES)[0];
}

UnicodeString FileSystem::getApplicationResourcesDirectory()
{
    return [[NSBundle mainBundle] resourcePath];
}

}

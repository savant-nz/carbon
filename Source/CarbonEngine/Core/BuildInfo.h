/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * This class provides information about the build configuration for the engine.
 */
class CARBON_API BuildInfo
{
public:

    /**
     * Returns build information about the engine including toolchain and architecture information, and compilation
     * environment setup. All of this information is automatically logged on startup.
     */
    static Vector<String> getBuildInfo();

    /**
     * Returns the engine's version. This returns the contents of the CARBON_VERSION define at build time or "Unknown"
     * if that define was missing. This is used to version SDK releases, see `BuildSDK.rb` for specific details about
     * versioning. The returned value will be suffixed with a '+' if the build was done on an unsynced branch or with
     * uncommitted changes present.
     */
    static String getVersion();

    /**
     * Returns whether this is a Max or Maya exporter build.
     */
    static bool isExporterBuild();
};

}

#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
# distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

import glob
import os

Import('*')

env = SConscript('Compilers/Clang.sconscript.py')

# Find the latest Mac OS X SDK
sdkPrefix = '/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.'
paths = glob.glob(sdkPrefix + '*')
if paths:
    paths = sorted(paths, key=lambda path: int(path[len(sdkPrefix):-4]))
    macOSXSDKPath = paths[-1]
else:
    print('Error: failed finding Mac OS X SDK, check that Xcode is installed')
    Exit(1)

# Setup environment for the selected SDK
flags = ['-arch', 'x86_64', '-mmacosx-version-min=10.8', '-isysroot', macOSXSDKPath]
env['CCFLAGS'] += flags + ['-fobjc-arc']
env['LINKFLAGS'] += flags + ['-Wl,-syslibroot,' + macOSXSDKPath]


# The SetupForLinkingCarbon() method sets up the environment for linking Carbon as a dynamic library or linking Carbon as a
# static library into a final application
def SetupForLinkingCarbon(self, **keywords):
    defaultDependencies = ['AngelScript', 'Bullet', 'FreeImage', 'FreeType', 'OpenAssetImport', 'PhysX', 'Vorbis', 'ZLib']

    dependencies = keywords.get('dependencies', defaultDependencies)

    self['LIBPATH'] += GetDependencyLIBPATH(*dependencies)
    self['LIBS'] += ['iconv'] + dependencies
    self['FRAMEWORKS'] += ['Cocoa', 'GameKit', 'IOKit', 'OpenAL', 'OpenGL', 'StoreKit']

env.AddMethod(SetupForLinkingCarbon)


# Add a method for setting up an environment ready for building against the installed SDK
def Carbonize(self, **keywords):
    if self.IsCarbonEngineStatic():
        self['CPPDEFINES'] += ['CARBON_STATIC_LIBRARY']

    self['LIBS'] += ['CarbonEngine' + {'Debug': 'Debug', 'Release': ''}[buildType]]

    if 'carbonroot' in ARGUMENTS:
        self['CPPPATH'] += [os.path.join(ARGUMENTS['carbonroot'], 'Source')]
        self['LIBPATH'] += [os.path.join(ARGUMENTS['carbonroot'], 'Build', 'MacOSX', 'x64', 'Clang', buildType)]

        if self.IsCarbonEngineStatic():
            self.SetupForLinkingCarbon()
    else:
        self['CPPPATH'] += ['/Applications/Carbon SDK/Include']
        self['LIBPATH'] += ['/Applications/Carbon SDK/Library']

        if self.IsCarbonEngineStatic():
            self.SetupForLinkingCarbon(dependencies=[])

    self.Append(**keywords)
    self.SetupPrecompiledHeader(keywords)

env.AddMethod(Carbonize)

# Return the build details
details = {'platform': 'MacOSX', 'architecture': 'x64', 'compiler': 'Clang', 'env': env}
Return('details')

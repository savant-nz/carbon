#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
# distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

import glob
import os
import sys

Import('*')

vars = Variables()
vars.AddVariables(
    ('architecture', 'Sets the target build architecture, must be ARM64 or x64.')
)
Help(vars.GenerateHelpText(Environment()))

# Get target architecture
architecture = ARGUMENTS.get('architecture', 'ARM64')
if architecture not in ['ARM64', 'x64']:
    print('Error: invalid build architecture')
    Exit(1)

# Get path to the SDK and associated flags
sdkPath = '/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk'
versionMinFlag = '-mios-simulator-version-min='

if not os.path.exists(sdkPath):
    print('Error: could not find the iOS Simulator SDK, check that Xcode is installed and up to date')
    Exit(1)

# Require iOS 12.0 as the minimum version
versionMinFlag += '12.0'

# Create build environment
env = SConscript('Compilers/Clang.sconscript.py')

sharedFlags = ['-arch', {'x64': 'x86_64', 'ARM64': 'arm64'}[architecture],
               '-isysroot', sdkPath, versionMinFlag]

env['ASFLAGS'] = sharedFlags
env['CCFLAGS'] += sharedFlags + ['-fobjc-arc', '-fobjc-legacy-dispatch']
env['LINKFLAGS'] += sharedFlags
env['AR'] = 'xcrun libtool'
env['ARCOM'] = '$AR $ARFLAGS $_LIBDIRFLAGS $_LIBFLAGS -o $TARGET $SOURCES > /dev/null 2>&1'
env['ARFLAGS'] = ['-static']

# Flags for the iOS simulator
env['CCFLAGS'] += ['-fobjc-abi-version=2']
env['LINKFLAGS'] += ['-Xlinker', '-objc_abi_version', '-Xlinker', '2']


# This method sets up the environment for linking Carbon as a static library into a final application
def SetupForLinkingCarbon(self, **keywords):
    defaultDependencies = ['AngelScript', 'Bullet', 'FreeImage', 'OpenAssetImport', 'Vorbis', 'ZLib']
    dependencies = keywords.get('dependencies', defaultDependencies)

    self['LIBPATH'] += GetDependencyLIBPATH(*dependencies)
    self['LIBS'] += dependencies

    self['FRAMEWORKS'] += ['CoreGraphics', 'Foundation', 'GameKit', 'OpenAL', 'OpenGLES', 'QuartzCore', 'StoreKit',
                           'UIKit']

env.AddMethod(SetupForLinkingCarbon)


# Add a method for setting up an environment ready for building against the installed SDK
def Carbonize(self, **keywords):
    if 'carbonroot' in ARGUMENTS:
        self['CPPPATH'] += [os.path.join(ARGUMENTS['carbonroot'], 'Source')]
        self['LIBPATH'] += [os.path.join(ARGUMENTS['carbonroot'], 'Build/iOSSimulator', architecture, 'Clang', buildType)]
        self['LIBS'] += ['CarbonEngine' + {True: 'Debug', False: ''}[isDebugBuild]]
        self.SetupForLinkingCarbon()
    else:
        self['CPPPATH'] += ['/Applications/Carbon SDK/Include']
        self['LIBPATH'] += ['/Applications/Carbon SDK/Library']
        self['LIBS'] += ['CarbonEngineiOSSimulator' + {True: 'Debug', False: ''}[isDebugBuild]]
        self.SetupForLinkingCarbon(dependencies=[])

    self.Append(**keywords)
    self.SetupPrecompiledHeader(keywords)
env.AddMethod(Carbonize)

# Return all the build setup details for this platform
details = {'platform': 'iOSSimulator', 'architecture': architecture, 'compiler': 'Clang', 'env': env,
           'isCarbonEngineStatic': True}
Return('details')

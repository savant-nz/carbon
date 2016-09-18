#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
# distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

import os
import sys

Import('*')

NDK_PATH = os.environ.get('CARBON_ANDROID_NDK_PATH', '')

vars = Variables()
vars.AddVariables(
    ('architecture', 'Sets the target build architecture, must be either ARMv7, ARM64, x86 or x64.', 'ARMv7'),
    ('ndk', 'Specifies the path to the Android NDK that should be used to do the Android build. The default is to read '
            'the Android NDK path from the CARBON_ANDROID_NDK_PATH environment variable.', NDK_PATH)
)
Help(vars.GenerateHelpText(Environment()))

NDK_PATH = os.path.abspath(ARGUMENTS.get('ndk', NDK_PATH))

# Get the Android name for the current platform
HOST_PLATFORM = {'darwin': 'darwin-x86_64', 'win32': 'windows-x86_64', 'linux2': 'linux-x86_64'}[sys.platform]

# Architecture configuration
architecture = ARGUMENTS.get('architecture', 'ARMv7')
ARCH_CONFIG = {
    'ARMv7': {
        'platform': 'arm', 'target': 'armv7-none-linux-androideabi', 'gcc_toolchain': 'arm-linux-androideabi',
        'libcpp': 'armeabi-v7a', 'gcc_bin_prefix': 'arm-linux-androideabi'
    },
    'ARM64': {
        'platform': 'arm64', 'target': 'aarch64-none-linux-android', 'gcc_toolchain': 'aarch64-linux-android',
        'libcpp': 'arm64-v8a', 'gcc_bin_prefix': 'aarch64-linux-android'
    },
    'x86': {
        'platform': 'x86', 'target': 'i686-none-linux-android', 'gcc_toolchain': 'x86',
        'libcpp': 'x86', 'gcc_bin_prefix': 'i686-linux-android'
    },
    'x64': {
        'platform': 'x86_64', 'target': 'x86_64-none-linux-android', 'gcc_toolchain': 'x86_64',
        'libcpp': 'x86_64', 'gcc_bin_prefix': 'x86_64-linux-android'
    }
}[architecture]


# Returns a full path inside the NDK
def GetNDKPath(*paths):
    return os.path.join(NDK_PATH, *paths)

# Construct the main paths in the NDK that will be needed and check they exist
NDK = {
    'sysroot': GetNDKPath('platforms/android-21/arch-' + ARCH_CONFIG['platform']),
    'clang_toolchain': GetNDKPath('toolchains/llvm-3.6/prebuilt', HOST_PLATFORM),
    'gcc_toolchain': GetNDKPath('toolchains', ARCH_CONFIG['gcc_toolchain'] + '-4.9/prebuilt', HOST_PLATFORM)
}
for name, path in NDK.items():
    if not os.path.isdir(path):
        print('Error: an expected NDK path is missing, check the Android NDK installation at "%s" is up to date'
              % NDK_PATH)
        Exit(1)


# Returns the path to a binary inside the Clang toolchain
def GetClangToolchainBinary(name):
    return os.path.join(NDK['clang_toolchain'], 'bin', name)


# Get base build environment then add the Android-specific changes needed
env = SConscript('Compilers/Clang.sconscript.py')

sharedFlags = ['--sysroot', NDK['sysroot'], '-target', ARCH_CONFIG['target']]

env['AR'] = GetClangToolchainBinary('llvm-ar')
env['ARFLAGS'].remove('-S')

env['AS'] = GetClangToolchainBinary('llvm-as')
env['ASFLAGS'] = sharedFlags

env['CC'] = GetClangToolchainBinary('clang')
env['CXX'] = GetClangToolchainBinary('clang++')
env['CPPPATH'] = [GetNDKPath('sources/android/cpufeatures'), GetNDKPath('sources/android/support/include'),
                  GetNDKPath('sources/cxx-stl/llvm-libc++/libcxx/include')]

env['CCFLAGS'] += sharedFlags
env['CCFLAGS'].remove('-stdlib=libc++')
if isStrictBuild:
    env['CCFLAGS'] += ['-Wno-c++98-compat-pedantic', '-Wno-pedantic', '-Wno-reserved-id-macro']
    env['CCFLAGS'].remove('-Wno-nullable-to-nonnull-conversion')

env['LIBPATH'] = [GetNDKPath('sources/cxx-stl/llvm-libc++/libs', ARCH_CONFIG['libcpp'])]
env['LIBS'] = ['android', 'c', 'c++_shared', 'dl', 'm']

env['LINK'] = GetClangToolchainBinary('clang++')
env['LINKCOM'] = '$LINK -o $TARGET $LINKFLAGS $__RPATH $SOURCES $_LIBDIRFLAGS $_LIBFLAGS'
env['LINKFLAGS'] = sharedFlags + ['-gcc-toolchain', NDK['gcc_toolchain']]

env['RANLIB'] = GetNDKPath(NDK['gcc_toolchain'], 'bin', ARCH_CONFIG['gcc_bin_prefix'] + '-ranlib')

env['SHLINKCOM'] = '$SHLINK -o $TARGET $SHLINKFLAGS $__RPATH $SOURCES $_LIBDIRFLAGS $_LIBFLAGS'
env['SHLINKFLAGS'] = ['$LINKFLAGS', '-shared']


# This method sets up the environment for linking Carbon as a static library
def SetupForLinkingCarbon(self):
    dependencies = ['AngelScript', 'Bullet', 'FreeImage', 'OpenALSoft', 'OpenAssetImport', 'Vorbis', 'ZLib']

    self['LIBPATH'] += GetDependencyLIBPATH(*dependencies)
    self['LIBS'] += dependencies + ['GLESv2', 'log', 'OpenSLES']

env.AddMethod(SetupForLinkingCarbon)


# Add a method for setting up an environment ready for building against the installed SDK
def Carbonize(self, **keywords):
    self['LIBS'] += ['CarbonEngine' + {True: 'Debug', False: ''}[isDebugBuild]]

    if ARGUMENTS.get('carbonroot', None):
        self['CPPPATH'] += [os.path.join(ARGUMENTS['carbonroot'], 'Source')]
        self['LIBPATH'] += [os.path.join(ARGUMENTS['carbonroot'], 'Build/Android', architecture, 'Clang', buildType)]
        self.SetupForLinkingCarbon()
    else:
        # There is no Android SDK at present
        raise 'Not supported'

    self.Append(**keywords)
    self.SetupPrecompiledHeader(keywords)

env.AddMethod(Carbonize)

# Return all the build setup details for this platform
details = {
    'platform': 'Android',
    'architecture': architecture,
    'compiler': 'Clang',
    'env': env,
    'isCarbonEngineStatic': True
}
Return('details')

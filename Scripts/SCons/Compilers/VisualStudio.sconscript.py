#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
# distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

# Creates and returns a build environment that uses Visual Studio.

Import('*')

EnsureSConsVersion(2, 4)

env = Environment(MSVC_VERSION={'VisualStudio2015': '14.0'}[compiler],
                  TARGET_ARCH={'x86': 'x86', 'x64': 'x86_64'}[architecture])

env['ARFLAGS'] = ['/MACHINE:' + architecture]
env['CCFLAGS'] = ['/EHsc', '/fp:fast', '/nologo']
env['CPPDEFINES'] = ['WIN32', '_WINDOWS']
env['LINKFLAGS'] = ['/DYNAMICBASE', '/MANIFEST', '/nologo', '/NXCOMPAT', '/MACHINE:' + architecture, '/INCREMENTAL:NO']

if architecture == 'x64':
    env['AS'] = 'ml64'

# Flags for strict builds
if isStrictBuild:
    env['CCFLAGS'] += ['/W4', '/WX']
    env['LINKFLAGS'] += ['/WX']

# Flags specific to the build type
if isDebugBuild:
    env['CPPDEFINES'] += ['_DEBUG']
    env['CCFLAGS'] += ['/MTd', '/Od', '/RTC1']
else:
    env['CCFLAGS'] += ['/MT', '/O2']
    env['CPPDEFINES'] += ['NDEBUG']
    env['LINKFLAGS'] += ['/OPT:REF,ICF']

    if architecture == 'x86':
        env['CCFLAGS'] += ['/arch:SSE2']


# Helper method for embedding a manifest file
def EmbedManifestFile(self, manifest, **keywords):
    self['_MANIFEST_FILE'] = File(manifest)
    if keywords.get('isDLL', False):
        self['SHLINKCOM'] += ['$MT /nologo -manifest $_MANIFEST_FILE -outputresource:$TARGET;2']
    else:
        self['LINKCOM'] += ['$MT /nologo -manifest $_MANIFEST_FILE -outputresource:$TARGET;1']

env.AddMethod(EmbedManifestFile)


# Precompiled header support
def UsePrecompiledHeader(self, header, **keywords):
    self['PCHSTOP'] = keywords.get('includeText', header)
    self['PCH'] = self.PCH(keywords.get('headerSourceFile', header[:-1] + 'cpp'))[0]
    self['PCHPDBFLAGS'] = ''

env.AddMethod(UsePrecompiledHeader)

Return('env')

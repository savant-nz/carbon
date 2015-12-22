#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
# distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

# Creates and returns a build environment that uses GCC.

import os
import sys
import GCCPrecompiledHeader

Import('*')

# Create base build environment
env = Environment(tools=['gcc', 'g++', 'gnulink', 'ar', 'as'])

env['ARFLAGS'] = ['rc', '-S']
env['CCFLAGS'] = ['-ffast-math', '-fvisibility=hidden', '-Wall']
env['CXXFLAGS'] = ['-std=c++11']
env['CPPDEFINES'] = []
env['ENV'] = {}
env['SHCCFLAGS'] = ['$CCFLAGS', '-fPIC']

if sys.platform == 'darwin':
    env.Tool('applelink')

# Silence output from ar and ranlib caused by object files that have no symbols
env['ARCOM'] += ' > /dev/null 2>&1'
env['RANLIBCOM'] += ' > /dev/null 2>&1'

# Flags for strict builds
if isStrictBuild:
    env['CCFLAGS'] += ['-pedantic-errors', '-Wextra', '-Wno-unused-parameter', '-Werror']
    env['LINKFLAGS'] += ['-Werror']

# Flags specific to the build type
if isDebugBuild:
    env['CCFLAGS'] += ['-g', '-O0']
    env['CPPDEFINES'] += ['DEBUG']
else:
    env['CCFLAGS'] += ['-O3']
    env['CPPDEFINES'] += ['NDEBUG']


# Precompiled header integration
def BuildPrecompiledHeader(self, header, **keywords):
    if keywords.get('isSharedLibrary', False):
        self['GCH'] = self.Clone().GchSh(header)[0]
    else:
        self['GCH'] = self.Clone().Gch(header)[0]

env.AddMethod(BuildPrecompiledHeader)
env.Tool(GCCPrecompiledHeader.generate)


def UsePrecompiledHeader(self, header, **keywords):
    self.BuildPrecompiledHeader(header, **keywords)
    self['CPPPATH'] = [keywords.get('pathToHeaderRoot', '.')] + self['CPPPATH']

env.AddMethod(UsePrecompiledHeader)

Return('env')

#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
# distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

# Creates and returns a build environment that uses Clang. This is done by altering the GCC build environment.

import os
import sys

Import('*')

env = SConscript('GCC.sconscript.py')

env['CC'] = 'clang'
env['CXX'] = 'clang++'
env['LINK'] = 'clang++'

env['CCFLAGS'] += ['-stdlib=libc++']
env['LINKFLAGS'] += ['-stdlib=libc++']

# Make color diagnostics work when piping Clang output through SCons
if 'TERM' in os.environ:
    env['ENV']['TERM'] = os.environ['TERM']
    env['CCFLAGS'] += ['-fcolor-diagnostics']

# Access the toolchain through xcrun when building on macOS
if sys.platform == 'darwin':
    for key in ['CC', 'CXX', 'LINK', 'AR', 'AS', 'RANLIB']:
        env[key] = 'xcrun ' + env[key]

# Extra warnings for strict builds
if isStrictBuild:
    env['CCFLAGS'] += ['-Weverything', '-Wno-c++98-compat', '-Wno-disabled-macro-expansion', '-Wno-documentation',
                       '-Wno-documentation-unknown-command', '-Wno-exit-time-destructors', '-Wno-float-equal',
                       '-Wno-format-nonliteral', '-Wno-global-constructors', '-Wno-header-hygiene',
                       '-Wno-implicit-fallthrough', '-Wno-keyword-macro', '-Wno-missing-noreturn',
                       '-Wno-missing-prototypes', '-Wno-nullable-to-nonnull-conversion', '-Wno-over-aligned',
                       '-Wno-padded', '-Wno-sign-conversion', '-Wno-switch-enum', '-Wno-weak-vtables']


# Alter GCC's precompiled header support to pass a -include-pch through to Clang
def UsePrecompiledHeader(self, header, **keywords):
    self.BuildPrecompiledHeader(header, **keywords)
    self['CCFLAGS'] += ['-Xclang', '-include-pch', '-Xclang', self['GCH']]

env.AddMethod(UsePrecompiledHeader)

Return('env')

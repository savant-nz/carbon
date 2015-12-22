#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
# distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

# Adds support for precompiled headers under GCC.

import SCons.Action
import SCons.Builder
import SCons.Scanner.C
import SCons.Util


def sourceDependsOnPrecompiledHeader(source, env):
    if not env.get('GCH'):
        return False

    precompiledHeaderSourceFile = str(env['GCH'].sources[0].srcnode())

    scanner = SCons.Scanner.C.CScanner()
    sourceDependencies = [str(x.srcnode()) for x in scanner(source[0], env, scanner.path(env))]

    return precompiledHeaderSourceFile in sourceDependencies


def staticObjectPCHEmitter(target, source, env):
    SCons.Defaults.StaticObjectEmitter(target, source, env)

    if sourceDependsOnPrecompiledHeader(source, env):
        env.Depends(target, env['GCH'])

    return target, source


def sharedObjectPCHEmitter(target, source, env):
    SCons.Defaults.SharedObjectEmitter(target, source, env)

    if sourceDependsOnPrecompiledHeader(source, env):
        env.Depends(target, env['GCH'])

    return target, source


def generateSuffix(env, sources):
    return sources[0].get_suffix() + '.gch'


def generate(env):
    GchBuilder = SCons.Builder.Builder(
        action=SCons.Action.Action('$GCHCOM', '$GCHCOMSTR'),
        source_scanner=SCons.Scanner.C.CScanner(),
        suffix=generateSuffix
    )

    GchShBuilder = SCons.Builder.Builder(
        action=SCons.Action.Action('$GCHSHCOM', '$GCHSHCOMSTR'),
        source_scanner=SCons.Scanner.C.CScanner(),
        suffix=generateSuffix
    )

    env['BUILDERS']['Gch'] = GchBuilder
    env['BUILDERS']['GchSh'] = GchShBuilder

    env['GCHCOM'] = '$CXX -o $TARGET -x c++-header $CCFLAGS $CXXFLAGS $_CCCOMCOM $SOURCE'
    env['GCHSHCOM'] = '$CXX -o $TARGET -x c++-header $CCFLAGS $CXXFLAGS $SHCCFLAGS $SHCXXFLAGS $_CCCOMCOM $SOURCE'

    for suffix in SCons.Util.Split('.c .C .cc .cxx .cpp .c++ .m .mm'):
        env['BUILDERS']['StaticObject'].add_emitter(suffix, staticObjectPCHEmitter)
        env['BUILDERS']['SharedObject'].add_emitter(suffix, sharedObjectPCHEmitter)

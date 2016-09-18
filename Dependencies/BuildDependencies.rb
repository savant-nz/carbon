#!/usr/bin/ruby
#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
# distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

#
# Use this script to run builds of all the dependencies. In order to do this the build environment must be setup,
# instructions doing this are in BUILDING.md. The dependency builds are all done through the SCons build system and
# don't rely on any build system or tool each individual dependency may use internally.
#
# If this script is run with no options then all dependencies for the current platform will be built. An alternate
# platform to build for can be specified with the --platform option, e.g.
#
#   BuildDependencies.rb --platform iOS
#
# To build a specific dependency or dependencies pass their names as arguments, e.g.
#
#   BuildDependencies.rb AngelScript FreeImage
#
# Run with --help for details of all supported options.
#

REPOSITORY_ROOT = File.expand_path File.join(File.dirname(__FILE__), '..')

require 'optparse'

require "#{REPOSITORY_ROOT}/Dependencies/DependencyBuilder.rb"

builder = DependencyBuilder.new

action = :build

options = OptionParser.new do |parser|
  parser.banner = 'Usage: BuildDependencies.rb [options] [<dependency-name>, ...]'

  parser.on '-c', '--clean', 'Deletes all intermediate build files for the dependencies rather than running a build' do
    action = :clean
  end

  parser.on '--compiler COMPILER', 'Sets the compiler to use, this overrides the platform\'s default' do |compiler|
    builder.compiler = compiler
  end

  parser.on '-h', '--help', 'Prints this help' do
    puts options
    exit
  end

  parser.on '-j', '--jobs N', 'Sets the number of concurrent build jobs' do |jobs|
    builder.jobs = jobs.to_i
  end

  parser.on '-p', '--platform PLATFORM', "Sets the target platform: #{supported_platforms.join ', '}" do |platform|
    builder.target_platform = platform.to_sym
  end

  parser.on '-v', '--verbose', 'Prints verbose build output' do |verbose|
    builder.verbose = verbose
  end
end

options.parse!

builder.dependencies = ARGV.map(&:to_sym)
builder.send "#{action}_dependencies"

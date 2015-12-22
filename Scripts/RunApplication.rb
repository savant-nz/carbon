#!/usr/bin/ruby
#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
# distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

#
# This script can be used to build and run the sample applications as well as simple external third party applications that
# don't have their own build scripts. Can be used for development and testing instead of the Visual Studio or Xcode projects.
#
# To build and run a sample application:
#
#     RunApplication.rb BoxesSample
#
# Run with --help for details of all supported options.
#

REPOSITORY_ROOT = File.expand_path File.join(File.dirname(__FILE__), '..')

require 'optparse'

require "#{REPOSITORY_ROOT}/Scripts/Shared.rb"
require "#{REPOSITORY_ROOT}/Scripts/SCons.rb"

# This class wraps a Carbon application that can then be built and run, intended for use during development and testing
class CarbonApplication
  attr_accessor :name, :source_root, :working_directory, :build_type, :clean, :jobs, :strict, :echo, :verbose

  def initialize
    self.build_type = :debug
    self.clean = false
    self.jobs = cpu_count
    self.strict = false
    self.echo = false
    self.verbose = false
  end

  def build_application
    error 'Dependencies are not built, first run Dependencies/BuildDependencies.rb' unless dependencies_built?

    SCons.scons scons_options(clean: true) if clean

    @executable = SCons.scons(scons_options).last
  end

  def run_application
    run @executable.quoted, echo: echo || verbose, echo_prefix: 'Running ... ', working_directory: working_directory do |line|
      if windows?
        print line
      else
        print_colored_line line
      end
    end

    puts "Exit code: #{$CHILD_STATUS.exitstatus}" unless $CHILD_STATUS.exitstatus == 0
  end

  private

  def dependencies_built?
    Dir.exist? File.join(REPOSITORY_ROOT, dependency_path(:ZLib, 'Library'))
  end

  def absolute_working_directory
    expanded_working_directory = File.expand_path working_directory

    error "The working directory does not exist: #{working_directory}" unless File.directory? expanded_working_directory

    expanded_working_directory
  end

  def scons_options(options = {})
    options[:jobs] = jobs
    options[:echo] = echo || verbose
    options[:verbose] = verbose
    options[:targets] = ['CarbonEngine', name]
    options[:working_directory] = REPOSITORY_ROOT
    options[:arguments] = scons_arguments options[:arguments]

    options
  end

  def scons_arguments(arguments = {})
    arguments ||= {}

    arguments[:type] = build_type.to_s.capitalize
    arguments[:strict] = strict
    arguments[:externalapp] = name if name !~ /Sample$/
    arguments[:externalappsourceroot] = source_root if source_root

    arguments
  end

  def print_colored_line(line)
    if line =~ /Error:/
      print "\e[1;31m#{line}\e[0m"
    elsif line =~ /Warning:/
      print "\e[1;34m#{line}\e[0m"
    else
      print "\e[32m#{line}\e[0m"
    end
  end
end

application = CarbonApplication.new

options = OptionParser.new do |parser|
  parser.banner = 'Usage: RunApplication.rb <application-name> [options]'

  parser.on '-c', '--clean', 'Forces a clean build of the application' do
    application.clean = true
  end

  parser.on '-e', '--echo', 'Prints the executed commands but does not turn on full verbose output' do
    application.echo = true
  end

  parser.on '-h', '--help', 'Prints this help' do
    puts options
    exit
  end

  parser.on '-j', '--jobs N', 'Sets the number of concurrent build jobs' do |jobs|
    application.jobs = jobs.to_i
  end

  parser.on '-r', '--source-root PATH', 'Sets the path to the application\'s source to build. Defaults to ../<app>' do |root|
    application.source_root = root
  end

  parser.on '-s', '--strict', 'Does a strict build, this enables maximum warning levels and all warnings are errors' do
    application.strict = true
  end

  parser.on '-t', '--build-type TYPE', [:debug, :release], 'Sets the build type, either debug or release' do |build_type|
    application.build_type = build_type
  end

  parser.on '-v', '--verbose', 'Prints verbose output' do
    application.verbose = true
  end

  parser.on '-w', '--working-directory DIR', 'Sets the application working directory' do |working_directory|
    application.working_directory = working_directory
  end
end

options.parse!

application.name = ARGV.first

if application.name.nil?
  puts options
  exit
end

if application.name =~ /Sample$/
  application.working_directory ||= "#{REPOSITORY_ROOT}/Assets/Samples"
else
  application.source_root ||= File.join '..', application.name
  application.working_directory ||= File.join REPOSITORY_ROOT, '..', application.name
end

application.build_application
application.run_application

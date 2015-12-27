#!/usr/bin/ruby
#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
# distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

#
# This script builds and packages the engine into a redistributable SDK for the current platform. There are SDKs for Windows and
# Mac OS X / iOS. To use this script the build environment must be setup, instructions for this can be found in BUILDING.md.
#
# On Windows, "Carbon SDK <version>.exe" is created.
#
# On Mac OS X, "Carbon SDK <version>.dmg" is created.
#

REPOSITORY_ROOT = File.expand_path File.join(File.dirname(__FILE__), '..')

require 'erb'
require 'optparse'

require "#{REPOSITORY_ROOT}/Scripts/Shared.rb"
require "#{REPOSITORY_ROOT}/Scripts/AmazonS3.rb"
require "#{REPOSITORY_ROOT}/Scripts/NSIS.rb"
require "#{REPOSITORY_ROOT}/Scripts/SCons.rb"
require "#{REPOSITORY_ROOT}/Scripts/CreateInstaller/CreateInstaller.rb"
require "#{REPOSITORY_ROOT}/SDK/Version.rb"
require "#{REPOSITORY_ROOT}/Dependencies/DependencyBuilder.rb"

# This class contains common functionality for building SDKs, it is subclassed for each platform
class SDKBuilderBase
  attr_accessor :compiler, :jobs, :verbose, :upload_to_s3

  def initialize
    @compiler = @default_compiler = SCons.default_arguments(platform).fetch :compiler
    @initial_working_directory = Dir.pwd
  end

  def build
    puts 'Building SDK ' + sdk_version + ' ...'

    build_documentation
    create_sdk
    upload_sdk if upload_to_s3
  ensure
    cleanup
  end

  protected

  def upload_sdk
    public_url = AmazonS3.upload_file sdk_filename, bucket: 'savant-nz-carbon', acl: 'public-read', target_prefix: 'sdks/'

    puts "URL: #{public_url}"
  end

  def restore_working_directory
    Dir.chdir @initial_working_directory
  end

  def build_documentation
    require "#{REPOSITORY_ROOT}/Documentation/BuildAPIReference.rb"
  end

  def scons_arguments(arguments = {})
    { compiler: compiler, strict: true }.merge arguments
  end

  def scons(options)
    SCons.scons({ jobs: jobs, verbose: verbose, echo: true }.merge(options))
  end

  def build_dependencies(options = {})
    builder = DependencyBuilder.new({ jobs: jobs, verbose: verbose, silent: !verbose }.merge(options))

    builder.compiler = compiler if compiler != @default_compiler

    builder.build_dependencies
  end

  def public_headers
    headers = Dir["#{REPOSITORY_ROOT}/Source/CarbonEngine/**/*.h"].select { |header| header !~ /EngineAssets\.h$/ }

    headers.each_with_object({}) { |header, hash| hash[header] = header.gsub("#{REPOSITORY_ROOT}/Source/", '') }
  end

  def public_scripts
    scripts = Dir["#{REPOSITORY_ROOT}/Scripts/**/*"].select do |script|
      script =~ /(rb|py)$/ && script !~ /(RunApplication|AmazonS3)\.rb$/
    end

    scripts.each_with_object({}) { |script, hash| hash[script] = script.gsub("#{REPOSITORY_ROOT}/Scripts/", '') }
  end

  def dependency_licenses
    CARBON_DEPENDENCIES.each do |dependency, version|
      license = Dir["#{REPOSITORY_ROOT}/Dependencies/#{dependency}-#{version}/License.*"].first

      yield license, dependency if license
    end
  end

  def create_template_files(options)
    puts 'Creating project template ...'

    class_name = options.fetch :class_name
    header_name = options[:header_name] || "#{class_name}.h"

    [:cpp, :h].each do |extension|
      erb_file = File.read "#{REPOSITORY_ROOT}/SDK/ProjectTemplates/Application.#{extension}.erb"

      File.write "#{options.fetch :target_without_extension}.#{extension}", ERB.new(erb_file).result(binding)
    end
  end

  def cleanup
    FileUtils.rm_rf "#{REPOSITORY_ROOT}/Documentation/APIReference"
  end
end

require "#{REPOSITORY_ROOT}/SDK/MacOSX/MacOSXSDKBuilder.rb"
require "#{REPOSITORY_ROOT}/SDK/Windows/WindowsSDKBuilder.rb"

builder_class = { Windows: WindowsSDKBuilder, MacOSX: MacOSXSDKBuilder }[platform]

error 'SDK build not supported on this platform' unless builder_class

builder = builder_class.new

options = OptionParser.new do |parser|
  parser.on '-c', '--compiler COMPILER', 'Sets the compiler to use, this overrides the platform\'s default' do |compiler|
    builder.compiler = compiler
  end

  if windows?
    parser.on '-f', '--fast-build', 'Makes the SDK build faster by using less aggressive compression options (Windows only)' do
      builder.fast_build = true
    end
  end

  parser.on '-h', '--help', 'Prints this help' do
    puts options
    exit
  end

  parser.on '-j', '--jobs N', 'Sets the number of concurrent build jobs' do |jobs|
    builder.jobs = jobs.to_i
  end

  parser.on '-u', '--upload-to-s3', 'Uploads the built SDK to the carbon-sdks Amazon S3 bucket' do |upload_to_s3|
    builder.upload_to_s3 = upload_to_s3
  end

  parser.on '-v', '--verbose', 'Prints verbose build output' do |verbose|
    builder.verbose = verbose
  end
end

options.parse!

builder.build

halt 'SDK build succeeded'

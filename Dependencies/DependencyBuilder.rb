#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
# distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

require "#{REPOSITORY_ROOT}/Scripts/Shared.rb"
require "#{REPOSITORY_ROOT}/Scripts/SCons.rb"

# The dependencies supported by each platform
SUPPORTED_DEPENDENCIES = {
  Android:      [:AngelScript, :Bullet, :FreeImage, :OpenALSoft, :OpenAssetImport, :Vorbis, :ZLib],
  iOS:          [:AngelScript, :Bullet, :FreeImage, :OpenAssetImport, :Vorbis, :ZLib],
  iOSSimulator: [:AngelScript, :Bullet, :FreeImage, :OpenAssetImport, :Vorbis, :ZLib],
  Linux:        [:AngelScript, :Bullet, :FreeImage, :FreeType, :OpenAssetImport, :Vorbis, :ZLib],
  macOS:        [:AngelScript, :Bullet, :FreeImage, :FreeType, :OpenAssetImport, :Vorbis, :ZLib],
  Windows:      [:AngelScript, :Bullet, :FreeImage, :FreeType, :OpenALSoft, :OpenAssetImport, :PhysX, :Vorbis, :ZLib]
}.freeze

# This class is the backend for BuildDependencies.rb
class DependencyBuilder
  attr_accessor :compiler, :jobs, :silent, :target_platform, :verbose

  def initialize(options = {})
    self.silent = false
    self.target_platform = platform
    self.verbose = false

    options.each do |key, value|
      send "#{key}=", value
    end
  end

  def dependencies
    @dependencies = supported_dependencies unless @dependencies && @dependencies.any?
    @dependencies
  end

  def dependencies=(dependencies)
    @dependencies = dependencies & supported_dependencies
  end

  def build_dependencies
    puts "Building #{target_platform} dependencies ..."

    send "build_#{target_platform.to_s.downcase}_dependencies"
  end

  def clean_dependencies
    dependencies.each do |dependency|
      puts "Cleaning dependency #{dependency} ..."

      FileUtils.rm_rf File.join(dependency_root(dependency), '.scons')
      FileUtils.rm_f File.join(dependency_root(dependency), '.sconsign.dblite')
    end
  end

  private

  def supported_dependencies
    SUPPORTED_DEPENDENCIES.fetch target_platform
  end

  def dependency_root(dependency)
    "#{REPOSITORY_ROOT}/Dependencies/#{dependency}-#{CARBON_DEPENDENCIES.fetch(dependency)}"
  end

  def scons_arguments(arguments)
    arguments ||= {}
    arguments[:compiler] ||= compiler
    arguments[:platform] ||= target_platform
    arguments
  end

  def scons_options(dependency, options = {})
    options ||= {}
    options[:arguments] = scons_arguments options[:arguments]
    options[:echo] = !silent
    options[:jobs] = jobs
    options[:verbose] = verbose
    options[:working_directory] = dependency_root(dependency)
    options
  end

  def build(dependency, options = {})
    options = scons_options(dependency, options)

    output = []

    options[:error] = proc do
      print output.join if silent
      error 'Dependency build failed'
    end

    SCons.scons options do |line|
      output << line
      print line unless silent
    end
  end

  def build_android_dependencies
    dependencies.product([:ARMv7, :x86]).each do |dependency, architecture|
      build dependency, arguments: { architecture: architecture }
    end
  end

  def build_ios_dependencies
    dependencies.each do |dependency|
      build dependency
    end
  end

  def build_iossimulator_dependencies
    dependencies.each do |dependency|
      libraries = ios_simulator_architectures.keys.map do |architecture|
        build dependency, arguments: { architecture: architecture }

        "#{dependency_root dependency}/Library/iOSSimulator/#{architecture}/lib#{dependency}.a"
      end

      merge_static_libraries libraries, "#{dependency_root dependency}/Library/iOSSimulator/lib#{dependency}.a", sdk: :iphoneos

      FileUtils.rm libraries
    end
  end

  def build_linux_dependencies
    dependencies.each do |dependency|
      build dependency
    end
  end

  def build_macos_dependencies
    dependencies.each do |dependency|
      build dependency
    end
  end

  def build_windows_dependencies
    dependencies.product([:Debug, :Release], [:x86, :x64]).each do |dependency, type, architecture|
      build dependency, arguments: { type: type, slim: true, architecture: architecture }
    end
  end
end

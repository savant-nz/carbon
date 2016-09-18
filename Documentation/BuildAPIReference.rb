#!/usr/bin/ruby
#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
# distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

REPOSITORY_ROOT = File.expand_path File.join(File.dirname(__FILE__), '..') unless defined? REPOSITORY_ROOT

require 'erb'
require 'tempfile'

require "#{REPOSITORY_ROOT}/Scripts/Shared.rb"

# This class uses Doxygen to build the API Reference from the source code documentation. The Doxygen configuration is
# stored in the `Doxygen.config.erb` file in this directory.
class APIReferenceBuilder
  def build
    FileUtils.rm_rf output_directory

    config_file = create_config_file_from_erb_template

    run_doxygen config_file
  ensure
    config_file.unlink if config_file
  end

  private

  def input_directories
    %w(
      . Core Core/FileSystem Core/Memory Core/Threads Game Game/Pathfinding Geometry Graphics Graphics/States Image Math
      Physics Platform Render Render/Shaders Render/Texture Scene Scene/EntityController Scene/GUI Scene/Mesh Scripting
      Sound
    ).map { |d| "Source/CarbonEngine/#{d}" }.join ' '
  end

  def output_directory
    File.join REPOSITORY_ROOT, 'Documentation/APIReference'
  end

  def config_file_template
    "#{REPOSITORY_ROOT}/Documentation/Doxygen.config.erb"
  end

  def config_file_doxygen_version
    File.read(config_file_template).split(' ')[2]
  end

  def installed_doxygen_version
    version = nil

    run('doxygen -v', echo: false, error: 'Doxygen is not installed') do |line|
      version ||= line.strip
    end

    version
  end

  def create_config_file_from_erb_template
    config_file = Tempfile.new 'doxygen.config'
    config_file.write ERB.new(File.read(config_file_template)).result(binding)
    config_file.close
    config_file
  end

  def run_doxygen(config_file)
    command = "doxygen #{config_file.path.quoted}"

    run command, working_directory: REPOSITORY_ROOT, echo: 'Building API reference ...',
                 error: 'Failed building API reference'
  end
end

APIReferenceBuilder.new.build

#!/usr/bin/ruby
#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
# distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

REPOSITORY_ROOT = File.expand_path File.join(File.dirname(__FILE__), '..')

require "#{REPOSITORY_ROOT}/Scripts/Shared.rb"
require "#{REPOSITORY_ROOT}/Scripts/Git.rb"

# This class uses clang-format and some custom extra customizations in order to automatically format the source code under this
# directory according to the rules specified in the .clang-format file. Requires that clang-format is installed. Note that
# current versions of clang-format do not preserve preprocessor indentation.
class SourceCodeFormatter
  def format_source_code
    source_files.each do |file|
      return [] unless File.exist? File.join(REPOSITORY_ROOT, file)

      clang_format file
      custom_format file
    end
  end

  private

  def source_files
    excluded_files = %w(Source/CarbonEngine/EngineAssets.h Source/CarbonEngine/Resource.h Source/WindowsSDKAssistant/Resource.h)

    Git.list_files(REPOSITORY_ROOT).select do |file|
      file =~ %r{^Source\/.*\.(h|cpp|mm)$} && !excluded_files.include?(file)
    end
  end

  def clang_format(file)
    run "clang-format -style=file -i #{file.quoted}", working_directory: REPOSITORY_ROOT, error: 'clang-format failed'
  end

  def file_lines(file)
    file = File.join REPOSITORY_ROOT, file

    return [] unless File.exist? file

    File.open(file, 'rt').readlines.map(&:rstrip)
  end

  def custom_format(file)
    lines = file_lines file

    new_lines = custom_format_lines lines
    new_lines << ''

    File.write File.join(REPOSITORY_ROOT, file), new_lines.join("\n")
  end

  def custom_format_lines(lines)
    lines.each_with_object([]) do |line, new_lines|
      # Add newline before left-aligned closing brace when the previous line was not indented
      new_lines << '' if line == '}' && new_lines.last[0] != ' ' && new_lines.last != '{' && new_lines.last.strip != '#endif'

      new_lines << line

      # Add newline after class access modifiers
      new_lines << '' if line =~ /^ *(public|protected|private):$/
    end
  end
end

SourceCodeFormatter.new.format_source_code

#!/usr/bin/ruby
#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
# distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

# Shortcut for opening a Carbon application's logfile, if no application name is passed on the command line then the
# most recently written logfile is opened.

require "#{File.expand_path File.dirname(__FILE__)}/Shared.rb"

def logfile_output_prefix
  if windows?
    ENV['APPDATA'].tr('\\', '/') + '/'
  elsif macos?
    "#{ENV['HOME']}/Library/Logs/"
  elsif linux?
    "#{ENV['HOME']}/."
  end
end

def logfile_for_application(application)
  logfile = "#{logfile_output_prefix}#{application}/#{application} Log.html"
  logfile = nil unless File.exist? logfile
  logfile
end

def most_recently_updated_logfile
  candidates = Dir.glob("#{logfile_output_prefix}{*,.*}/").map do |dir|
    candidate_name = File.basename dir
    candidate_name[0] = '' if linux?

    logfile_for_application candidate_name
  end

  candidates.compact.sort_by { |logfile| File.stat(logfile).mtime }.last
end

def memory_leaks_logfile(application_logfile)
  logfile = "#{application_logfile[0...-8]}Memory Leaks.html"
  logfile = nil unless File.exist?(logfile) && !File.read(logfile).include?('Detected 0 memory leaks')
  logfile
end

logfile = if ARGV.first
            logfile_for_application ARGV.first
          else
            most_recently_updated_logfile
          end

open_file_with_default_application logfile
open_file_with_default_application memory_leaks_logfile(logfile)

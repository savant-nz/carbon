#!/usr/bin/ruby
#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
# distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

REPOSITORY_ROOT = File.expand_path File.join(File.dirname(__FILE__), '..') unless defined? REPOSITORY_ROOT

require "#{REPOSITORY_ROOT}/Scripts/Shared.rb"
require "#{REPOSITORY_ROOT}/Scripts/Git.rb"

def sdk_version
  version = File.read("#{REPOSITORY_ROOT}/VERSION").lines.first.strip

  # Include the branch name if not on the master branch
  branch = Git.active_branch(REPOSITORY_ROOT)
  branch = nil if branch == 'master' || branch == 'HEAD'

  # Include the short commit hash if not building an official release from master
  commit_hash = Git.short_head_commit_hash(REPOSITORY_ROOT) if version.include?('-') || branch

  [version, branch, commit_hash].compact.join '-'
end

# frozen_string_literal: true

class Bop
  Version = Data.define(:major, :minor, :patch) do
    def self.from(string)
      new(*string.split(".").map(&:to_i))
    end

    def to_s
      "#{major}.#{minor}.#{patch}"
    end
  end

  attr_reader :version_file, :version, :previous_version

  def initialize(version_file)
    @version_file = version_file
    @previous_version = @version = current_version
  end

  def bop_to(string_version)
    bop(Version.from(string_version))
  end

  def bop_major
    bop(@previous_version.with(major: @previous_version.major + 1))
  end

  def bop_minor
    bop(@previous_version.with(minor: @previous_version.minor + 1))
  end

  def bop_patch
    bop(@previous_version.with(patch: @previous_version.patch + 1))
  end

  def bop_part(part)
    case part
    when "major"
      bop_major
    when "minor"
      bop_minor
    when "patch"
      bop_patch
    else
      raise "Unrecognized bop level '#{part}'"
    end
  end

  def bop(version)
    @version = version
  end

  def to_s
    @version.to_s
  end

  def current_version
    text_version = File.open(@version_file) { |f| f.grep(/VERSION "([\d.]+)"/) { $1 } }.first
    Version.from(text_version)
  end
end

namespace :version do
  desc "Bump major version"
  task :major do
    Rake::Task["version:_update_version"].invoke("major")
  end

  desc "Bump minor version"
  task :minor do
    Rake::Task["version:_update_version"].invoke("minor")
  end

  desc "Bump patch version"
  task :patch do
    Rake::Task["version:_update_version"].invoke("patch")
  end

  task :_update_version, [:bump] do |_task, args| # rubocop:disable Rake/Desc
    type = args[:bump]
    version_file = "src/playlistfs_version.h"
    bop = Bop.new(version_file)
    
    bop.bop_part(type)
    puts "Bumping version from #{bop.previous_version} to #{bop}"
    Rake::Task["version:_update_version_file"].invoke(version_file, bop)
    Rake::Task["version:_update_changelog"].invoke(File.basename(Dir.pwd), bop)
    Rake::Task["version:_commit_and_tag"].invoke(version_file, bop)
  end

  task :_update_version_file, [:file, :bop] do |_task, args| # rubocop:disable Rake/Desc
    name = args[:file]
    bop = args[:bop]
    text = File.read(name)
    text.gsub!(bop.previous_version.to_s, bop.version.to_s)
    File.write(name, text)
  end

  task :_update_changelog, [:name, :bop] do |_task, args| # rubocop:disable Rake/Desc
    require "date"

    name = args[:name]
    new_version = args[:bop]

    changelog = File.read("CHANGELOG.md").split(/(^(?>##+)[^\n]+\n\n)/)
    # Add new section and update version
    next_index = changelog.index { _1.match?(/^## \[Next\]/) }
    changelog[next_index].sub!(/^## \[Next\](.*)\n/, <<~TEXT)
      ## [Next]

      [Compare v#{new_version}...main](https://github.com/trinistr/#{name}/compare/v#{new_version}...main)

      ## [v#{new_version}]#{$1} — #{Date.today.to_s}
    TEXT
    # Change previous comparison link
    prev_index = changelog.index { _1.match?(/^## \[v#{new_version}\]/) }
    changelog[prev_index + 1].gsub!("...main", "...v#{new_version}") if prev_index
    # Add a version link
    changelog.last.sub!(
      /\[Next\]: .+/,
      "\\0\n[v#{new_version}]: https://github.com/trinistr/#{name}/tree/v#{new_version}"
    )
    # Delete v0.0.0 if present
    changelog.delete_if { _1.match?(/^## \[v0\.0\.0\]/) }

    File.write("CHANGELOG.md", changelog.join)
  end

  task :_commit_and_tag, [:file, :bop] do |_task, args| # rubocop:disable Rake/Desc
    file = args[:file]
    new_version = args[:bop]

    %W[CHANGELOG.md #{file}].each do |f|
      system("git", "add", "--update", f)
    end
    system("git", "commit", "-m", "Bump version to #{new_version}")
    system("git", "tag", "-s", "-m", "v#{new_version}", "v#{new_version}")
  end
end

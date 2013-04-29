require File.join(File.dirname(__FILE__), '..', 'clipp_test')

class TestRegression < Test::Unit::TestCase
  include CLIPPTest

  def make_config(file, extras = {})
    return {
      :config => "LoadModule \"ibmod_lua.so\"\n" +
        "LuaInclude \"#{file}\"\n" +
        "LuaCommitRules",
      :default_site_config => "RuleEnable all",
    }.merge(extras)
  end

  SITE_CONFIG = "RuleEnable all"


  def test_sig
    lua = <<-EOS
      Sig("basic1", "1"):
        fields([[REQUEST_METHOD]]):
        op('imatch', [[GET]]):
        phase([[REQUEST_HEADER]]):
        action([[clipp_announce:basic1]])
    EOS
    lua_file = File.expand_path(File.join(BUILDDIR, rand(10000).to_s + '.lua'))
    File.open(lua_file, 'w') {|fp| fp.print lua}

    clipp(make_config(lua_file,
      :input => "echo:\"GET /foo\""
    ))
    assert_no_issues
    assert_log_match /CLIPP ANNOUNCE: basic1/
  end

  def test_action
    lua = <<-EOS
      Action("basic2", "1"):
        phase([[REQUEST_HEADER]]):
        action([[clipp_announce:basic2]])
    EOS
    lua_file = File.expand_path(File.join(BUILDDIR, rand(10000).to_s + '.lua'))
    File.open(lua_file, 'w') {|fp| fp.print lua}

    clipp(make_config(lua_file,
      :input => "echo:\"GET /foo\""
    ))
    assert_no_issues
    assert_log_match /CLIPP ANNOUNCE: basic2/
  end
end

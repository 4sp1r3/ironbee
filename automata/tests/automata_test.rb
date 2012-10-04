$:.unshift(File.dirname(File.dirname(File.expand_path(__FILE__))))
require 'set'

module AutomataTest
  BENCH = File.join(ENV['abs_builddir'], "..", "bench")
  EC = File.join(ENV['abs_builddir'], "..", "ec")
  ACGEN = File.join(ENV['abs_builddir'], "..", "ac_generator")
  OPTIMIZE = File.join(ENV['abs_builddir'], "..", "optimize")
  OPTIMIZE_ARGS = ["--fast"]

  # Returns map of word to ending position of word in input.
  def substrings(words, input)
    result = Hash.new {|h,k| h[k] = Set.new}
    l = input.length
    words.each do |word|
      i = 0
      while i + word.length <= l
        j = input.index(word, i)
        break if ! j
        result[word] << j + word.length
        i = j + 1
      end
    end
    result
  end

  def parse_bench_output(output)
    result = Hash.new {|h,k| h[k] = Set.new}
    output.split("\n").each do |line|
      line.chomp!
      if line =~ /^\s*(\d+): (\w+)$/
        result[$2] << $1.to_i
      end
    end
    result
  end

  def assert_substrings_equal(a, b)
    a_keys = Set.new a.keys
    b_keys = Set.new b.keys
    a_minus_b = a_keys - b_keys
    b_minus_a = b_keys - a_keys
    assert_block("Missing keys: #{a_minus_b.to_a.join(', ')}") {a_minus_b.empty?}
    assert_block("Extra keys: #{b_minus_a.to_a.join(', ')}") {b_minus_a.empty?}

    a.each do |word, locations|
      assert_equal(locations, b[word], "Locations of #{word} mismatch.")
    end
  end

  def run_from_file(cmd, inpath, outpath)
    child_pid = nil
    File.open(inpath, "r") do |inf|
      File.open(outpath, "w") do |outf|
        child_pid = fork
        if child_pid.nil?
          STDIN.reopen(inf)
          STDOUT.reopen(outf)
          exec(*cmd)
        end
      end
    end
    Process.wait(child_pid)
    assert($?.success?)
  end

  def ac_test(words, text, prefix = "full_test", optimize = false)
    dir = "/tmp/automata_test_#{prefix}#{$$}.#{rand(100000)}"
    Dir.mkdir(dir)
    puts "Test files are in #{dir}"

    words_path = File.join(dir, "words")
    File.open(words_path, "w") do |fp|
      fp.puts words.join("\n")
    end

    ac_automata_path = File.join(dir, "ac_automata")
    run_from_file([ACGEN], words_path, ac_automata_path)

    input_path = File.join(dir, "input")
    File.open(input_path, "w") do |fp|
      fp.print text
    end

    if optimize
      automata_path = File.join(dir, "optimized_automata")
      run_from_file([OPTIMIZE, *OPTIMIZE_ARGS], ac_automata_path, automata_path)
    else
      automata_path = ac_automata_path
    end

    eudoxus_path = File.join(dir, "eudoxus")
    system(EC, "-i", automata_path, "-o", eudoxus_path)

    output_path = File.join(dir, "output")
    system(BENCH, "-a", eudoxus_path, "-o", output_path, "-i", input_path, "-t", "length")

    output_substrings = parse_bench_output(IO.read(output_path))
    assert_substrings_equal(substrings(words, text), output_substrings)
  end
end

Title:  IronBee Fast Pattern Manual
Author: Christopher Alfeld <calfeld@qualys.com>

Fast Pattern User Manual
========================

Christopher Alfeld <calfeld@qualys.com><br>

**Warning:** This document describes a feature that is under ongoing development.  Only portions of what is described are currently available and all of it is subject to change.  This document is intended for IronBee developers, those wanting to play on the bleeding edge, and those interested in upcoming major features.

Introduction
------------

The IronBee Fast Pattern system as an upcoming system for improved rule performance.  It allows certain rules to be selected for evaluation in a manner that is significantly faster than the default process.  In particular, properly setup, fast pattern rules that are not evaluated impose near-zero time cost.  In contrast, in normal IronBee, every rule is evaluated and thus imposed some time cost on every transaction.  The system does use more space (RAM) than default and should be viewed as a space/time trade-off.

The system works by attaching one or more fast patterns to a rule.  The rule will only be evaluated if the fast pattern appears in the input.  It is important to note that a rule may still evaluate to false.  Typically, a fast pattern represents a string (or set of strings) that must be present in the input.  For example, a rule for request headers that depends on the regular expression `^Foo:` could have a fast pattern of `Foo:`, in which case it would only be evaluated if 'Foo:' was present somewhere in the header data.  If that occurrence was `Content-Type: Foo:`, then the rule would evaluate to false as the regexp would not match.

An important constraint on fast pattern rules is that the order they execute in is not guaranteed.  Thus, any rule that depends on another rule in the same phase or that is depended on by another rule in the same phase should not use fast patterns.  The final constraint is that fast patterns do not work well with transformations.

Internally, all fast patterns for a phase are compiled into an IronAutomata automata.  At each phase, the automata is executed and searches for the patterns as substrings in the input.  For any patterns found, the associated rules are then evaluated.

Fast Pattern Syntax
-------------------

The fast pattern syntax is that of the IronAutomata Aho-Corasick patterns.  The syntax, unlike regular expressions, only allows fixed width expressions.  It provides operators for escaping, e.g., `\e` for escape, and for character sets, e.g., `\l` for any lower case character.  For the latest syntax, run `ac_generator --help` from IronAutomata.  The result as of this writing is:

	Patterns provide a variety of fixed width operators that are shortcuts for
	a byte or span of bytes.  E.g., "foo\dbar" is a pattern for "foo0bar",
	"foo1bar", ..., "foo9bar".

	Single Shortcuts:
	- \\ -- Backslash.
	- \t -- Horizontal tab.
	- \v -- Vertical tab.
	- \n -- New line
	- \r -- Carriage return.
	- \f -- Form feed.
	- \0 -- Null.
	- \e -- Escape.

	Parameterized Single Shortcuts:
	- \^X -- Control character, where X is A-Z, [, \, ], ^, _, or ?.
	- \xXX -- Byte XX in hex.
	- \iX -- Match lower case of X and upper case of X where X is A-Za-z.

	Multiple Shortcuts:
	- \d -- Digit -- 0-9
	- \D -- Non-Digit -- all but 0-9
	- \h -- Hexadecimal digit -- A-Fa-f0-9
	- \w -- Word Character -- A-Za-z0-9
	- \W -- Non-Word Character -- All but A-Za-z0-9
	- \a -- Alphabetic character -- A-Za-z
	- \l -- Lowercase letters -- a-z
	- \u -- Uppercase letters -- A-Z
	- \s -- White space -- space, \t\r\n\v\f
	- \S -- Non-white space -- All but space, \t\r\n\v\f
	- \$ -- End of line -- \r\f
	- \p -- Printable character, ASCII hex 20 through 7E.
	- \. -- Any character.

    Union Shortcuts:
    - [...] -- Union of all shortcuts inside brackets.  Hyphens are treated
               differently in unions.  A hyphen must either appear at the
               beginning of the union or as part of a range A-B where A < B.
               A and B may be single shortcuts.  An initial hyphen indicates
               that a hyphen should be part of the union.
    - [^...] -- As above, but negated.

Using Fast Patterns
-------------------

**Step 1**: Add `fast:` modifiers to your rules.

Look for rules that require a certain substring in order to be meaningful.  Add `fast:substring` to those rules.  For more advanced use, specify AC patterns (see previous section).  For example, to require `foo` in a case insensitive manner, use `fast:\if\io\io`.

If there is no single required substring but instead a small number of alternatives, you can use multiple fast modifiers.  E.g., for a regular expression `foo|bar`, consider `fast:foo fast:bar`.

See the next section for a way to largely automate this task.

**Step 2**: Build the automata.

In order for IronBee to take advantage of fast modifiers, it needs the corresponding automata.  This automata is an IronAutomata Eudoxus file with specific metadata.  The easiest way to build it is to run `fast/build.rb` (currently this must be run in the *object tree* `fast` directory) with a single argument specifying the rules file.  It will generate a bunch of build artifacts, including a `.e` file suitable for loading into IronBee.  The script will work with Waggle rule files as well so long as they end in `.lua` or `.waggle` and the `ruby-lua` gem is installed.

Note that you must be run `build.rb` on a platform of the same endianness as where you intend to run IronBee.

Here is an example run:

	obj/fast> ../../ironbee/fast/build.rb test.txt
	Extracting rules from test.txt to test.txt.manifest
	  .../fast/extract.rb
	Generating AC automata from test.txt.manifest to test.txt.automata
	  ./generate
	Optimizing automata from test.txt.automata to test.txt.optimized
	  ../automata/bin/optimize --translate-nonadvancing-structural
	Translate Nonadvancing [structural]: 6
	Compiling optimized from test.txt.optimized to test.txt.e
	  ../automata/bin/ec -i test.txt.automata -o test.txt.e -h 0.5
	bytes            = 1993
	id_width         = 2
	align_to         = 1
	high_node_weight = 0.5
	ids_used         = 489
	padding          = 0
	low_nodes        = 177
	low_nodes_bytes  = 1361
	high_nodes       = 1
	high_nodes_bytes = 77
	pc_nodes         = 2
	pc_nodes_bytes   = 16
	bytes @ 1        = 1504
	bytes @ 2        = 1993
	bytes @ 4        = 2971
	bytes @ 8        = 4927

During this run the following files were created:

- `test.txt.manifest`: The patterns and rule ids.  Human readable.
- `test.txt.automata`: The initial automata.  This automata can be viewed as a GraphViz dot file via `automata/bin/to_dot`.
- `test.txt.optimized`: The automata after some optimizations.  This automata can also be viewed via `to_dot` but may be more confusing.
- `test.txt.e`: The result of compiling `test.txt.optimized` via the Eudoxus Compiler (`ec`).  This file is what you will load into IronBee.

Note that `bytes = 1993` line.  This line shows the space (RAM) cost of using fast patterns over normal IronBee.

**Step 3**: Tell IronBee about the automata.

IronBee must be told to use the fast pattern system and about the automata you built in step 2.  Make sure you load the `fast` module.  Then use the `FastAutomata` directive to provide the path to the `.e` file you built in step 2.

At present, you should use a single automata built from every fast pattern rule, regardless of phase or context.  The fast pattern system will filter the results of the automata execution to only evaluate rules appropriate to the current context and phase.  The current assumption is that a single automata plus filtering is better choice in terms of space and time than per-context/phase automata.  This assumption may be incorrect or such usage may be too onerous to users.  As such, this behavior may change in the future.

suggest.rb
----------

*Overview*

There is a script, `fast/suggest.rb` which takes rules on standard in and outputs the rules to standout with additional comments suggesting fast patterns based on regular expressions in the rule.  It requires the `regexp_parser` gem which can be installed via `gem install regexp_parser`.

Comments will generally be of the form:

    # FAST RE: ...
    # FAST Suggest: ...
    # FAST Result Table:
    # FAST ...

Followed by the rule the comments apply to.

Result tables will only be displayed for certain regular expressions (see below).

The RE comment will display the regular expression of the rule that the suggestion and result table apply to.  Rules containing multiple regular expressions will have multiple comment sets.

The Suggest comment display what it believes is the best fast pattern settings for that regular expression.  In most cases, you should check the suggestion for sanity and then copy the modifiers to the rule.

The Result table displays the complete set of suggestions that the suggestion code found.  It is formatted as a boolean expression.  Each line is a ANDed clause of patterns and all the lines are joined together with OR.  The fast system evaluates a rule if any of the fast patterns for that rule are found.  As such, there is no way to specify the AND relationship.  As such, to generate fast patterns for the rule, a single pattern from each line needs to be chosen and added as a fast modifier.  The Suggest line is simply one such choice that the suggestion code believes is the best.

Suggest can also be used with Lua/Waggle rules.  Pass `--lua` in as an argument to `suggest.rb`.  It can also be used directly with regexps: pass `--rx` in as an argument and provide one regexp per line on stdin.

*Suggestions*

It is important to check the suggestions for sanity for two reasons.  First, this code is in an early state and may get things wrong.  Second, if your regular expression is incorrect, this may be obvious in the patterns.  For example, consider the regular expression:

    HeaderName:\s*Value1|Value2

The comments are:

    # FAST RE: HeaderName:\s*Value1|Value2
    # FAST Suggest: "fast:HeaderName:" "fast:Value2"
    # FAST Result Table:
    # FAST ( Value1 AND HeaderName: ) OR
    # FAST ( Value2 )

The lack of Value1 in the suggestion suggests something is awry.  Further investigation reveals the problem: the regular expression is equivalent to

    (?:HeaderName:\s*Value1)|(?:Value2)

instead of the intended

    HeaderName:\s*(?:Value1|Value2)

Correcting it changes the comment to:

    # FAST RE: HeaderName:\s*(?:Value1|Value2)
    # FAST Suggest: "fast:HeaderName:"
    # FAST Result Table:
    # FAST ( Value1 AND HeaderName: ) OR
    # FAST ( Value2 AND HeaderName: )

A much more reasonable suggestion and table.

*RX Mode*

The suggest.rb script can be run as `suggest.rb --rx` in which case each line is treated as a single regular expression (in normal mode, the input is treated as a rules file).  This mode can be useful for development and debugging.

*Exceptions*

In addition to the comments described above, you may see

    # FAST Exception: ...

comments.  These either indicate a bug in the suggestion code or a known limitation.  Please report them to the author.

*Why no comments?*

Some rules that contain regular expressions will not receive comments.  There are three major reasons for this:

1. The rule already has a fast modifier.
2. The rule has a transformation modifier, `t:`.  At present, the fast system has no support for transformations.
3. The suggestion code could not find a reasonable fast pattern for the regexp.  Very short patterns may not be worthwhile and are not suggested.  If any row of the result table contains only such short patterns, then there is no suggestion.

*Hard Limitations*

The suggestion code understands a large portion of regular expressions.  However, there are a few notable limitations.  Some of these are due to limitations in the third party regular expression parser.  Others may be solved in the future.  See `re_to_ac.rb` for details.

- \cX is not supported for X non-alpha.
- Only the i option is supported.
- No unicode support.
- Many pcre specific features are not supported.
- Back references are not supported.
- Stacked quantifiers, e.g., `x{2}{3}`, will result in suboptimal patterns.  Add non-capturing groups to fix, e.g., `(?:x{2}){3}`.

*Soft Limitations*

The suggestion code can handle quantifiers and alternations, but can have poor results in certain combinations.  This behavior is due to a fundamental mismatch between the fixed width nature of AC patterns and the variable width nature of quantifiers and alternations.

Expressions with many alternations will be ignored, e.g., `a|b|c|d|e|f|g`.

Expressions with many repetitions will be treated as having fewer, e.g., `a{1000}` will be treated as shorter, e.g., `a{10}`.

Expressions quantifying alterations may result in ridiculous result tables, e.g., `(a|b){100}` is highly inadvisable.

Try to avoid combining quantifiers with high minimums and alternations.  When using alternations, try to pull common parts out.  For example, use `foo(?:bar|baz)` instead of `foobar|foobaz`.

*To Learn More*

To learn more, including an overview of how the suggestion code works, look at the comments and code in `re_to_ac.rb`.

Advanced Usage
--------------

Advanced users may want to tune their automata further.  Additional optimizations can be attempted and different space/time tradeoffs taken.  Users should be familiar with IronAutomata and the options available, especially high node weight in `ec`.  The initial automata can be generated from the manifest via the `fast/generate`.  That automata can then be optimized and compiled in whatever manner desired so long as an equivalent Eudoxus automata is the end result.

Performance Notes
-----------------

The underlying automata should execute in `O(n)` time where `n` is the size of the transaction data.  Given an automata execution that results in `k` rules, an additional `O(k)` time is needed to filter the rules down to the `k' <= k` rules appropriate to the phase and context.  Finally, `O(k')` time is needed to evaluate and potentially execute the rules.  In contrast, default IronBee uses `O(m)` time (where `m` is the number of rules in the current phase and context) to select, evaluate, and execute rules.  Thus fast pattern rules provide an advantage where `m` is large and `k` is small.  Such a situation occurs when there are many specific rules.  If you have a small rule set, or most of your rules are very general, default IronBee is likely the better choice.

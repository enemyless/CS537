#!/bin/bash

prog='./mysort'

[ -x "$prog" ] || {
	echo >&2 "$prog not found -- do you need to compile it?"
	exit 1
}

fail() { echo >&2 "Test '$1' failed"; success=false; }
runtest()
{
	cmp -s <(echo "$2" | "$prog") <(echo "$3") || fail "$1"
	cmp -s <(echo "$2" | "$prog" -r) <(echo "$3" | tac) || fail "$1 (with '-r')"
	cmp -s <(echo "$2" | "$prog" -n1) <(echo "$3" | head -n1) || fail "$1 (with '-n1')"
}

success=true

#
# In the below test cases, the *_in variables are the provided test
# inputs and the *_out variables are the expected (correct) output.
# Each is run once with no flags, once with '-r', and once with '-n1'.
# (The alterations to the correct output for the '-r' and '-n1' cases
# are done by the 'tac' and 'head -n1' commands in the 'runtest'
# function above.)
#

# minimal: about as simple as can be.
minimal_in='c
b
a'
minimal_out='a
b
c'

runtest "minimal" "$minimal_in" "$minimal_out"

# case: sorting must be case sensitive (upper case before lower)
case_in='q
Z
a'
case_out='Z
a
q'

runtest "case" "$case_in" "$case_out"

# multiword: don't split or parse lines, leave them as-is!
multiword_in='some lines have multiple words
othersdont
does not matter
just sort the sequence of lines'
multiword_out='does not matter
just sort the sequence of lines
othersdont
some lines have multiple words'

runtest "multiword" "$multiword_in" "$multiword_out"

if $success; then
	echo "Basic tests passed!"
	echo "You'll need to do more thorough testing of your own as well though."
	echo "(Use valgrind, check edge & corner cases.)"
fi
$success

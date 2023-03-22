#!/bin/bash

# A lua linter for PRs powered by luacheck
#
# When submitting a PR from, say, "my-feature-branch" to "main", run this
# like the following"
#    $ ./scripts/luacheckpatch.sh main my-feature-branch
#
# To install luacheck:
#    $ sudo apt-get install luarocks && sudo luarocks install luacheck
#
# Must be run from the top of the repository

ADDED_FILE_PREFIX="+++ b/"
error=0

if [ $# -ne 2 ]
then
    echo 'usage: $0 <from-sha1> <to-sha1>' 1>&2
    echo 'e.g.: $0 main HEAD' 1>&2
    exit 1
fi
from="$1"
to="$2"

requirements="luacheck git sed cut"
for requirement in $requirements
do
    if [ "$(command -v "$requirement")" == "" ]
    then
        echo "Please install '$requirement'" 1>&2
        error=1
    fi
done
if [ $error -eq 1 ]
then
    exit $error
fi

declare -A changes

while read -r line
do
    if [[ "$line" == "$ADDED_FILE_PREFIX"* ]]
    then
        file="${line#"$ADDED_FILE_PREFIX"}"
    elif [[ "$line" == "@@ "* ]]
    then
        line_nrs="$(echo "$line" | sed -e 's/^@@ -[0-9]*[,]*[0-9]* +\([0-9]*\)[,]*\([0-9]*\) .*/\1 \2 /' -e 's/  / 1/')"
        start="$(echo $line_nrs | cut -d " " -f1)"
        count="$(echo $line_nrs | cut -d " " -f2)"
        if [ "${changes["$file"]+xyz}" ]
        then
            changes["$file"]="${changes["$file"]} $(seq -s' ' $start $(( $start + $count - 1 )))"
        else
            changes["$file"]="$(seq -s' ' $start $(( $start + $count - 1 )))"
        fi
    fi
done < <(git diff -U0 "$from" "$to" -- '*.lua')

while read -r line
do
    file="$(echo "$line" | cut -d: -f1)"
    line_nr="$(echo "$line" | cut -d: -f2)"
   
    if [ "${changes["$file"]+xyz}" ]
    then
        lines="${changes["$file"]}"
        if [[ $lines == "$line_nr "* ]] ||
           [[ $lines == *" $line_nr "* ]] || [[ $lines == *" $line_nr" ]] ||
           [ "$lines" == "$line_nr" ]
        then
            echo "WARN: $line"
            error=1
        fi
    fi
done < <(luacheck --no-color --formatter=plain --allow-defined-top --no-max-line-length $(find -name '*.lua'))

exit $error

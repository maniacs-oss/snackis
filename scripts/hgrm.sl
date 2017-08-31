#!/usr/local/bin/snabel

"hgrm.sl"

"Reads text from STDIN and writes N most frequent words longer than M"
"ordered by frequency to STDOUT.                                     "

"Usage:                 "
"cat *.txt | hgrm.sl N M"

let: min-wlen stoi64; _
let: max-len stoi64; _
let: tbl Str I64 table;

stdin read unopt words unopt
{len @min-wlen gte? $1 _} filter
{@tbl $1 1 &+1 upsert _} for
@tbl list {right $1 right lt?} sort
@max-len nlist {unzip '$1\t$0' say _ _} for
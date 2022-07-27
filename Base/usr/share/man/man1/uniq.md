## Name

uniq - filter out repeated adjacent lines

## Synopsis

```**sh
$ uniq [-c] [-d|-u] [-f nfields] [-s nchars] [--version] [INPUT] [OUTPUT]
```

## Description

Filter out repeated adjacent lines from INPUT (or standard input) and write to OUTPUT (or standard output). It is recommended to sort out the input using `sort(1)`.

## Options

* `-c`, `--count`: Precede each line with its number of occurences.
* `-d`, `--repeated`: Only print repeated lines.
* `-u`, `--unique`: Only print unique lines (default).
* `-f N`, `--skip-fields N`: Skip first N fields of each line before comparing.
* `-c N`, `--skip-chars N`: Skip first N chars of each line before comparing.
* `--help`: Display help message and exit.
* `--version`: Print version.

## Examples

```sh
# Filter out repeated lines from README.md and write to standard output
$ uniq README.md

# Filter out repeated lines from README.md and write to UNIQUE.md
$ uniq README.md UNIQUE.md

# Filter out repeated lines from standard input with their occurence count
$ echo "Well\nWell\nWell\nHello Friends!" | uniq -c
3 Well
1 Hello Friends!
```

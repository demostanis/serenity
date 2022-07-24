/*
 * Copyright (c) 2020, Matthew L. Curry <matthew.curry@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/RefPtr.h>
#include <AK/StringView.h>
#include <LibCore/ArgsParser.h>
#include <LibCore/Stream.h>
#include <LibCore/System.h>
#include <unistd.h>

static Optional<String> get_line_content(StringView line, size_t count, bool show_duplicates, bool print_count)
{
    if (!show_duplicates || count > 0) {
        if (print_count)
            return String::formatted("{} {}\n", count + 1, line);
        else
            return String::formatted("{}\n", line);
    }
    return {};
}

static StringView skip(StringView buf, unsigned int nchars, unsigned int nfields)
{
    StringView tmp(buf);
    tmp = tmp.trim("\n"sv);

    if (nfields) {
        bool in_field = false;
        auto fields = tmp.find_all([&](char c) {
            if (c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\r')
                in_field = false;
            else if (!in_field) {
                in_field = true;
                return true;
            }
            return false;
        });
        if (nfields >= fields.size())
            nfields = fields.size() - 1;
        tmp = tmp.substring_view(fields[nfields]);
    }
    if (tmp.length() < nchars)
        nchars = tmp.length();

    tmp = tmp.substring_view(nchars);
    return tmp;
}

// Borrowed from cmp.cpp, there should probably be a simpler way
static ErrorOr<NonnullOwnPtr<Core::Stream::BufferedFile>> open_file_or_stdin(String const& filename)
{
    OwnPtr<Core::Stream::File> file;
    if (filename.is_empty() || filename == "-")
        file = TRY(Core::Stream::File::adopt_fd(STDIN_FILENO, Core::Stream::OpenMode::Read));
    else
        file = TRY(Core::Stream::File::open(filename, Core::Stream::OpenMode::Read));
    return TRY(Core::Stream::BufferedFile::create(file.release_nonnull()));
}

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    TRY(Core::System::pledge("stdio rpath wpath cpath"));

    StringView inpath;
    StringView outpath;
    bool duplicates = false;
    bool unique = false;
    bool ignore_case = false;
    bool print_count = false;
    unsigned int skip_chars = 0;
    unsigned int skip_fields = 0;

    Core::ArgsParser args_parser;
    args_parser.add_option(duplicates, "Only print duplicated lines", "repeated", 'd');
    args_parser.add_option(unique, "Only print unique lines (default)", "unique", 'u');
    args_parser.add_option(ignore_case, "Ignore case when comparing lines", "ignore-case", 'i');
    args_parser.add_option(print_count, "Prefix each line by its number of occurences", "count", 'c');
    args_parser.add_option(skip_chars, "Skip N chars", "skip-chars", 's', "N");
    args_parser.add_option(skip_fields, "Skip N fields", "skip-fields", 'f', "N");
    args_parser.add_positional_argument(inpath, "Input file", "input", Core::ArgsParser::Required::No);
    args_parser.add_positional_argument(outpath, "Output file", "output", Core::ArgsParser::Required::No);
    args_parser.parse(arguments);

    if (!unique && !duplicates)
        unique = true;
    else if (unique && duplicates)
        // Printing duplicated and unique lines shouldn't print anything
        exit(0);

    if (outpath.is_empty() || outpath == "-"sv)
        outpath = "/dev/stdout"sv;

    auto infile = TRY(open_file_or_stdin(inpath));
    auto outfile = TRY(Core::Stream::File::open(outpath, Core::Stream::OpenMode::Write));

    size_t count = 0;
    StringView current;
    ByteBuffer previous_buf = TRY(ByteBuffer::create_zeroed(1024));
    ByteBuffer current_buf = TRY(ByteBuffer::create_zeroed(1024));

    StringView previous = TRY(infile->read_line(previous_buf));
    StringView previous_to_compare = skip(previous, skip_chars, skip_fields);

    while (TRY(infile->can_read_line())) {
        current = TRY(infile->read_line(current_buf));

        StringView current_to_compare = skip(current, skip_chars, skip_fields);
        if (ignore_case ? (!current_to_compare.equals_ignoring_case(previous_to_compare)) : (current_to_compare != previous_to_compare)) {
            auto maybe_line_content = get_line_content(previous, count, duplicates, print_count);
            if (maybe_line_content.has_value())
                TRY(outfile->write(maybe_line_content.value().bytes()));
            count = 0;
        } else
            count++;
        swap(current_to_compare, previous_to_compare);
        swap(current_buf, previous_buf);
        swap(current, previous);
    }

    auto maybe_line_content = get_line_content(previous, count, duplicates, print_count);
    if (maybe_line_content.has_value())
        TRY(outfile->write(maybe_line_content.value().bytes()));

    infile->close();
    outfile->close();

    return 0;
}

#! /usr/bin/env python3

from functools import reduce
from string import Template
from sys import argv

import re

def printf_escape(page):
    return page.replace('%', '%%').replace('\n', '\\r\\n').replace('\t', '\\t').replace('"', '\\"')

def find_replacements(page):
    return [
        match.split(':')
        for match in re.findall(r'(?<=\$\{)[\w:.]*(?=\})', page)]

def create_c_format_string(page):
    return re.sub(r'\$\{\w*\:\w*\:([\w.]*)\}', r'%\1', page)

def gen_struct_code(page_name, page):

    replacements = find_replacements(page)
    fields = reduce(
        lambda a,b: a+b,
        ['\n\t{} {};'.format(r[1], r[0]) for r in replacements])

    return Template(
"""struct webpage_$page_name
{$fields
};""").substitute({'page_name':page_name, 'fields':fields})


def gen_sprintf_code(page_name, page):

    c_fmt_string = create_c_format_string(page)
    replacements = find_replacements(page)

    fields = reduce(
        lambda a,b: a+b,
        [', page->{}'.format(r[0]) for r in replacements])

    return Template(
"""#include <stdio.h>
int print_webpage_$page_name(struct webpage_$page_name *page, char *buf, int max_size)
#ifdef GENERATED_SOURCE
{
    return snprintf(buf, max_size, "HTTP/1.1 200 OK\\r\\nConnection: close\\r\\n\\r\\n" "$c_fmt_string"$fields);
}
#else
;
#endif""").substitute(
        {'page_name':page_name, 'fields':fields, 'c_fmt_string':c_fmt_string})


if __name__ == "__main__":
    if len(argv) != 3:
        raise Exception("Args must be struct name and file name");
    with open(argv[2], 'r') as f:
        page = printf_escape(f.read())
        print(gen_struct_code(argv[1], page))
        print(gen_sprintf_code(argv[1], page))
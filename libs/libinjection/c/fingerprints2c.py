#!/usr/bin/env python
#
#  Copyright 2012, Nick Galbreath
#  nickg@client9.com
#  BSD License -- see COPYING.txt for details
#

from sqlparse_map import *

print """
#ifndef _SQLPARSE_FINGERPRINTS_H
#define _SQLPARSE_FINGERPRINTS_H

"""

with open('fingerprints.txt', 'r') as fd:
    sqlipat = [ line.strip() for line in fd ]

print 'static const char* patmap[] = {'
for k in sorted(sqlipat):
    print '    "%s",' % (k,)
print '};'
print 'static const size_t patmap_sz = %d;' % (len(sqlipat))
print

print """
/* Simple binary search */
bool is_sqli_pattern(const char *key)
{
    int pos;
    int left = 0;
    int right = (int)patmap_sz - 1;
    int cmp = 0;

    while (left <= right) {
        pos = (left + right) / 2;
        cmp = strcmp(patmap[pos], key);
        if (cmp == 0) {
            return true;
        } else if (cmp < 0) {
            left = pos + 1;
        } else {
            right = pos - 1;
        }
    }
    return false;
}
"""

print "#endif"

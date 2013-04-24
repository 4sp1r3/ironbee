#!/usr/bin/env python
#
#  Copyright 2012, 2013 Nick Galbreath
#  nickg@client9.com
#  BSD License -- see COPYING.txt for details
#

"""
Data for libinjection.   These are simple data structures
which are exported to JSON.  This is done so comments can be
added to the data directly (JSON doesn't support comments).
"""

KEYWORDS = {
'UTL_INADDR.GET_HOST_ADDRESS': 'f',

# http://blog.red-database-security.com/2009/01/17/tutorial-oracle-sql-injection-in-webapps-part-i/print/
'DBMS_PIPE.RECEIVE_MESSAGE':   'f',
'CTXSYS.DRITHSX.SN': 'f',
'SYS.STRAGG': 'f',
'ABS'                         : 'f',
'ACCESSIBLE'                  : 'k',
'ACOS'                        : 'f',
'ADD'                         : 'k',
'ADDDATE'                     : 'f',
'ADDTIME'                     : 'f',
'AES_DECRYPT'                 : 'f',
'AES_ENCRYPT'                 : 'f',
'AGAINST'                     : 'k',
'ALTER'                       : 'k',

# 'ALL_USERS' - oracle
'ALL_USERS'                   : 'k',

'ANALYZE'                     : 'k',
'AND'                         : '&',
'AS'                          : 'k',
'ASC'                         : 'k',
'ASCII'                       : 'f',
'ASENSITIVE'                  : 'k',
'ASIN'                        : 'f',
'ATAN'                        : 'f',
'ATAN2'                       : 'f',
'AVG'                         : 'f',
'BEFORE'                      : 'k',
'BEGIN'                       : 'k',
'BENCHMARK'                   : 'f',
'BETWEEN'                     : 'k',
'BIGINT'                      : 'k',
'BIN'                         : 'f',
'BINARY'                      : 'k',
'BINARY_DOUBLE_INFINITY'      : '1',
'BINARY_DOUBLE_NAN'           : '1',
'BINARY_FLOAT_INFINITY'       : '1',
'BINARY_FLOAT_NAN'            : '1',
'BINBINARY'                   : 'f',
'BIT_AND'                     : 'f',
'BIT_COUNT'                   : 'f',
'BIT_LENGTH'                  : 'f',
'BIT_OR'                      : 'f',
'BIT_XOR'                     : 'f',
'BLOB'                        : 'k',
'BOOLEAN'                     : 'k',
'BOTH'                        : 'k',
'BY'                          : 'n',
'CALL'                        : 'k',
'CASCADE'                     : 'k',
'CASE'                        : 'o',
'CAST'                        : 'f',
'CEIL'                        : 'f',
'CEILING'                     : 'f',
'CHANGE'                      : 'k',

# 'CHAR'
# sometimes a function too
'CHAR'                        : 'f',

'CHARACTER'                   : 'k',
'CHARACTER_LENGTH'            : 'f',
'CHARSET'                     : 'f',
'CHAR_LENGTH'                 : 'f',
'CHECK'                       : 'k',
'CHR'                         : 'f',
'COALESCE'                    : 'k',
'COERCIBILITY'                : 'f',
'COLLATE'                     : 'k',
'COLLATION'                   : 'f',
'COLUMN'                      : 'k',
'COMPRESS'                    : 'f',
'CONCAT'                      : 'f',
'CONCAT_WS'                   : 'f',
'CONDITION'                   : 'k',
'CONNECTION_ID'               : 'f',
'CONSTRAINT'                  : 'k',
'CONTINUE'                    : 'k',
'CONV'                        : 'f',
'CONVERT'                     : 'f',
'CONVERT_TZ'                  : 'f',
'COS'                         : 'f',
'COT'                         : 'f',
'COUNT'                       : 'f',
'CRC32'                       : 'f',
'CREATE'                      : 'k',
'CURDATE'                     : 'f',
'CURRENT_DATE'                : 'k',
'CURRENT_DATECURRENT_TIME'    : 'f',
'CURRENT_TIME'                : 'k',
'CURRENT_TIMESTAMP'           : 'k',
'CURRENT_USER'                : 'k',
'CURSOR'                      : 'k',
'CURTIME'                     : 'f',
'DATABASE'                    : 'k',
'DATABASES'                   : 'k',
'DATE'                        : 'f',
'DATEDIFF'                    : 'f',
'DATE_ADD'                    : 'f',
'DATE_FORMAT'                 : 'f',
'DATE_SUB'                    : 'f',
'DAY'                         : 'f',
'DAYNAME'                     : 'f',
'DAYOFMONTH'                  : 'f',
'DAYOFWEEK'                   : 'f',
'DAYOFYEAR'                   : 'f',
'DAY_HOUR'                    : 'k',
'DAY_MICROSECOND'             : 'k',
'DAY_MINUTE'                  : 'k',
'DAY_SECOND'                  : 'k',
'DEC'                         : 'k',
'DECIMAL'                     : 'k',
'DECLARE'                     : 'k',
'DECODE'                      : 'f',
'DEFAULT'                     : 'k',
'DEGREES'                     : 'f',
'DELAY'                       : 'k',
'DELAYED'                     : 'k',
'DELETE'                      : 'k',
'DESC'                        : 'k',
'DESCRIBE'                    : 'k',
'DES_DECRYPT'                 : 'f',
'DES_ENCRYPT'                 : 'f',
'DETERMINISTIC'               : 'k',
'DISTINCROW'                  : 'k',
'DISTINCT'                    : 'k',
'DIV'                         : 'o',
'DROP'                        : 'k',
'DUAL'                        : 'k',
'EACH'                        : 'k',
'ELSE'                        : 'k',
'ELSEIF'                      : 'k',
'ELT'                         : 'f',
'ENCLOSED'                    : 'k',
'ENCODE'                      : 'f',
'ENCRYPT'                     : 'f',
'ESCAPED'                     : 'k',

# TBD
#'END'                         : 'k',

# 'EXEC' - MSSQL
#
'EXEC'                        : 'k',
'EXECUTE'                     : 'k',
'EXISTS'                      : 'k',
'EXIT'                        : 'k',
'EXP'                         : 'f',
'EXPLAIN'                     : 'k',
'EXPORT_SET'                  : 'f',
'EXTRACT'                     : 'f',
'EXTRACTVALUE'                : 'f',
'EXTRACT_VALUE'               : 'f',
'FALSE'                       : '1',
'FETCH'                       : 'k',
'FIELD'                       : 'f',
'FIND_IN_SET'                 : 'f',
'FLOOR'                       : 'f',
'FORCE'                       : 'k',
'FOREIGN'                     : 'k',
'FOR'                         : 'n',
'FORMAT'                      : 'f',
'FOUND_ROWS'                  : 'f',
'FROM'                        : 'k',
'FROM_DAYS'                   : 'f',
'FROM_UNIXTIME'               : 'f',
'FULLTEXT'                    : 'k',
'GENERATE_SERIES'             : 'f',
'GET_FORMAT'                  : 'f',
'GET_LOCK'                    : 'f',
'GOTO'                        : 'k',
'GRANT'                       : 'k',
'GREATEST'                    : 'f',
'GROUP'                       : 'n',
'GROUP_CONCAT'                : 'f',

#
# 'HAVING' - MSSQL
'HAVING'                      : 'k',

'HEX'                         : 'f',
'HIGH_PRIORITY'               : 'k',
'HOUR'                        : 'f',
'HOUR_MICROSECOND'            : 'k',
'HOUR_MINUTE'                 : 'k',
'HOUR_SECOND'                 : 'k',

# 'HOST_NAME' -- unknown db
'HOST_NAME'                   : 'f',

# 'IF - if is normally a function, except in TSQL
# http://msdn.microsoft.com/en-us/library/ms182717.aspx
'IF'                          : 'k',

'IFF'                         : 'f',
'IFNULL'                      : 'f',
'IGNORE'                      : 'k',
'IIF'                         : 'f',

# IN is a special case.. sometimes a function, sometimes a keyword
'IN'                          : 'n',

'INDEX'                       : 'k',
'INET_ATON'                   : 'f',
'INET_NTOA'                   : 'f',
'INFILE'                      : 'k',
'INNER'                       : 'k',
'INOUT'                       : 'k',
'INSENSITIVE'                 : 'k',
'INSERT'                      : 'k',
'INSTR'                       : 'f',
'INT'                         : 'k',
'INT1'                        : 'k',
'INT2'                        : 'k',
'INT3'                        : 'k',
'INT4'                        : 'k',
'INT8'                        : 'k',
'INTEGER'                     : 'k',
'INTERVAL'                    : 'k',
'INTO'                        : 'k',
'IS'                          : 'o',
'ISNULL'                      : 'f',
'IS_FREE_LOCK'                : 'f',
#
# 'IS_MEMBER' - MSSQL
'IS_MEMBER'
#                : 'f',
# 'IS_SRV...' MSSQL
'IS_SRVROLEMEMBER'            : 'f',
'IS_USED_LOCK'                : 'f',
'ITERATE'                     : 'k',
'JOIN'                        : 'k',
'KEYS'                        : 'k',
'KILL'                        : 'k',
'LAST_INSERT_ID'              : 'f',
'LCASE'                       : 'f',
'LEADING'                     : 'k',
'LEAST'                       : 'f',
'LEAVE'                       : 'k',
'LEFT'                        : 'n',
'LENGTH'                      : 'f',
'LIKE'                        : 'o',
'LIMIT'                       : 'k',
'LINEAR'                      : 'k',
'LINES'                       : 'k',
'LN'                          : 'f',
'LOAD'                        : 'k',
'LOAD_FILE'                   : 'f',
'LOCALTIME'                   : 'k',
'LOCALTIMESTAMP'              : 'k',
'LOCATE'                      : 'f',
'LOCK'                        : 'n',
'LOG'                         : 'f',
'LOG10'                       : 'f',
'LOG2'                        : 'f',
'LONGBLOB'                    : 'k',
'LONGTEXT'                    : 'k',
'LOOP'                        : 'k',
'LOWER'                       : 'f',
'LOW_PRIORITY'                : 'k',
'LPAD'                        : 'f',
'LTRIM'                       : 'f',
'MAKEDATE'                    : 'f',
'MAKE_SET'                    : 'f',
'MASTER_BIND'                 : 'k',
'MASTER_POS_WAIT'             : 'f',
'MASTER_SSL_VERIFY_SERVER_CERT': 'k',
'MATCH'                       : 'k',
'MAX'                         : 'f',
'MAXVALUE'                    : 'k',
'MD5'                         : 'f',
'MEDIUMBLOB'                  : 'k',
'MEDIUMINT'                   : 'k',
'MEDIUMTEXT'                  : 'k',
'MERGE'                       : 'k',
'MICROSECOND'                 : 'f',
'MID'                         : 'f',
'MIDDLEINT'                   : 'k',
'MIN'                         : 'f',
'MINUTE'                      : 'f',
'MINUTE_MICROSECOND'          : 'k',
'MINUTE_SECOND'               : 'k',
'MOD'                         : 'o',
'MODE'                        : 'n',
'MODIFIES'                    : 'k',
'MONTH'                       : 'f',
'MONTHNAME'                   : 'f',
'NAME_CONST'                  : 'f',
'NOT'                         : 'o',
'NOW'                         : 'f',
'NO_WRITE_TO_BINLOG'          : 'k',
'NULL'                        : '1',
'NULLIF'                      : 'f',
'NUMERIC'                     : 'k',
'OCT'                         : 'f',
'OCTET_LENGTH'                : 'f',
'OFFSET'                      : 'k',
'OLD_PASSWORD'                : 'f',

# need to investigate how used
#'ON'                          : 'k',
'ONE_SHOT'                    : 'k',

# obviously not SQL but used in attacks
'OWN3D'                       : 'k',

# 'OPEN'
# http://msdn.microsoft.com/en-us/library/ms190500.aspx
'OPEN'                        : 'k',

# 'OPENDATASOURCE'
# http://msdn.microsoft.com/en-us/library/ms179856.aspx
'OPENDATASOURCE'              : 'f',
'OPENXML'                     : 'f',
'OPENQUERY'                   : 'f',
'OPENROWSET'                  : 'f',
'OPTIMIZE'                    : 'k',
'OPTION'                      : 'k',
'OPTIONALLY'                  : 'k',
'OR'                          : '&',
'ORD'                         : 'f',
'ORDER'                       : 'n',
'OUT'                         : 'k',
'OUTFILE'                     : 'k',
'PARTITION'                   : 'k',

# keyword "SET PASSWORD", and a function
'PASSWORD'                    : 'k',
'PERIOD_ADD'                  : 'f',
'PERIOID_DIFF'                : 'f',
'PG_ADVISORY_LOCK'            : 'f',
'PG_SLEEP'                    : 'f',
'PI'                          : 'f',
'POSITION'                    : 'f',
'POW'                         : 'f',
'POWER'                       : 'f',
'PRECISION'                   : 'k',
'PRIMARY'                     : 'k',
'PROCEDURE'                   : 'k',
'PURGE'                       : 'k',
'QUARTER'                     : 'f',
'QUOTE'                       : 'f',
'RADIANS'                     : 'f',
'RAND'                        : 'f',

# 'RANDOMBLOB' - sqlite3
'RANDOMBLOB'                  : 'f',
'RANGE'                       : 'k',
'READ'                        : 'k',
'READS'                       : 'k',
'READ_WRITE'                  : 'k',

# 'REAL' only used in data definition
'REAL'                        : 'n',
'REFERENCES'                  : 'k',
'REGEXP'                      : 'o',
'RELEASE'                     : 'k',
'RELEASE_LOCK'                : 'f',
'RENAME'                      : 'k',
'REPEAT'                      : 'k',
'REPLACE'                     : 'k',
'REQUIRE'                     : 'k',
'RESIGNAL'                    : 'k',
'RESTRICT'                    : 'k',
'RETURN'                      : 'k',
'REVERSE'                     : 'f',
'REVOKE'                      : 'k',
'RIGHT'                       : 'n',
'RLIKE'                       : 'o',
'ROUND'                       : 'f',
'ROW'                         : 'f',
'ROW_COUNT'                   : 'f',
'RPAD'                        : 'f',
'RTRIM'                       : 'f',
'SCHEMA'                      : 'k',
'SCHEMAS'                     : 'k',
'SECOND_MICROSECOND'          : 'k',
'SEC_TO_TIME'                 : 'f',
'SELECT'                      : 'k',
'SENSITIVE'                   : 'k',
'SEPARATOR'                   : 'k',
'SESSION_USER'                : 'f',
'SET'                         : 'k',
'SHA'                         : 'f',
'SHA1'                        : 'f',
'SHA2'                        : 'f',
'SHOW'                        : 'n',
'SHUTDOWN'                    : 'k',
'SIGN'                        : 'f',
'SIGNAL'                      : 'k',
'SIMILAR'                     : 'k',
'SIN'                         : 'f',
'SLEEP'                       : 'f',
'SMALLINT'                    : 'k',
'SOUNDEX'                     : 'f',
'SOUNDS'                      : 'o',
'SPACE'                       : 'f',
'SPATIAL'                     : 'k',
'SPECIFIC'                    : 'k',
'SQL'                         : 'k',
'SQLEXCEPTION'                : 'k',
'SQLSTATE'                    : 'k',
'SQLWARNING'                  : 'k',
'SQL_BIG_RESULT'              : 'k',
'SQL_CALC_FOUND_ROWS'         : 'k',
'SQL_SMALL_RESULT'            : 'k',
'SQRT'                        : 'f',
'SSL'                         : 'k',
'STARTING'                    : 'k',
'STDDEV'                      : 'f',
'STDDEV_POP'                  : 'f',
'STDDEV_SAMP'                 : 'f',
'STRAIGHT_JOIN'               : 'k',
'STRCMP'                      : 'f',
'STR_TO_DATE'                 : 'f',
'SUBDATE'                     : 'f',
'SUBSTR'                      : 'f',
'SUBSTRING'                   : 'f',
'SUBSTRING_INDEX'             : 'f',
'SUBTIME'                     : 'f',
'SUM'                         : 'f',
'SYSDATE'                     : 'f',

# 'SYSCOLUMNS'
# http://msdn.microsoft.com/en-us/library/aa260398(v=sql.80).aspx
'SYSCOLUMNS'                  : 'k',

# 'SYSOBJECTS'
# http://msdn.microsoft.com/en-us/library/aa260447(v=sql.80).aspx
'SYSOBJECTS'                  : 'k',

# 'SYSUSERS' - MSSQL
'SYSUSERS'                    : 'k',
'SYSTEM_USER'                 : 'f',

# 'TABLE'
# because SQLi really can't use 'TABLE'
'TABLE'                       : 'k',
'TAN'                         : 'f',
'TERMINATED'                  : 'k',
'THEN'                        : 'k',
'TIME'                        : 'k',
'TIMEDIFF'                    : 'f',
'TIMESTAMP'                   : 'f',
'TIMESTAMPADD'                : 'f',
'TIME_FORMAT'                 : 'f',
'TIME_TO_SEC'                 : 'f',
'TINYBLOB'                    : 'k',
'TINYINT'                     : 'k',
'TINYTEXT'                    : 'k',
#
# 'TO_CHAR' -- oracle
'TO_CHAR'                     : 'f',
'TO_DAYS'                     : 'f',
'TO_SECONDS'                  : 'f',
'TOP'                         : 'k',

# 'TRAILING' -- only used in TRIM(TRAILING
# http://www.w3resource.com/sql/character-functions/trim.php
'TRAILING'                    : 'n',
'TRIGGER'                     : 'k',
'TRIM'                        : 'f',
'TRUE'                        : '1',
'TRUNCATE'                    : 'f',
'UCASE'                       : 'f',
'UNCOMPRESS'                  : 'f',
'UNCOMPRESS_LENGTH'           : 'f',
'UNDO'                        : 'k',
'UNHEX'                       : 'f',
'UNION'                       : 'U',

# 'UNI_ON' -- odd variation that comes up
'UNI_ON'                      : 'U',

# 'UNIQUE'
# only used as a function (DB2) or as "CREATE UNIQUE"
'UNIQUE'                      : 'n',

'UNIX_TIMESTAMP'              : 'f',
'UNLOCK'                      : 'k',
'UNSIGNED'                    : 'k',
'UPDATE'                      : 'k',
'UPDATEXML'                   : 'f',
'UPPER'                       : 'f',
'USAGE'                       : 'k',
'USE'                         : 'k',

# 'USER' -- a MySQL function?
#'USER'                       : 'k',

'USING'                       : 'f',
'UTC_DATE'                    : 'k',
'UTC_TIME'                    : 'k',
'UTC_TIMESTAMP'               : 'k',
'UUID'                        : 'f',
'UUID_SHORT'                  : 'f',
'VALUES'                      : 'k',
'VARBINARY'                   : 'k',
'VARCHAR'                     : 'k',
'VARCHARACTER'                : 'k',
'VARIANCE'                    : 'f',
'VARYING'                     : 'k',
'VAR_POP'                     : 'f',
'VAR_SAMP'                    : 'f',
'VERSION'                     : 'f',
'WAITFOR'                     : 'k',
'WEEK'                        : 'f',
'WEEKDAY'                     : 'f',
'WEEKOFYEAR'                  : 'f',
'WHEN'                        : 'k',
'WHERE'                       : 'k',
'WHILE'                       : 'k',
'WITH'                        : 'k',
# XML... oracle
'XMLELEMENT'                  : 'f',
'XMLFOREST'                   : 'f',
'XMLFORMAT'                   : 'f',
'XMLTYPE'                     : 'f',
'XOR'                         : 'o',
'XP_EXECRESULTSET'            : 'k',
'YEAR'                        : 'f',
'YEARWEEK'                    : 'f',
'YEAR_MONTH'                  : 'k',
'ZEROFILL'                    : 'k'
}


# special in that single char is a valid operator
# special case in that '<=' might also be '<=>'
# ":" isn't an operator in mysql, but other dialects
#   use it.
OPERATORS2 = (
    '!=',   # oracle
    '||',
    '&&',
    '>=',
    '>>',
    '<=',
    '<>',
    ':=',
    '<<',
    '!<', # http://msdn.microsoft.com/en-us/library/ms188074.aspx
    '!>', # http://msdn.microsoft.com/en-us/library/ms188074.aspx
    '+=',
    '-=',
    '*=',
    '/=',
    '%=',
    '|=',
    '&=',
    '^=',
    '|/', # http://www.postgresql.org/docs/9.1/static/functions-math.html
    '!!', # http://www.postgresql.org/docs/9.1/static/functions-math.html
    '~*', # http://www.postgresql.org/docs/9.1/static/functions-matching.html
    '!~', # http://www.postgresql.org/docs/9.1/static/functions-matching.html
    '@>',
    '<@'
    # '!~*'
    )

CHARMAP = [
    'CHAR_WHITE', # 0
    'CHAR_WHITE', # 1
    'CHAR_WHITE', # 2
    'CHAR_WHITE', # 3
    'CHAR_WHITE', # 4
    'CHAR_WHITE', # 5
    'CHAR_WHITE', # 6
    'CHAR_WHITE', # 7
    'CHAR_WHITE', # 8
    'CHAR_WHITE', # 9
    'CHAR_WHITE', # 10
    'CHAR_WHITE',
    'CHAR_WHITE',
    'CHAR_WHITE',
    'CHAR_WHITE',
    'CHAR_WHITE',
    'CHAR_WHITE',
    'CHAR_WHITE',
    'CHAR_WHITE',
    'CHAR_WHITE',
    'CHAR_WHITE', # 20
    'CHAR_WHITE',
    'CHAR_WHITE',
    'CHAR_WHITE',
    'CHAR_WHITE',
    'CHAR_WHITE',
    'CHAR_WHITE',
    'CHAR_WHITE',
    'CHAR_WHITE',
    'CHAR_WHITE',
    'CHAR_WHITE', #30
    'CHAR_WHITE',
    'CHAR_WHITE',
    'CHAR_OP2',   # 33 !
    'CHAR_STR',   # 34 "
    'CHAR_COM1',  # 35 "#"
    'CHAR_MONEY',  # 36 $ -- ignore optional currency symbol for TSQL money types
    'CHAR_OP1',   # 37 %
    'CHAR_OP2',   # 38 &
    'CHAR_STR',   # 39 '
    'CHAR_CHAR',  # 40 (
    'CHAR_CHAR',  # 41 )
    'CHAR_OP2',   # 42 *
    'CHAR_OP1',   # 43 +
    'CHAR_CHAR',  # 44 ,
    'CHAR_DASH',  # 45 -
    'CHAR_NUM',   # 46 .
    'CHAR_SLASH', # 47 /
    'CHAR_NUM',   # 48 0
    'CHAR_NUM',   # 49 1
    'CHAR_NUM',   # 50 2
    'CHAR_NUM',   # 51 3
    'CHAR_NUM',   # 52 4
    'CHAR_NUM',   # 53 5
    'CHAR_NUM',   # 54 6
    'CHAR_NUM',   # 55 7
    'CHAR_NUM',   # 56 8
    'CHAR_NUM',   # 57 9
    'CHAR_CHAR',  # 58 : colon
    'CHAR_CHAR',  # 59 ; semiclon
    'CHAR_OP2',   # 60 <
    'CHAR_OP2',   # 61 =
    'CHAR_OP2',   # 62 >
    'CHAR_OTHER', # 63 ?   BEEP BEEP
    'CHAR_VAR',   # 64 @
    'CHAR_WORD',  # 65 A
    'CHAR_WORD',  # 66 B
    'CHAR_WORD',  # @
    'CHAR_WORD',  # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # Z
    'CHAR_OTHER',           # [
    'CHAR_BACK',            # \\
        'CHAR_OTHER',           # ]
    'CHAR_OP1',             # ^
    'CHAR_WORD',            # _
    'CHAR_WORD',            # backtick
    'CHAR_WORD',            # A
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # @
    'CHAR_WORD',            # z
    'CHAR_OTHER',            # 123 { left brace
    'CHAR_OP2',             # 124 | pipe
    'CHAR_OTHER',            # 125 } right brace
    'CHAR_OP1',             # 126  ~
    'CHAR_WHITE'
]

PHRASES = {
    'IN BOOLEAN'        : 'n',
    'IN BOOLEAN MODE'   : 'k',
    'CROSS JOIN'        : 'k',
    'ALTER DOMAIN'      : 'k',
    'ALTER TABLE'       : 'k',
    'GROUP BY'          : 'B',
    'ORDER BY'          : 'B',
    'OWN3D BY'          : 'B',
    'SELECT ALL'        : 'k',
    'READ WRITE'        : 'k',

    # 'LOCAL TABLE' pgsql/oracle
    # http://www.postgresql.org/docs/current/static/sql-lock.html
    'LOCK TABLE'        : 'k',

    # 'LOCK TABLES' MYSQL
    #  http://dev.mysql.com/doc/refman/4.1/en/lock-tables.html
    'LOCK TABLES'       : 'k',
    'LEFT OUTER'        : 'k',
    'LEFT JOIN'         : 'k',
    'RIGHT OUTER'       : 'k',
    'RIGHT JOIN'        : 'k',
    'FULL OUTER'        : 'k',
    'NATURAL JOIN'      : 'k',
    'NATURAL INNER'     : 'k',
    'NATURAL OUTER'     : 'k',
    'NATURAL LEFT'      : 'k',
    'NATURAL RIGHT'     : 'k',
    'NATURAL FULL'      : 'k',
    'SOUNDS LIKE'       : 'o',
    'IS NOT'            : 'o',
    'NOT LIKE'          : 'o',
    'NOT BETWEEN'       : 'o',
    'NOT SIMILAR'       : 'o',

    # 'NOT RLIKE' -- MySQL
    'NOT RLIKE'         : 'o',

    'NOT REGEXP'        : 'o',
    'NOT IN'            : 'o',
    'SIMILAR TO'        : 'o',
    'NOT SIMILAR TO'    : 'o',
    'UNION ALL'         : 'U',

    # 'INTERSECT ALL' -- ORACLE
    'INTERSECT ALL'     : 'o'
    }

import json
def dump():
    """
    generates a JSON file, sorted keys
    """

    objs = {
        'keywords': KEYWORDS,
        'charmap': CHARMAP,
        'operators2': OPERATORS2,
        'phrases': PHRASES
        }
    return json.dumps(objs, sort_keys=True, indent=4)

if __name__ == '__main__':
    print dump()


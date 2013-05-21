/*
 */
#include <time.h>
#include <string.h>
#include <stdio.h>

#include "libinjection.h"

void testIsSQL(void)
{
    const char* s[] = {
        "123 LIKE -1234.5678E+2;",
        "APPLE 19.123 'FOO' \"BAR\"",
        "/* BAR */ UNION ALL SELECT (2,3,4)",
        "1 || COS(+0X04) --FOOBAR",
        "dog apple cat banana bar",
        "dog apple cat \"banana \'bar",
        "102 TABLE CLOTH",
        NULL
    };
    const int imax = 1000000;
    int i, j;
    sfilter sf;
    clock_t t0 = clock();
    for (i = imax, j=0; i != 0; --i, ++j) {
        if (s[j] == NULL) {
            j = 0;
        }
        libinjection_is_sqli(&sf, s[j], strlen(s[j]), NULL, NULL);
    }
    clock_t t1 = clock();
    double total = (double) (t1 - t0) / (double) CLOCKS_PER_SEC;
    printf("IsSQLi TPS                    = %f\n", (double) imax / total);
}

int main()
{
    testIsSQL();
    return 0;
}

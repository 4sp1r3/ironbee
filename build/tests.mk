# -Wno-unused-parameter is because IronBee is so callback heavy multiple
# implements ignore one or more of their parameters.
# In C++, we could ommit the name, but not in C.
CPPFLAGS += @IB_DEBUG@ \
            -I$(top_srcdir)/tests \
            -I$(top_srcdir)/include \
            -I$(top_srcdir)/util \
            -I$(top_srcdir)/engine \
            -I$(top_srcdir) \
            -I$(top_builddir) \
            -Wno-unused-parameter

CXXFLAGS += -g

# Alias for "check"
test: check

.PHONY: test

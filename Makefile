CXXFLAGS=-Wall -Wextra -pedantic -Wshadow -Wconversion -Wdouble-promotion
CXXFLAGS+=-Wnull-dereference -Wformat=2 -Wformat-overflow -Wformat-truncation
CXXFLAGS+=-Wswitch-default -Wformat-security -Wmissing-declarations -Winit-self
CXXFLAGS+=-Wpointer-arith -Wswitch-enum -Wundef -Wstrict-prototypes
CXXFLAGS+=-D_FORTIFY_SOURCE=1 -fstack-protector -fno-common
CXXFLAGS+=-Isrc -rdynamic -pthread
LDFLAGS=-Wl,-O1 -Wl,-z,relro -Wl,-z,now
LIBS=
PREFIX?=/usr/local

SOURCES=$(wildcard src/**/*.cpp src/*.cpp)
OBJECTS=$(patsubst %.cpp,%.o,$(SOURCES))
DEPS=$(wildcard src/**/*.h src/*.h)

TEST_SRC=$(wildcard tests/*_tests.cpp)
TESTS=$(patsubst %.cpp,%,$(TEST_SRC))

TARGET=./build/alraune

# The Target Build
all: CXXFLAGS += -O2 -DNDEBUG
all: build $(TARGET)

dev: CXXFLAGS += -Og -g
dev: build $(TARGET)

$(TARGET): $(OBJECTS) $(DEPS)
	$(CXX) $^ $(CXXFLAGS) $(LDFLAGS) $(LIBS) -o $@

build:
	@mkdir -p build

# The Unit Tests
.PHONY: tests
tests: CXXFLAGS += $(TARGET)
tests: $(TESTS)
	sh ./tests/runtests.sh

# The Cleaner
clean:
	rm -rf build $(OBJECTS) $(TESTS)
	rm -f tests/tests.log
	find . -name "*.gc*" -exec rm {} \;
	rm -rf `find . -name "*.dSYM" -print`

# The Install
install: all
	install -d $(DESTDIR)/$(PREFIX)/bin/
	install $(TARGET) $(DESTDIR)/$(PREFIX)/bin/

.PHONY: run
run:
	$(TARGET)

# The Checker
check:
	@echo Files with potentially dangerous functions.
	@egrep '[^_.>a-zA-Z0-9](str(n?cpy|n?cat|xfrm|n?dup|str|pbrk|tok|_)\|stpn?cpy|a?sn?printf|byte_)' $(SOURCES) || true


CXX=g++
CXXFLAGS=-g -O2 -Wall -Wextra -Isrc -rdynamic -DNDEBUG -pthread
LIBS=
PREFIX?=/usr/local

SOURCES=$(wildcard src/**/*.cpp src/*.cpp)
OBJECTS=$(patsubst %.cpp,%.o,$(SOURCES))
DEPS=$(wildcard src/**/*.h src/*.h)

TEST_SRC=$(wildcard tests/*_tests.cpp)
TESTS=$(patsubst %.cpp,%,$(TEST_SRC))

TARGET=./build/alraune

# The Target Build
all: build $(TARGET) tests

dev: CXXFLAGS=-g -Wall -Isrc -Wall -Wextra -pthread $(OPTFLAGS)
dev: all

$(TARGET): CXXFLAGS += -fPIC
$(TARGET): $(OBJECTS) $(DEPS)
	$(CXX) $^ $(CXXFLAGS) $(LIBS) -o $@

build:
	@mkdir -p build
	@mkdir -p bin

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
	install -d $(DESTDIR)/$(PREFIX)/lib/
	install $(TARGET) $(DESTDIR)/$(PREFIX)/lib/

# The Checker
check:
	@echo Files with potentially dangerous functions.
	@egrep '[^_.>a-zA-Z0-9](str(n?cpy|n?cat|xfrm|n?dup|str|pbrk|tok|_)\
		|stpn?cpy|a?sn?printf|byte_)' $(SOURCES) || true


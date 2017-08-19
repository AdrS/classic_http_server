EXECUTABLE 	= httpserver
CC			= gcc

# list of test drivers (with main()) for development
TESTSOURCES = $(wildcard test_*.c)
# names of test executables
TESTS       = $(TESTSOURCES:%.c=%)

# list of sources used in project
SOURCES 	= $(wildcard *.c)
SOURCES     := $(filter-out $(TESTSOURCES), $(SOURCES))
# list of objects used in project
OBJECTS		= $(SOURCES:%.c=%.o)

# If main() is in another file delete the line above, edit and uncomment below
PROJECTFILE = http_server.c

#Default Flags
CCFLAGS = -Wconversion -Wall -Werror -Wextra

# make release - will compile "all" with $(CCFLAGS) and the -O3 flag
#				 also defines NDEBUG so that asserts will not check
release: CCFLAGS += -O3 -DNDEBUG
release: all

# make debug - will compile "all" with $(CCFLAGS) and the -g flag
#              also defines DEBUG so that "#ifdef DEBUG /*...*/ #endif" works
debug: CCFLAGS += -g3 -DDEBUG
debug: clean all

# make profile - will compile "all" with $(CCFLAGS) and the -pg flag
profile: CCFLAGS += -pg
profile: clean all

# highest target; sews together all objects into executable
all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
ifeq ($(EXECUTABLE), executable)
	@echo Edit EXECUTABLE variable in Makefile.
	@echo Using default a.out.
	$(CC) $(CCFLAGS) $(OBJECTS)
else
	$(CC) $(CCFLAGS) $(OBJECTS) -lpthread -o $(EXECUTABLE)
endif

# Automatically generate any build rules for test*.c files
define make_tests
    ifeq ($$(PROJECTFILE),)
	    @echo Edit PROJECTFILE variable to .c file with main\(\)
	    @exit 1
    endif
    TMP = $$(filter-out $$(PROJECTFILE), $$(SOURCES))
    SRCS = $$(filter-out threaded_server.c, $$(TMP))
    OBJS = $$(SRCS:%.c=%.o)
    HDRS = $$(wildcard *.h)
    $(1): CCFLAGS += -g3 -DDEBUG
    $(1): $$(OBJS) $$(HDRS) $(1).c
	$$(CC) $$(CCFLAGS) $$(OBJS) $(1).c -o $(1)
endef
$(foreach test, $(TESTS), $(eval $(call make_tests, $(test))))

alltests: $(TESTS)

# rule for creating objects
%.o: %.c
	$(CC) $(CCFLAGS) -c $*.c

# make clean - remove .o files, executables
clean:
	rm -f $(OBJECTS) $(EXECUTABLE) $(TESTS)

runtests: alltests
	./test_url
	./test_util
	./test_encoding_prefs

#######################
# individual dependencies for objects

http_server.o: http_server.c http.h threaded_server.h

http.o: http.c http.h util.h encoding_prefs.h url.h files.h

test_encoding_prefs.o: test_encoding_prefs.c encoding_prefs.h
test_util.o: test_util.c util.h
test_url.o: test_url.c url.h

######################

# these targets do not create any files
.PHONY: all release debug profile clean alltests
# disable built-in rules
.SUFFIXES:

GCC_COMPILER = gcc
GCC_COMPILER_FLAGS = -ggdb -Wall

XML_INCS = `xml2-config --cflags`
XML_LIBS = `xml2-config --libs`

MISC_LIBS = -lm -lrt -lpthread
MISC_LIBS =

INCS = $(XML_INCS)
LIBS = $(XML_LIBS) $(MISC_LIBS)

TARGET = osc2sql

SRC_DIRS := .
SRC_FILES := $(foreach DIR, $(SRC_DIRS), $(wildcard $(DIR)/*.c))
OBJS := $(patsubst %.c, %.o, $(SRC_FILES))

all : $(TARGET)
	@echo All done

#uses http://wiki.openstreetmap.org/wiki/Osmconvert
osc:
	./osmconvert/osmconvert $(OLD_DATA) $(NEW_DATA) --diff -o=changefile.osc
sql:
	./osc2sql -i changefile.osc -o update_diff.sql

$(TARGET) : $(OBJS)
	$(GCC_COMPILER) $(GCC_COMPILER_FLAGS) -o $@ $^ $(LIBS)

%.o : %.c
	$(GCC_COMPILER) $(GCC_COMPILER_FLAGS) -o $@ -c $(INCS) $<

clean :
	rm -f $(OBJS) $(TARGET)
	@echo Clean done

.PHONY: clean


# $^ test.o / test.cu
# $@ test
# $< test.o / test.cu

HOST_GCC=g++
TARGET_GCC=gcc
PLUGIN_SOURCE_FILES= src/flattenstruct.c
GCCPLUGINS_DIR:= $(shell $(TARGET_GCC) -print-file-name=plugin)
CXXFLAGS+= -I$(GCCPLUGINS_DIR)/include -fPIC -fno-rtti -O0 -g3

default: flattenstruct.so

flattenstruct.so: $(PLUGIN_SOURCE_FILES)
	$(HOST_GCC) -shared $(CXXFLAGS) $^ -o $@

.PHONY: install uninstall
install: flattenstruct.so
	cp flattenstruct.so $(GCCPLUGINS_DIR)
	chmod +r $(GCCPLUGINS_DIR)/flattenstruct.so

uninstall:
	rm -f $(GCCPLUGINS_DIR)/flattenstruct.so


.PHONY: test
test: flattenstruct.so
	$(TARGET_GCC) -fplugin=./flattenstruct.so -c test/compile_test.c -o /dev/null
	# Passed Tests

.PHONY: clean
clean:
	rm -f flattenstruct.so

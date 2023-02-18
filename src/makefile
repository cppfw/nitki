include prorab.mk
include prorab-license.mk
include prorab-clang-format.mk

$(eval $(call prorab-try-simple-include, $(CONANBUILDINFO_DIR)conanbuildinfo.mak))

$(eval $(call prorab-config, ../config))

this_name := nitki

this_soname := $(shell cat $(d)soname.txt)

this_srcs := $(call prorab-src-dir, .)

this_cxxflags += $(addprefix -I,$(CONAN_INCLUDE_DIRS))
this_cxxflags_test += $(addprefix -I,$(CONAN_INCLUDE_DIRS))
this_ldflags += $(addprefix -L,$(CONAN_LIB_DIRS))

ifeq ($(os),linux)
    this_ldlibs += -lpthread
endif

this_ldlibs += -lutki -lopros

$(eval $(prorab-build-lib))

$(eval $(prorab-clang-format))

this_src_dir := 
this_license_file := ../LICENSE
$(eval $(prorab-license))

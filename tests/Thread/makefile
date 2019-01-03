include prorab.mk



this_name := tests


#compiler flags
this_cxxflags += -std=c++11
this_cxxflags += -Wall
this_cxxflags += -DDEBUG
this_cxxflags += -O3
this_cxxflags += -fstrict-aliasing #strict aliasing!!!

this_srcs += main.cpp tests.cpp

this_ldlibs += -lnitki -lpogodi -lstdc++

ifeq ($(os),macosx)
    this_cxxflags += -stdlib=libc++ #this is needed to be able to use c++11 std lib
    this_ldlibs += -lc++
else ifeq ($(os),windows)
else
    this_ldlibs += -lpthread
endif

this_ldflags += -L$(d)../../src/

#add dependency on libnitki.so
$(abspath $(d)tests): $(abspath $(d)../../src/libnitki$(soext))


$(eval $(prorab-build-app))

include $(d)../test_target.mk


#include makefile for building nitki
$(eval $(call prorab-include,$(d)../../src/makefile))

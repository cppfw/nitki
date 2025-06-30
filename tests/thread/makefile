include prorab.mk
include prorab-test.mk

$(eval $(call prorab-config, ../../config))

this_name := tests

this_srcs += $(call prorab-src-dir, .)

this_ldlibs += -lopros$(this_dbg)
this_ldlibs += -lutki$(this_dbg)

this__libnitki := ../../src/out/$(c)/libnitki$(this_dbg)$(dot_so)

this_ldlibs += $(this__libnitki)

this_no_install := true

$(eval $(prorab-build-app))

this_test_cmd := $(prorab_this_name)
this_test_deps := $(prorab_this_name)
this_test_ld_path := ../../src/out/$(c)
$(eval $(prorab-test))

# include makefile for building nitki
$(eval $(call prorab-include, ../../src/makefile))

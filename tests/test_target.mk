this_dirs := $(subst /, ,$(d))
this_test := $(word $(words $(this_dirs)),$(this_dirs))

ifeq ($(prorab_os),windows)
    this_test_cmd := (cd $(prorab_this_dir); cp ../../src/libnitki.dll . || true; ./$$(notdir $$^))
else
    ifeq ($(prorab_os),macosx)
        this_test_cmd := (cd $(prorab_this_dir); DYLD_LIBRARY_PATH=../../src ./$$(notdir $$^))
    else
        this_test_cmd := (cd $(prorab_this_dir); LD_LIBRARY_PATH=../../src ./$$(notdir $$^))
    endif
endif

define this_rule
test:: $(prorab_this_name)
	@prorab-running-test.sh $(this_test)
	@$(this_test_cmd) && prorab-passed.sh || prorab-error.sh "test failed"
endef
$(eval $(this_rule))






ifeq ($(prorab_os),windows)
    this_gdb_cmd := (cd $(prorab_this_dir); cp ../../src/libnitki.dll . || true; gdb ./$$(notdir $$^))
else
    ifeq ($(prorab_os),macosx)
        this_gdb_cmd := (cd $(prorab_this_dir); DYLD_LIBRARY_PATH=../../src gdb ./$$(notdir $$^))
    else
        this_gdb_cmd := (cd $(prorab_this_dir); LD_LIBRARY_PATH=../../src gdb ./$$(notdir $$^))
    endif
endif


define this_rule
gdb:: $(prorab_this_name)
	@echo running $$^...
	@$(this_gdb_cmd)
endef
$(eval $(this_rule))

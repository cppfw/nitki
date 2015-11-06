# Prorab build system
# Copyright 2015 Ivan Gagis <igagis@gmail.com>


#once
ifneq ($(prorab_included),true)
    prorab_included := true

    #for storing list of included makefiles
    prorab_included_makefiles :=


    #check if running minimal supported GNU make version
    prorab_min_gnumake_version := 3.81
    ifeq ($(filter $(prorab_min_gnumake_version),$(firstword $(sort $(MAKE_VERSION) $(prorab_min_gnumake_version)))),)
        $(error GNU make $(prorab_min_gnumake_version) or higher is needed, but found only $(MAKE_VERSION))
    endif


    #check that prorab.mk is the first file included
    ifneq ($(words $(MAKEFILE_LIST)),2)
        $(error prorab.mk is not a first include in the makefile, include prorab.mk should be the very first thing done in the makefile.)
    endif


    #define arithmetic functions
    prorab-num = $(words $1)
    prorab-add = $1 $2
    prarab-inc = x $1
    prorab-dec = $(wordlist 2,$(words $1),$1)


    #define this directory for parent makefile
    prorab_this_makefile := $(word $(call prorab-num,$(call prorab-dec,$(MAKEFILE_LIST))),$(MAKEFILE_LIST))
    prorab_this_dir := $(dir $(prorab_this_makefile))

    #define local variables used by prorab
    this_name :=
    this_soname :=
    this_cflags :=
    this_ldflags :=
    this_ldlibs :=
    this_srcs :=

    #variables used with prorab-apply-version rule
    this_version_files :=



    .PHONY: clean all install distclean deb test


    #define the very first default target
    all:

    #define distclean target which does same as clean. This is to make some older versions of debhelper happy.
    distclean: clean

    test::

    #directory of prorab.mk
    prorab_dir := $(dir $(lastword $(MAKEFILE_LIST)))

    #initialize standard vars for "install" target
    ifeq ($(PREFIX),) #PREFIX is environment variable, but if it is not set, then set default value
        PREFIX := /usr/local
    endif

    #Detect operating system
    prorab_private_os := $(shell uname)
    ifeq ($(patsubst MINGW%,MINGW,$(prorab_private_os)), MINGW)
        prorab_os := windows
    else ifeq ($(prorab_private_os), Darwin)
        prorab_os := macosx
    else
        prorab_os := linux
    endif

    #set library extension
    ifeq ($(prorab_os), windows)
        prorab_lib_extension := .dll
    else ifeq ($(prorab_os), macosx)
        prorab_lib_extension := .dylib
    else
        prorab_lib_extension := .so
    endif



    prorab_obj_dir := obj/


    prorab_echo := @


    define prorab-private-app-specific-rules
        $(eval prorab_private_ldflags := )

        $(if $(filter windows,$(prorab_os)), \
                $(eval prorab_this_name := $(abspath $(prorab_this_dir)$(this_name).exe)) \
            , \
                $(eval prorab_this_name := $(abspath $(prorab_this_dir)$(this_name))) \
            )

        $(eval prorab_this_symbolic_name := $(prorab_this_name))

        install:: $(prorab_this_name)
		$(prorab_echo)install -d $(DESTDIR)$(PREFIX)/bin/
		$(prorab_echo)install $(prorab_this_name) $(DESTDIR)$(PREFIX)/bin/
    endef



    define prorab-private-lib-specific-rules-nix-systems
        $(if $(filter macosx,$(prorab_os)), \
                $(eval prorab_this_symbolic_name := $(abspath $(prorab_this_dir)lib$(this_name)$(prorab_lib_extension))) \
                $(eval prorab_this_name := $(abspath $(prorab_this_dir)lib$(this_name).$(this_soname)$(prorab_lib_extension))) \
                $(eval prorab_private_ldflags += -dynamiclib -Wl,-install_name,$(prorab_this_name),-headerpad_max_install_names,-undefined,dynamic_lookup,-compatibility_version,1.0,-current_version,1.0) \
            ,\
                $(eval prorab_this_symbolic_name := $(abspath $(prorab_this_dir)lib$(this_name)$(prorab_lib_extension))) \
                $(eval prorab_this_name := $(prorab_this_symbolic_name).$(this_soname)) \
                $(eval prorab_private_ldflags := -shared -Wl,-soname,$(notdir $(prorab_this_name))) \
            )

        #symbolic link to shared library rule
        $(prorab_this_symbolic_name): $(prorab_this_name) $(prorab_this_makefile)
			@echo "Creating symbolic link $$(notdir $$@) -> $$(notdir $$<)..."
			$(prorab_echo)(cd $$(dir $$<); ln -f -s $$(notdir $$<) $$(notdir $$@))

        all: $(prorab_this_symbolic_name)

        install:: $(prorab_this_symbolic_name)
		@install -d $(DESTDIR)$(PREFIX)/lib/
		$(prorab_echo)(cd $(DESTDIR)$(PREFIX)/lib/; ln -f -s $(notdir $(prorab_this_name)) $(notdir $(prorab_this_symbolic_name)))

        clean::
		$(prorab_echo)rm -f $(prorab_this_symbolic_name)
    endef

    #foreach is used to filter out all files which are not inside of a directory
    define prorab-private-lib-install-headers-rule
        install::
		$(prorab_echo)for i in $(foreach v,$(patsubst $(prorab_this_dir)%,%,$(shell find $(prorab_this_dir) -type f -name "*.hpp")),$(if $(findstring /,$(v)),$(v),)); do \
		    install -d $(DESTDIR)$(PREFIX)/include/$$$${i%/*}; \
		    install $(prorab_this_dir)$$$$i $(DESTDIR)$(PREFIX)/include/$$$$i; \
		done
    endef

    define prorab-private-lib-specific-rules
        $(if $(filter windows,$(prorab_os)), \
                $(eval prorab_this_name := $(abspath $(prorab_this_dir)lib$(this_name)$(prorab_lib_extension))) \
                $(eval prorab_private_ldflags := -shared -s) \
                $(eval prorab_this_symbolic_name := $(prorab_this_name)) \
            , \
                $(prorab-private-lib-specific-rules-nix-systems) \
            )

        $(eval prorab_this_staticlib := $(abspath $(prorab_this_dir)lib$(this_name).a))


        all: $(prorab_this_staticlib)


        #static library rule
        $(prorab_this_staticlib): $(addprefix $(prorab_this_dir)$(prorab_obj_dir),$(patsubst %.cpp,%.o,$(this_srcs))) $(prorab_this_makefile)
			@echo "Creating static library $$(notdir $$@)..."
			$(prorab_echo)ar cr $$@ $$(filter %.o,$$^)


        clean::
		$(prorab_echo)rm -f $(prorab_this_staticlib)

        $(prorab-private-lib-install-headers-rule)

        install:: $(prorab_this_staticlib) $(prorab_this_name)
		$(prorab_echo)install -d $(DESTDIR)$(PREFIX)/lib/
		$(prorab_echo)install $(prorab_this_staticlib) $(DESTDIR)$(PREFIX)/lib/
		$(prorab_echo)install $(prorab_this_name) $(DESTDIR)$(PREFIX)/lib/
		$(if $(filter macosx,$(prorab_os)), \
		        $(prorab_echo)install_name_tool -id "$(PREFIX)/lib/$(notdir $(prorab_this_name))" $(DESTDIR)$(PREFIX)/lib/$(notdir $(prorab_this_name)) \
		    )
    endef


    define prorab-private-common-rules

        all: $(prorab_this_name)

        $(eval prorab_this_objs := $(addprefix $(prorab_this_dir)$(prorab_obj_dir),$(patsubst %.cpp,%.o,$(this_srcs))))

        #compile static pattern rule
        $(prorab_this_objs): $(prorab_this_dir)$(prorab_obj_dir)%.o: $(prorab_this_dir)%.cpp $(prorab_this_makefile)
		@echo "Compiling $$<..."
		$(prorab_echo)mkdir -p $$(dir $$@)
		$(prorab_echo)$$(CXX) -c -MF "$$(patsubst %.o,%.d,$$@)" -MD -o "$$@" $(CXXFLAGS) $(CPPFLAGS) $(this_cflags) $$<

        #include rules for header dependencies
        include $(wildcard $(addsuffix *.d,$(dir $(addprefix $(prorab_this_dir)$(prorab_obj_dir),$(this_srcs)))))

        #link rule
        $(prorab_this_name): $(prorab_this_objs) $(prorab_this_makefile)
		@echo "Linking $$@..."
		$(prorab_echo)$$(CXX) $$(filter %.o,$$^) -o "$$@" $(this_ldlibs) $(this_ldflags) $(LDLIBS) $(LDFLAGS) $(prorab_private_ldflags)

        #clean rule
        clean::
		$(prorab_echo)rm -rf $(prorab_this_dir)$(prorab_obj_dir)
		$(prorab_echo)rm -f $(prorab_this_name)
    endef

    #if there are no any sources in this_srcs then just install headers, no need to build binaries
    define prorab-build-lib
        $(if $(this_srcs), \
                $(prorab-private-lib-specific-rules) \
                $(prorab-private-common-rules) \
                , \
                $(prorab-private-lib-install-headers-rule) \
            )
    endef


    define prorab-build-app
        $(prorab-private-app-specific-rules)
        $(prorab-private-common-rules)
    endef




    define prorab-include
        $(if $(filter $(abspath $1),$(prorab_included_makefiles)), \
            , \
                $(eval prorab_included_makefiles += $(abspath $1)) \
                $(call prorab-private-include,$1) \
            )
    endef


    #for storing previous prorab_this_makefile when including other makefiles
    prorab_private_this_makefiles :=

    #include file with correct prorab_this_dir
    define prorab-private-include
        prorab_private_this_makefiles += $$(prorab_this_makefile)
        prorab_this_makefile := $1
        prorab_this_dir := $$(dir $$(prorab_this_makefile))
        include $1
        prorab_this_makefile := $$(lastword $$(prorab_private_this_makefiles))
        prorab_this_dir := $$(dir $$(prorab_this_makefile))
        prorab_private_this_makefiles := $$(wordlist 1,$$(call prorab-num,$$(call prorab-dec,$$(prorab_private_this_makefiles))),$$(prorab_private_this_makefiles))

    endef
    #!!!NOTE: the trailing empty line in 'prorab-private-include' definition is needed so that include files would be separated from each other

    #include all makefiles in subdirectories
    define prorab-build-subdirs
        $(foreach path,$(wildcard $(prorab_this_dir)*/makefile),$(call prorab-include,$(path)))
    endef


    prorab-clear-this-vars = $(foreach var,$(filter this_%,$(.VARIABLES)),$(eval $(var) := ))



    #doxygen docs are only possible for libraries, so install path is lib*-doc
    define prorab-build-doxygen
        all: doc

        doc:: $(prorab_this_dir)doxygen

        $(prorab_this_dir)doxygen.cfg: $(prorab_this_dir)doxygen.cfg.in $(prorab_this_dir)../debian/changelog
		@echo "path = $PATH"
		$(prorab_echo)prorab-apply-version.sh $(shell prorab-deb-version.sh $(prorab_this_dir)../debian/changelog) $$(firstword $$^)

        $(prorab_this_dir)doxygen: $(prorab_this_dir)doxygen.cfg
		@echo "Building docs..."
		$(prorab_echo)(cd $(prorab_this_dir); doxygen doxygen.cfg || true)

        clean::
		$(prorab_echo)rm -rf $(prorab_this_dir)doxygen
		$(prorab_echo)rm -rf $(prorab_this_dir)doxygen.cfg

        install:: $(prorab_this_dir)doxygen
		$(prorab_echo)install -d $(DESTDIR)$(PREFIX)/share/doc/lib$(this_name)-doc
		$(prorab_echo)install $(prorab_this_dir)doxygen/* $(DESTDIR)$(PREFIX)/share/doc/lib$(this_name)-doc || true #ignore error, not all systems have doxygen

    endef



    define prorab-build-deb
        $(if $(filter true, $(prorab_private_deb_target_set)), $(error more than one 'prorab-build-deb' detected, only one is allowed in a makefile))

        $(eval prorab_private_deb_target_set := true)

        deb: $(prorab_this_dir)debian/control $(patsubst %.install.in, %$(this_soname).install, $(shell ls $(prorab_this_dir)debian/*.install.in 2>/dev/null))
		$(prorab_echo)(cd $(prorab_this_dir); dpkg-buildpackage)

        $(prorab_this_dir)debian/control: $(prorab_this_dir)debian/control.in $(prorab_this_makefile) $(this_soname_dependency)
		@echo "Generating $$@..."
		$(prorab_echo)sed -e "s/\$$$$(soname)/$(this_soname)/g" $$(filter %debian/control.in, $$^) > $$@

        %$(this_soname).install: %.install.in $(prorab_this_makefile) $(this_soname_dependency)
		@echo "Generating $$@..."
		$(prorab_echo)cp $$< $$@
    endef
endif #~once


$(if $(filter $(prorab_this_makefile),$(prorab_included_makefiles)), \
        \
    , \
        $(eval prorab_included_makefiles += $(abspath $(prorab_this_makefile))) \
    )

$(prorab-clear-this-vars)

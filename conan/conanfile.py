import os
from conan import ConanFile
from conan.tools.scm import Git
from conan.tools.files import load, update_conandata, copy
from conan.tools.layout import basic_layout

class NitkiConan(ConanFile):
	name = "nitki"
	# NOTE: version is supposed to be specified as `--version` argument
	#  to 'conan create' command from command line
	# version = ""
	license = "MIT"
	author = "Ivan Gagis <igagis@gmail.com>"
	url = "http://github.com/cppfw/" + name
	description = "C++ cross-platofrm multithreading library"
	topics = ("C++", "cross-platform")
	settings = "os", "compiler", "build_type", "arch"
	package_type = "library"
	options = {"shared": [True, False], "fPIC": [True, False]}
	default_options = {"shared": False, "fPIC": True}
	generators = "AutotoolsDeps" # this will set CXXFLAGS etc. env vars

	def requirements(self):
		self.requires("utki/[>=1.1.192]@cppfw/main", transitive_headers=True, transitive_libs=True)
		self.requires("opros/[>=0.0.1]@cppfw/main", transitive_headers=True, transitive_libs=True)

	def build_requirements(self):
		self.tool_requires("prorab/[>=2.0.27]@cppfw/main")
		self.tool_requires("prorab-extra/[>=0.2.57]@cppfw/main")

	def config_options(self):
		if self.settings.os == "Windows":
			del self.options.fPIC

	# save commit and remote URL to conandata.yml for packaging
	def export(self):
		git = Git(self)
		scm_url = git.get_remote_url()
		# NOTE: Git.get_commit() doesn't work properly,
		# it gets latest commit of the folder in which conanfile.py resides.
		# So, we use "git rev-parse HEAD" instead as it gets the actual HEAD
		# commit regardless of the current working directory within the repo.
		scm_commit = git.run("rev-parse HEAD") # get current commit
		update_conandata(self, {"sources": {"commit": scm_commit, "url": scm_url}})

	def source(self):
		git = Git(self)
		sources = self.conan_data["sources"]
		# shallow fetch commit
		git.fetch_commit(url=sources["url"], commit=sources['commit'])
		# shallow clone submodules
		git.run("submodule update --init --remote --depth 1")

	def build(self):
		self.run("make $MAKE_INCLUDE_DIRS_ARG lint=off")
		self.run("make $MAKE_INCLUDE_DIRS_ARG lint=off test")

	def package(self):
		src_dir = os.path.join(self.build_folder, "src")
		src_rel_dir = os.path.join(self.build_folder, "src/out/rel")
		dst_include_dir = os.path.join(self.package_folder, "include")
		dst_lib_dir = os.path.join(self.package_folder, "lib")
		dst_bin_dir = os.path.join(self.package_folder, "bin")
		
		copy(conanfile=self, pattern="*.h",                    dst=dst_include_dir, src=src_dir,     keep_path=True)
		copy(conanfile=self, pattern="*.hpp",                  dst=dst_include_dir, src=src_dir,     keep_path=True)

		if self.options.shared:
			copy(conanfile=self, pattern="*" + self.name + ".lib", dst=dst_lib_dir,     src="",          keep_path=False)
			copy(conanfile=self, pattern="*.dll",                  dst=dst_bin_dir,     src=src_rel_dir, keep_path=False)
			copy(conanfile=self, pattern="*.so",                   dst=dst_lib_dir,     src=src_rel_dir, keep_path=False)
			copy(conanfile=self, pattern="*.so.*",                 dst=dst_lib_dir,     src=src_rel_dir, keep_path=False)
			copy(conanfile=self, pattern="*.dylib",                dst=dst_lib_dir,     src=src_rel_dir, keep_path=False)
		else:
			copy(conanfile=self, pattern="*" + self.name + ".lib", dst=dst_lib_dir,     src="",          keep_path=False)
			copy(conanfile=self, pattern="*.a",                    dst=dst_lib_dir,     src=src_rel_dir, keep_path=False)

	def package_info(self):
		self.cpp_info.libs = [self.name]

		if self.settings.os == "Linux":
			# in case of static linking we need -pthread flag on Linux when using threads
			if not self.options.shared:
				ldflags = '-pthread'
				self.cpp_info.sharedlinkflags.extend([ldflags])
				self.cpp_info.exelinkflags.extend([ldflags])

	def package_id(self):
		# change package id only when minor or major version changes, i.e. when ABI breaks
		self.info.requires.minor_mode()

# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.25

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/bin/cmake

# The command to remove a file.
RM = /usr/local/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/offloadingcontainer

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/offloadingcontainer/cmake/build

# Include any dependencies generated for this target.
include CMakeFiles/offloading-container.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/offloading-container.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/offloading-container.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/offloading-container.dir/flags.make

CMakeFiles/offloading-container.dir/main.cc.o: CMakeFiles/offloading-container.dir/flags.make
CMakeFiles/offloading-container.dir/main.cc.o: /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/offloadingcontainer/main.cc
CMakeFiles/offloading-container.dir/main.cc.o: CMakeFiles/offloading-container.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/root/workspace/Storage-Engine-Instance/storage-engine-instance-container/offloadingcontainer/cmake/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/offloading-container.dir/main.cc.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/offloading-container.dir/main.cc.o -MF CMakeFiles/offloading-container.dir/main.cc.o.d -o CMakeFiles/offloading-container.dir/main.cc.o -c /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/offloadingcontainer/main.cc

CMakeFiles/offloading-container.dir/main.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/offloading-container.dir/main.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/offloadingcontainer/main.cc > CMakeFiles/offloading-container.dir/main.cc.i

CMakeFiles/offloading-container.dir/main.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/offloading-container.dir/main.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/offloadingcontainer/main.cc -o CMakeFiles/offloading-container.dir/main.cc.s

CMakeFiles/offloading-container.dir/SnippetScheduler.cc.o: CMakeFiles/offloading-container.dir/flags.make
CMakeFiles/offloading-container.dir/SnippetScheduler.cc.o: /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/offloadingcontainer/SnippetScheduler.cc
CMakeFiles/offloading-container.dir/SnippetScheduler.cc.o: CMakeFiles/offloading-container.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/root/workspace/Storage-Engine-Instance/storage-engine-instance-container/offloadingcontainer/cmake/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/offloading-container.dir/SnippetScheduler.cc.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/offloading-container.dir/SnippetScheduler.cc.o -MF CMakeFiles/offloading-container.dir/SnippetScheduler.cc.o.d -o CMakeFiles/offloading-container.dir/SnippetScheduler.cc.o -c /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/offloadingcontainer/SnippetScheduler.cc

CMakeFiles/offloading-container.dir/SnippetScheduler.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/offloading-container.dir/SnippetScheduler.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/offloadingcontainer/SnippetScheduler.cc > CMakeFiles/offloading-container.dir/SnippetScheduler.cc.i

CMakeFiles/offloading-container.dir/SnippetScheduler.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/offloading-container.dir/SnippetScheduler.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/offloadingcontainer/SnippetScheduler.cc -o CMakeFiles/offloading-container.dir/SnippetScheduler.cc.s

# Object files for target offloading-container
offloading__container_OBJECTS = \
"CMakeFiles/offloading-container.dir/main.cc.o" \
"CMakeFiles/offloading-container.dir/SnippetScheduler.cc.o"

# External object files for target offloading-container
offloading__container_EXTERNAL_OBJECTS =

offloading-container: CMakeFiles/offloading-container.dir/main.cc.o
offloading-container: CMakeFiles/offloading-container.dir/SnippetScheduler.cc.o
offloading-container: CMakeFiles/offloading-container.dir/build.make
offloading-container: libss_grpc_proto.a
offloading-container: /lib/libgrpc++_reflection.a
offloading-container: /lib/libgrpc++.a
offloading-container: /lib/libprotobuf.a
offloading-container: /lib/libgrpc.a
offloading-container: /lib/libz.a
offloading-container: /lib/libcares.a
offloading-container: /lib/libaddress_sorting.a
offloading-container: /lib/libre2.a
offloading-container: /lib/libabsl_raw_hash_set.a
offloading-container: /lib/libabsl_hashtablez_sampler.a
offloading-container: /lib/libabsl_hash.a
offloading-container: /lib/libabsl_city.a
offloading-container: /lib/libabsl_low_level_hash.a
offloading-container: /lib/libabsl_statusor.a
offloading-container: /lib/libabsl_bad_variant_access.a
offloading-container: /lib/libgpr.a
offloading-container: /lib/libupb.a
offloading-container: /lib/libabsl_status.a
offloading-container: /lib/libabsl_random_distributions.a
offloading-container: /lib/libabsl_random_seed_sequences.a
offloading-container: /lib/libabsl_random_internal_pool_urbg.a
offloading-container: /lib/libabsl_random_internal_randen.a
offloading-container: /lib/libabsl_random_internal_randen_hwaes.a
offloading-container: /lib/libabsl_random_internal_randen_hwaes_impl.a
offloading-container: /lib/libabsl_random_internal_randen_slow.a
offloading-container: /lib/libabsl_random_internal_platform.a
offloading-container: /lib/libabsl_random_internal_seed_material.a
offloading-container: /lib/libabsl_random_seed_gen_exception.a
offloading-container: /lib/libabsl_cord.a
offloading-container: /lib/libabsl_bad_optional_access.a
offloading-container: /lib/libabsl_cordz_info.a
offloading-container: /lib/libabsl_cord_internal.a
offloading-container: /lib/libabsl_cordz_functions.a
offloading-container: /lib/libabsl_exponential_biased.a
offloading-container: /lib/libabsl_cordz_handle.a
offloading-container: /lib/libabsl_str_format_internal.a
offloading-container: /lib/libabsl_synchronization.a
offloading-container: /lib/libabsl_stacktrace.a
offloading-container: /lib/libabsl_symbolize.a
offloading-container: /lib/libabsl_debugging_internal.a
offloading-container: /lib/libabsl_demangle_internal.a
offloading-container: /lib/libabsl_graphcycles_internal.a
offloading-container: /lib/libabsl_malloc_internal.a
offloading-container: /lib/libabsl_time.a
offloading-container: /lib/libabsl_strings.a
offloading-container: /lib/libabsl_throw_delegate.a
offloading-container: /lib/libabsl_int128.a
offloading-container: /lib/libabsl_strings_internal.a
offloading-container: /lib/libabsl_base.a
offloading-container: /lib/libabsl_spinlock_wait.a
offloading-container: /lib/libabsl_raw_logging_internal.a
offloading-container: /lib/libabsl_log_severity.a
offloading-container: /lib/libabsl_civil_time.a
offloading-container: /lib/libabsl_time_zone.a
offloading-container: /lib/libssl.a
offloading-container: /lib/libcrypto.a
offloading-container: CMakeFiles/offloading-container.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/root/workspace/Storage-Engine-Instance/storage-engine-instance-container/offloadingcontainer/cmake/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX executable offloading-container"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/offloading-container.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/offloading-container.dir/build: offloading-container
.PHONY : CMakeFiles/offloading-container.dir/build

CMakeFiles/offloading-container.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/offloading-container.dir/cmake_clean.cmake
.PHONY : CMakeFiles/offloading-container.dir/clean

CMakeFiles/offloading-container.dir/depend:
	cd /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/offloadingcontainer/cmake/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/offloadingcontainer /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/offloadingcontainer /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/offloadingcontainer/cmake/build /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/offloadingcontainer/cmake/build /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/offloadingcontainer/cmake/build/CMakeFiles/offloading-container.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/offloading-container.dir/depend


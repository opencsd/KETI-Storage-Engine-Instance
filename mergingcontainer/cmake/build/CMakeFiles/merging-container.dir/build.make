# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.24

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/mergingcontainer

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/mergingcontainer/cmake/build

# Include any dependencies generated for this target.
include CMakeFiles/merging-container.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/merging-container.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/merging-container.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/merging-container.dir/flags.make

CMakeFiles/merging-container.dir/main.cc.o: CMakeFiles/merging-container.dir/flags.make
CMakeFiles/merging-container.dir/main.cc.o: /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/mergingcontainer/main.cc
CMakeFiles/merging-container.dir/main.cc.o: CMakeFiles/merging-container.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/root/workspace/Storage-Engine-Instance/storage-engine-instance-container/mergingcontainer/cmake/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/merging-container.dir/main.cc.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/merging-container.dir/main.cc.o -MF CMakeFiles/merging-container.dir/main.cc.o.d -o CMakeFiles/merging-container.dir/main.cc.o -c /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/mergingcontainer/main.cc

CMakeFiles/merging-container.dir/main.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/merging-container.dir/main.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/mergingcontainer/main.cc > CMakeFiles/merging-container.dir/main.cc.i

CMakeFiles/merging-container.dir/main.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/merging-container.dir/main.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/mergingcontainer/main.cc -o CMakeFiles/merging-container.dir/main.cc.s

CMakeFiles/merging-container.dir/BufferManager.cc.o: CMakeFiles/merging-container.dir/flags.make
CMakeFiles/merging-container.dir/BufferManager.cc.o: /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/mergingcontainer/BufferManager.cc
CMakeFiles/merging-container.dir/BufferManager.cc.o: CMakeFiles/merging-container.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/root/workspace/Storage-Engine-Instance/storage-engine-instance-container/mergingcontainer/cmake/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/merging-container.dir/BufferManager.cc.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/merging-container.dir/BufferManager.cc.o -MF CMakeFiles/merging-container.dir/BufferManager.cc.o.d -o CMakeFiles/merging-container.dir/BufferManager.cc.o -c /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/mergingcontainer/BufferManager.cc

CMakeFiles/merging-container.dir/BufferManager.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/merging-container.dir/BufferManager.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/mergingcontainer/BufferManager.cc > CMakeFiles/merging-container.dir/BufferManager.cc.i

CMakeFiles/merging-container.dir/BufferManager.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/merging-container.dir/BufferManager.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/mergingcontainer/BufferManager.cc -o CMakeFiles/merging-container.dir/BufferManager.cc.s

CMakeFiles/merging-container.dir/MergeQueryManager.cc.o: CMakeFiles/merging-container.dir/flags.make
CMakeFiles/merging-container.dir/MergeQueryManager.cc.o: /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/mergingcontainer/MergeQueryManager.cc
CMakeFiles/merging-container.dir/MergeQueryManager.cc.o: CMakeFiles/merging-container.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/root/workspace/Storage-Engine-Instance/storage-engine-instance-container/mergingcontainer/cmake/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/merging-container.dir/MergeQueryManager.cc.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/merging-container.dir/MergeQueryManager.cc.o -MF CMakeFiles/merging-container.dir/MergeQueryManager.cc.o.d -o CMakeFiles/merging-container.dir/MergeQueryManager.cc.o -c /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/mergingcontainer/MergeQueryManager.cc

CMakeFiles/merging-container.dir/MergeQueryManager.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/merging-container.dir/MergeQueryManager.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/mergingcontainer/MergeQueryManager.cc > CMakeFiles/merging-container.dir/MergeQueryManager.cc.i

CMakeFiles/merging-container.dir/MergeQueryManager.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/merging-container.dir/MergeQueryManager.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/mergingcontainer/MergeQueryManager.cc -o CMakeFiles/merging-container.dir/MergeQueryManager.cc.s

# Object files for target merging-container
merging__container_OBJECTS = \
"CMakeFiles/merging-container.dir/main.cc.o" \
"CMakeFiles/merging-container.dir/BufferManager.cc.o" \
"CMakeFiles/merging-container.dir/MergeQueryManager.cc.o"

# External object files for target merging-container
merging__container_EXTERNAL_OBJECTS =

merging-container: CMakeFiles/merging-container.dir/main.cc.o
merging-container: CMakeFiles/merging-container.dir/BufferManager.cc.o
merging-container: CMakeFiles/merging-container.dir/MergeQueryManager.cc.o
merging-container: CMakeFiles/merging-container.dir/build.make
merging-container: libss_grpc_proto.a
merging-container: /lib/libgrpc++_reflection.a
merging-container: /lib/libgrpc++.a
merging-container: /lib/libprotobuf.a
merging-container: /lib/libgrpc.a
merging-container: /lib/libz.a
merging-container: /lib/libcares.a
merging-container: /lib/libaddress_sorting.a
merging-container: /lib/libre2.a
merging-container: /lib/libabsl_raw_hash_set.a
merging-container: /lib/libabsl_hashtablez_sampler.a
merging-container: /lib/libabsl_hash.a
merging-container: /lib/libabsl_city.a
merging-container: /lib/libabsl_low_level_hash.a
merging-container: /lib/libabsl_statusor.a
merging-container: /lib/libabsl_bad_variant_access.a
merging-container: /lib/libgpr.a
merging-container: /lib/libupb.a
merging-container: /lib/libabsl_status.a
merging-container: /lib/libabsl_random_distributions.a
merging-container: /lib/libabsl_random_seed_sequences.a
merging-container: /lib/libabsl_random_internal_pool_urbg.a
merging-container: /lib/libabsl_random_internal_randen.a
merging-container: /lib/libabsl_random_internal_randen_hwaes.a
merging-container: /lib/libabsl_random_internal_randen_hwaes_impl.a
merging-container: /lib/libabsl_random_internal_randen_slow.a
merging-container: /lib/libabsl_random_internal_platform.a
merging-container: /lib/libabsl_random_internal_seed_material.a
merging-container: /lib/libabsl_random_seed_gen_exception.a
merging-container: /lib/libabsl_cord.a
merging-container: /lib/libabsl_bad_optional_access.a
merging-container: /lib/libabsl_cordz_info.a
merging-container: /lib/libabsl_cord_internal.a
merging-container: /lib/libabsl_cordz_functions.a
merging-container: /lib/libabsl_exponential_biased.a
merging-container: /lib/libabsl_cordz_handle.a
merging-container: /lib/libabsl_str_format_internal.a
merging-container: /lib/libabsl_synchronization.a
merging-container: /lib/libabsl_stacktrace.a
merging-container: /lib/libabsl_symbolize.a
merging-container: /lib/libabsl_debugging_internal.a
merging-container: /lib/libabsl_demangle_internal.a
merging-container: /lib/libabsl_graphcycles_internal.a
merging-container: /lib/libabsl_malloc_internal.a
merging-container: /lib/libabsl_time.a
merging-container: /lib/libabsl_strings.a
merging-container: /lib/libabsl_throw_delegate.a
merging-container: /lib/libabsl_int128.a
merging-container: /lib/libabsl_strings_internal.a
merging-container: /lib/libabsl_base.a
merging-container: /lib/libabsl_spinlock_wait.a
merging-container: /lib/libabsl_raw_logging_internal.a
merging-container: /lib/libabsl_log_severity.a
merging-container: /lib/libabsl_civil_time.a
merging-container: /lib/libabsl_time_zone.a
merging-container: /lib/libssl.a
merging-container: /lib/libcrypto.a
merging-container: CMakeFiles/merging-container.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/root/workspace/Storage-Engine-Instance/storage-engine-instance-container/mergingcontainer/cmake/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Linking CXX executable merging-container"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/merging-container.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/merging-container.dir/build: merging-container
.PHONY : CMakeFiles/merging-container.dir/build

CMakeFiles/merging-container.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/merging-container.dir/cmake_clean.cmake
.PHONY : CMakeFiles/merging-container.dir/clean

CMakeFiles/merging-container.dir/depend:
	cd /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/mergingcontainer/cmake/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/mergingcontainer /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/mergingcontainer /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/mergingcontainer/cmake/build /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/mergingcontainer/cmake/build /root/workspace/Storage-Engine-Instance/storage-engine-instance-container/mergingcontainer/cmake/build/CMakeFiles/merging-container.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/merging-container.dir/depend


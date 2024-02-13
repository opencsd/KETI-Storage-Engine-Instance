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
CMAKE_SOURCE_DIR = /root/workspace/keti/Storage-Engine-Instance/storage-engine-instance/monitoring_module

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /root/workspace/keti/Storage-Engine-Instance/storage-engine-instance/monitoring_module/cmake/build

# Include any dependencies generated for this target.
include CMakeFiles/monitoring-container.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/monitoring-container.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/monitoring-container.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/monitoring-container.dir/flags.make

CMakeFiles/monitoring-container.dir/monitoring_interface.cc.o: CMakeFiles/monitoring-container.dir/flags.make
CMakeFiles/monitoring-container.dir/monitoring_interface.cc.o: /root/workspace/keti/Storage-Engine-Instance/storage-engine-instance/monitoring_module/monitoring_interface.cc
CMakeFiles/monitoring-container.dir/monitoring_interface.cc.o: CMakeFiles/monitoring-container.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/root/workspace/keti/Storage-Engine-Instance/storage-engine-instance/monitoring_module/cmake/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/monitoring-container.dir/monitoring_interface.cc.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/monitoring-container.dir/monitoring_interface.cc.o -MF CMakeFiles/monitoring-container.dir/monitoring_interface.cc.o.d -o CMakeFiles/monitoring-container.dir/monitoring_interface.cc.o -c /root/workspace/keti/Storage-Engine-Instance/storage-engine-instance/monitoring_module/monitoring_interface.cc

CMakeFiles/monitoring-container.dir/monitoring_interface.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/monitoring-container.dir/monitoring_interface.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /root/workspace/keti/Storage-Engine-Instance/storage-engine-instance/monitoring_module/monitoring_interface.cc > CMakeFiles/monitoring-container.dir/monitoring_interface.cc.i

CMakeFiles/monitoring-container.dir/monitoring_interface.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/monitoring-container.dir/monitoring_interface.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /root/workspace/keti/Storage-Engine-Instance/storage-engine-instance/monitoring_module/monitoring_interface.cc -o CMakeFiles/monitoring-container.dir/monitoring_interface.cc.s

# Object files for target monitoring-container
monitoring__container_OBJECTS = \
"CMakeFiles/monitoring-container.dir/monitoring_interface.cc.o"

# External object files for target monitoring-container
monitoring__container_EXTERNAL_OBJECTS =

monitoring-container: CMakeFiles/monitoring-container.dir/monitoring_interface.cc.o
monitoring-container: CMakeFiles/monitoring-container.dir/build.make
monitoring-container: libss_grpc_proto.a
monitoring-container: /lib/libgrpc++_reflection.a
monitoring-container: /lib/libgrpc++.a
monitoring-container: /lib/libprotobuf.a
monitoring-container: /lib/libgrpc.a
monitoring-container: /lib/libz.a
monitoring-container: /lib/libcares.a
monitoring-container: /lib/libaddress_sorting.a
monitoring-container: /lib/libre2.a
monitoring-container: /lib/libabsl_raw_hash_set.a
monitoring-container: /lib/libabsl_hashtablez_sampler.a
monitoring-container: /lib/libabsl_hash.a
monitoring-container: /lib/libabsl_city.a
monitoring-container: /lib/libabsl_low_level_hash.a
monitoring-container: /lib/libabsl_statusor.a
monitoring-container: /lib/libabsl_bad_variant_access.a
monitoring-container: /lib/libgpr.a
monitoring-container: /lib/libupb.a
monitoring-container: /lib/libabsl_status.a
monitoring-container: /lib/libabsl_random_distributions.a
monitoring-container: /lib/libabsl_random_seed_sequences.a
monitoring-container: /lib/libabsl_random_internal_pool_urbg.a
monitoring-container: /lib/libabsl_random_internal_randen.a
monitoring-container: /lib/libabsl_random_internal_randen_hwaes.a
monitoring-container: /lib/libabsl_random_internal_randen_hwaes_impl.a
monitoring-container: /lib/libabsl_random_internal_randen_slow.a
monitoring-container: /lib/libabsl_random_internal_platform.a
monitoring-container: /lib/libabsl_random_internal_seed_material.a
monitoring-container: /lib/libabsl_random_seed_gen_exception.a
monitoring-container: /lib/libabsl_cord.a
monitoring-container: /lib/libabsl_bad_optional_access.a
monitoring-container: /lib/libabsl_cordz_info.a
monitoring-container: /lib/libabsl_cord_internal.a
monitoring-container: /lib/libabsl_cordz_functions.a
monitoring-container: /lib/libabsl_exponential_biased.a
monitoring-container: /lib/libabsl_cordz_handle.a
monitoring-container: /lib/libabsl_str_format_internal.a
monitoring-container: /lib/libabsl_synchronization.a
monitoring-container: /lib/libabsl_stacktrace.a
monitoring-container: /lib/libabsl_symbolize.a
monitoring-container: /lib/libabsl_debugging_internal.a
monitoring-container: /lib/libabsl_demangle_internal.a
monitoring-container: /lib/libabsl_graphcycles_internal.a
monitoring-container: /lib/libabsl_malloc_internal.a
monitoring-container: /lib/libabsl_time.a
monitoring-container: /lib/libabsl_strings.a
monitoring-container: /lib/libabsl_throw_delegate.a
monitoring-container: /lib/libabsl_int128.a
monitoring-container: /lib/libabsl_strings_internal.a
monitoring-container: /lib/libabsl_base.a
monitoring-container: /lib/libabsl_spinlock_wait.a
monitoring-container: /lib/libabsl_raw_logging_internal.a
monitoring-container: /lib/libabsl_log_severity.a
monitoring-container: /lib/libabsl_civil_time.a
monitoring-container: /lib/libabsl_time_zone.a
monitoring-container: /lib/libssl.a
monitoring-container: /lib/libcrypto.a
monitoring-container: CMakeFiles/monitoring-container.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/root/workspace/keti/Storage-Engine-Instance/storage-engine-instance/monitoring_module/cmake/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable monitoring-container"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/monitoring-container.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/monitoring-container.dir/build: monitoring-container
.PHONY : CMakeFiles/monitoring-container.dir/build

CMakeFiles/monitoring-container.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/monitoring-container.dir/cmake_clean.cmake
.PHONY : CMakeFiles/monitoring-container.dir/clean

CMakeFiles/monitoring-container.dir/depend:
	cd /root/workspace/keti/Storage-Engine-Instance/storage-engine-instance/monitoring_module/cmake/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /root/workspace/keti/Storage-Engine-Instance/storage-engine-instance/monitoring_module /root/workspace/keti/Storage-Engine-Instance/storage-engine-instance/monitoring_module /root/workspace/keti/Storage-Engine-Instance/storage-engine-instance/monitoring_module/cmake/build /root/workspace/keti/Storage-Engine-Instance/storage-engine-instance/monitoring_module/cmake/build /root/workspace/keti/Storage-Engine-Instance/storage-engine-instance/monitoring_module/cmake/build/CMakeFiles/monitoring-container.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/monitoring-container.dir/depend


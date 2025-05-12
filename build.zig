const std = @import("std");
pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    // Helper function to get path the environment variable
    const getPath = struct {
        const GetPathError = error {
            EnvironmentVariableNotFound,
        };

        fn call(bld: *std.Build, env_var: []const u8) GetPathError![]const u8 {
            return std.process.getEnvVarOwned(bld.allocator, env_var) catch |err| switch (err) {
                error.EnvironmentVariableNotFound => return GetPathError.EnvironmentVariableNotFound,
                else => @panic("Failed to access environment variable"),
            };
        }
    }.call;
    // Get include and library paths from options or environment variables
    const boost_include = getPath(b, "BOOST_INCLUDE") catch |err| {
        std.log.err("Failed to get BOOST_INCLUDE: {}", .{err});
        @panic("Build configuration error");
    };
    const reflex_include = getPath(b, "REFLEX_INCLUDE") catch |err| {
        std.log.err("Failed to get REFLEX_INCLUDE: {}", .{err});
        @panic("Build configuration error");
    };
    const reflex_lib = getPath(b, "REFLEX_LIB") catch |err| {
        std.log.err("Failed to get REFLEX_LIB: {}", .{err});
        @panic("Build configuration error");
    };
    const cli11_include = getPath(b, "CLI11_INCLUDE") catch |err| {
        std.log.err("Failed to get CLI11_INCLUDE: {}", .{err});
        @panic("Build configuration error");
    };
    const catch2_include = getPath(b, "CATCH2_INCLUDE") catch |err| {
        std.log.err("Failed to get CATCH2_INCLUDE: {}", .{err});
        @panic("Build configuration error");
    };
    const catch2_lib = getPath(b, "CATCH2_LIB") catch |err| {
        std.log.err("Failed to get CATCH2_LIB: {}", .{err});
        @panic("Build configuration error");
    };
    // Executable: minbpe-cc
    const minbpe_cc = b.addExecutable(.{
        .name = "minbpe-cc",
        .target = target,
        .optimize = optimize,
    });
    minbpe_cc.addCSourceFile(.{
        .file = b.path("code/examples/minbpe-cc.cpp"),
        .flags = &.{"-std=c++23"},
    });
    // Fixed LazyPath usage
    minbpe_cc.addIncludePath(b.path(boost_include));
    minbpe_cc.addIncludePath(b.path(reflex_include));
    minbpe_cc.addIncludePath(b.path(cli11_include));
    minbpe_cc.addIncludePath(b.path("code/include"));
    minbpe_cc.addLibraryPath(b.path(reflex_lib));
    minbpe_cc.linkSystemLibrary("boost_regex");
    minbpe_cc.linkSystemLibrary("reflex");
    minbpe_cc.linkLibCpp();
    b.installArtifact(minbpe_cc);
    // Executable: train
    const train = b.addExecutable(.{
        .name = "train",
        .target = target,
        .optimize = optimize,
    });
    train.addCSourceFile(.{
        .file = b.path("code/examples/train.cpp"),
        .flags = &.{"-std=c++23"},
    });
    train.addIncludePath(b.path(boost_include));
    train.addIncludePath(b.path(reflex_include));
    train.addIncludePath(b.path("code/include"));
    train.addLibraryPath(b.path(reflex_lib));
    train.linkSystemLibrary("boost_regex");
    train.linkSystemLibrary("reflex");
    train.linkLibCpp();
    b.installArtifact(train);
    // Executable: test
    const test_exe = b.addExecutable(.{
        .name = "test",
        .target = target,
        .optimize = optimize,
    });
    test_exe.addCSourceFile(.{
        .file = b.path("code/test/test.cpp"),
        .flags = &.{"-std=c++23"},
    });
    test_exe.addIncludePath(b.path(boost_include));
    test_exe.addIncludePath(b.path(reflex_include));
    test_exe.addIncludePath(b.path(catch2_include));
    test_exe.addIncludePath(b.path("code/include"));
    test_exe.addLibraryPath(b.path(reflex_lib));
    test_exe.addLibraryPath(b.path(catch2_lib));
    test_exe.linkSystemLibrary("boost_regex");
    test_exe.linkSystemLibrary("reflex");
    test_exe.linkSystemLibrary("Catch2Main");
    test_exe.linkLibCpp();
    b.installArtifact(test_exe);
    // Add a step to run the test executable
    const run_test = b.addRunArtifact(test_exe);
    const test_step = b.step("test", "Run the tests");
    test_step.dependOn(&run_test.step);
}

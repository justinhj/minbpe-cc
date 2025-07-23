const std = @import("std");

// Define a specific error set for getPath
const GetPathError = error{
    EnvironmentVariableNotSetAndNoDefault,
    UnderlyingEnvironmentAccessError,
};

// Helper function to get path from the environment variable
fn getPath(
    bld: *std.Build,
    env_var: []const u8,
    isLib: bool,
    default_lib_path: ?[]const u8,
    default_include_path: ?[]const u8,
) GetPathError![]const u8 { // Return our specific error set or []const u8
    return std.process.getEnvVarOwned(bld.allocator, env_var) catch |err| switch (err) {
        error.EnvironmentVariableNotFound => blk: {
            if (isLib) {
                if (default_lib_path) |lib_path| {
                    break :blk lib_path; // Return the default lib_path
                } else {
                    std.log.err("Environment variable '{s}' not set and DEFAULT_LIB_PATH is not set.", .{env_var});
                    return GetPathError.EnvironmentVariableNotSetAndNoDefault; // Return an error
                }
            } else { // isInclude path
                if (default_include_path) |inc_path| {
                    break :blk inc_path; // Return the default inc_path
                } else {
                    std.log.err("Environment variable '{s}' not set and DEFAULT_INCLUDE_PATH is not set.", .{env_var});
                    return GetPathError.EnvironmentVariableNotSetAndNoDefault; // Return an error
                }
            }
        },
        else => {
            // Log the original error from getEnvVarOwned for more context
            std.log.err("Failed to access environment variable '{s}': {any}", .{ env_var, err });
            return GetPathError.UnderlyingEnvironmentAccessError; // Return an error
        },
    };
}

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // See if the user set DEFAULT_LIB_PATH or DEFAULT_INCLUDE_PATH in the environment
    // and retrieve the values or null if they don't using Zig's optional type
    const default_lib_path = std.process.getEnvVarOwned(b.allocator, "DEFAULT_LIB_PATH") catch |err| switch (err) {
        error.EnvironmentVariableNotFound => null,
        else => {
            std.log.err("Failed to access environment variable DEFAULT_LIB_PATH: {any}", .{err});
            // It's generally better to return an error from build() or handle gracefully
            // For this example, we'll keep panic but ideally, this would also be an error.
            @panic("Failed to access DEFAULT_LIB_PATH environment variable");
        },
    };
    const default_include_path = std.process.getEnvVarOwned(b.allocator, "DEFAULT_INCLUDE_PATH") catch |err| switch (err) {
        error.EnvironmentVariableNotFound => null,
        else => {
            std.log.err("Failed to access environment variable DEFAULT_INCLUDE_PATH: {any}", .{err});
            @panic("Failed to access DEFAULT_INCLUDE_PATH environment variable");
        },
    };

    // Get include and library paths from options or environment variables
    // Line 44 was here:
    const boost_include = getPath(b, "BOOST_INCLUDE", false, default_lib_path, default_include_path) catch |err| {
        std.log.err("Failed to get BOOST_INCLUDE: {any}", .{err}); // Use {any} for error types
        @panic("Build configuration error: BOOST_INCLUDE");
    };
    const pcre2_include = getPath(b, "PCRE2_INCLUDE", false, default_lib_path, default_include_path) catch |err| {
        std.log.err("Failed to get PCRE2_INCLUDE: {any}", .{err});
        @panic("Build configuration error: PCRE2_INCLUDE");
    };
    const cli11_include = getPath(b, "CLI11_INCLUDE", false, default_lib_path, default_include_path) catch |err| {
        std.log.err("Failed to get CLI11_INCLUDE: {any}", .{err});
        @panic("Build configuration error: CLI11_INCLUDE");
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
    minbpe_cc.addIncludePath(b.path(boost_include));
    minbpe_cc.addIncludePath(b.path(pcre2_include));
    minbpe_cc.addIncludePath(b.path(cli11_include));
    minbpe_cc.addIncludePath(b.path("code/include"));
    minbpe_cc.linkSystemLibrary("pcre2-8");  // ICU Internationalization Library
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
    train.addIncludePath(b.path(pcre2_include));
    train.addIncludePath(b.path("code/include"));
    train.linkSystemLibrary("pcre2-8");  // ICU Internationalization Library
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
    test_exe.addCSourceFile(.{
        .file = b.path("code/catch2/catch_amalgamated.cpp"),
        .flags = &.{"-std=c++23"},
    });
    test_exe.addIncludePath(b.path(boost_include));
    test_exe.addIncludePath(b.path(pcre2_include));
    test_exe.addIncludePath(b.path("code/include"));
    test_exe.addIncludePath(b.path("code/catch2"));
    test_exe.linkSystemLibrary("pcre2-8");
    test_exe.linkLibCpp();
    b.installArtifact(test_exe);

    // Add a step to run the test executable
    const run_test = b.addRunArtifact(test_exe);
    const test_step = b.step("test", "Run the tests");
    test_step.dependOn(&run_test.step);
}

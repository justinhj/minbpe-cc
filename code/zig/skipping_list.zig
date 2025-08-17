const std = @import("std");

// This struct will be used to return a string (pointer + length) to C/C++.
// It's important that it has a C-compatible layout.
pub const CStringResult = extern struct {
    ptr: [*c]u8,
    len: usize,
};

// This function is exported to be callable from C/C++.
// It takes a C string (null-terminated pointer) and returns a CStringResult.
pub export fn hello_from_zig(name: [*c]const u8) CStringResult {
    // Use the C allocator (malloc/free) so the C++ side can manage memory if needed,
    // though we provide a free function.
    const allocator = std.heap.c_allocator;

    // Convert the C string to a Zig slice.
    const zig_name = std.mem.span(name);

    // Create the "hello, " string.
    const result_slice = std.fmt.allocPrint(allocator, "hello, {s}", .{zig_name}) catch |err| {
        // On error, print to stderr and return a null pointer.
        std.debug.print("Failed to allocate or format string: {any}\n", .{err});
        return CStringResult{ .ptr = null, .len = 0 };
    };

    // Return the result as a pointer and length.
    return CStringResult{ .ptr = result_slice.ptr, .len = result_slice.len };
}

// This function is exported to allow the C/C++ side to free the memory
// allocated by `hello_from_zig`.
pub export fn free_zig_string(result: CStringResult) void {
    const allocator = std.heap.c_allocator;
    allocator.free(result.ptr[0..result.len]);
}

// The original test can be adapted or kept to test the internal logic.
test "hello function C interop" {
    const allocator = std.testing.allocator;
    const name = "C++";
    const c_name = allocator.dupeZ(u8, name) catch @panic("oom");
    defer allocator.free(c_name);

    const result = hello_from_zig(c_name.ptr);
    defer free_zig_string(result);

    const result_slice = result.ptr[0..result.len];
    try std.testing.expectEqualStrings("hello, C++", result_slice);
}
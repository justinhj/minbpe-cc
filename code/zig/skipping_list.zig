const std = @import("std");

pub fn hello(allocator: std.mem.Allocator, name: []const u8) ![]const u8 {
    return std.fmt.allocPrint(allocator, "Hello, {s}", .{name});
}

test "hello function" {
    const allocator = std.testing.allocator;
    const greeting = try hello(allocator, "Justin");
    defer allocator.free(greeting);
    try std.testing.expectEqualStrings("Hello, Justin", greeting);
}

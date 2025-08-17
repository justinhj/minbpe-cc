const std = @import("std");

pub fn SkippingList(comptime T: type, skip_bits: T) type {
    const MAX_SKIP_BITS = 16;

    comptime {
        const T_info = @typeInfo(T);
        if (T_info != .Int or T_info.int.signedness != .Unsigned or T_info.int.bits < 32) {
            @compileError("SkippingList(T) requires T to be a numeric (integer or float) type 32 bits and up.");
        }
        if (skip_bits == 0 or skip_bits > MAX_SKIP_BITS) {
            @compileError("SkippingList: skip_bits must be between 1 and " ++ std.fmt.bufPrint("{d}", .{MAX_SKIP_BITS}) ++ ".");
        }
    }


    return struct {
        const Self = @This();

        pub fn init(
            allocator: std.mem.Allocator,
            initialCapacity: usize,
            sourceData: []const T
        ) !Self {
        }
    };
}

const testing = std.testing;

// // A simple comparison function for i32, required by SkippingList.
// fn i32LessThan(a: i32, b: i32) bool {
//     return a < b;
// }

// test "init and deinit" {
//     var list = try SkippingList(i32).initCapacity(testing.allocator, 10, i32LessThan);
//     defer list.deinit();

//     // The test passes if init succeeds and deinit completes without errors.
//     // The testing.allocator will automatically detect any memory leaks.
//     try testing.expect(list.level == 0);
//     try testing.expect(list.header.level == MAX_LEVEL);
//     try testing.expect(list.header.forward[0] == null);
// }

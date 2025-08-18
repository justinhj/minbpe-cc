const std = @import("std");

pub fn SkippingList(comptime T: type, skip_bits: T) type {
    const MAX_SKIP_BITS = 16;

    comptime {
        const T_info = @typeInfo(T);
        if (!(T_info == .int and T_info.int.signedness == .unsigned and T_info.int.bits >= 32)) {
            @compileError("SkippingList(T) requires T to be a numeric integer type 32 bits and up.");
        }
        if (skip_bits == 0 or skip_bits > MAX_SKIP_BITS) {
            @compileError("SkippingList: skip_bits must be between 1 and " ++ std.fmt.bufPrint("{d}", .{MAX_SKIP_BITS}) ++ ".");
        }
    }

    return struct {
        const Self = @This();
        const T_BITS = @typeInfo(T).Int.bits;
        const SHIFT_AMOUNT = T_BITS - skip_bits;
        const VALUE_MASK = std.math.maxInt(T) >> skip_bits;
        const MAX_SKIP_VALUE = (@as(T, 1) << skip_bits) - 1;

        allocator: std.mem.Allocator,
        data: []T,

        pub fn init(
            allocator: std.mem.Allocator,
            sourceData: []const T
        ) !Self {
            const data = try allocator.alloc(T, sourceData.len);
            errdefer allocator.free(data);
            @memcpy(data, sourceData);
            return Self{
                .allocator = allocator,
                .data = data,
            };
        }

        pub fn deinit(self: *Self) void {
            self.allocator.free(self.data);
        }
        
        pub fn get_skip(self: Self, index: usize) T {
            return self.data[index] >> SHIFT_AMOUNT;
        }

        pub fn get_value(self: Self, index: usize) T {
            return self.data[index] & VALUE_MASK;
        }

        pub fn set_skip(self: *Self, index: usize, skip: T) void {
            std.debug.assert(skip <= MAX_SKIP_VALUE);
            const value_part = self.data[index] & VALUE_MASK;
            const skip_part = skip << SHIFT_AMOUNT;
            self.data[index] = value_part | skip_part;
        }
    };
}

const testing = std.testing;

test "init and deinit" {
    const allocator = testing.allocator;
    const source_data = [_]u32{ 1, 2, 3, 4, 5 };
    var list = try SkippingList(u32, 8).init(allocator, &source_data);
    defer list.deinit();

    try testing.expectEqualSlices(u32, &source_data, list.data);
}


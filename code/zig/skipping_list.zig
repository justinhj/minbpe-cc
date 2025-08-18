const std = @import("std");

pub fn SkippingList(comptime T: type, comptime skip_bits: u4) type {
    const MAX_SKIP_BITS = 16;

    comptime {
        const T_info = @typeInfo(T);
        if (!(T_info == .int and T_info.int.signedness == .unsigned and T_info.int.bits >= 32)) {
            @compileError("SkippingList(T) requires T to be an unsigned integer type of 32 bits or more.");
        }
        if (skip_bits == 0 or skip_bits > MAX_SKIP_BITS) {
            @compileError("SkippingList: skip_bits must be between 1 and " ++ std.fmt.comptimePrint("{}", .{MAX_SKIP_BITS}) ++ ".");
        }
    }

    return struct {
        const Self = @This();
        const T_BITS = @typeInfo(T).int.bits;
        const SHIFT_AMOUNT = T_BITS - skip_bits;
        const VALUE_MASK = std.math.maxInt(T) >> skip_bits;
        const MAX_SKIP_VALUE = (@as(T, 1) << skip_bits) - 1;

        allocator: std.mem.Allocator,
        data: []T,

        pub fn init(
            allocator: std.mem.Allocator,
            sourceData: []const T,
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

        // --- ADDED ITERATOR ---

        pub const Iterator = struct {
            list: *const Self,
            index: usize,

            /// Returns the value of the next element and advances the iterator
            /// by `1 + skip_amount`. Returns `null` at the end.
            pub fn next(it: *Iterator) ?T {
                if (it.index >= it.list.data.len) {
                    return null;
                }

                const current_value = it.list.get_value(it.index);
                const skip_amount = it.list.get_skip(it.index);

                // Advance the index for the next call. A skip of 0 means we
                // just advance to the very next element.
                it.index += @as(usize, @intCast(skip_amount)) + 1;

                return current_value;
            }
        };

        /// Returns an iterator that traverses the list, respecting skip values.
        pub fn iterator(self: *const Self) Iterator {
            return Iterator{
                .list = self,
                .index = 0,
            };
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

// --- ADDED TEST FOR THE ITERATOR ---
test "iterator and skipping" {
    const allocator = testing.allocator;
    const MyList = SkippingList(u32, 8);
    const source_data = [_]u32{ 10, 20, 30, 40, 50 };
    var list = try MyList.init(allocator, &source_data);
    defer list.deinit();

    // Set element at index 1 (value 20) to skip 1 element ahead.
    // The iterator should visit 10, then 20, then jump to 40 (skipping 30).
    list.set_skip(1, 1);

    var sum: u32 = 0;
    var it = list.iterator();
    while (it.next()) |value| {
        sum += value;
    }

    // Expected sum is 10 (index 0) + 20 (index 1) + 40 (index 3) + 50 (index 4) = 120
    const expected_sum: u32 = 10 + 20 + 40 + 50;
    try testing.expectEqual(expected_sum, sum);
}

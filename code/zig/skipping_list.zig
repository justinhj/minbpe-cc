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

        fn get_skip(self: Self, index: usize) T {
            return self.data[index] >> SHIFT_AMOUNT;
        }

        fn get_value(self: Self, index: usize) T {
            return self.data[index] & VALUE_MASK;
        }

        fn set_skip(self: *Self, index: usize, skip: T) void {
            std.debug.assert(skip <= MAX_SKIP_VALUE);
            const value_part = self.data[index] & VALUE_MASK;
            const skip_part = skip << SHIFT_AMOUNT;
            self.data[index] = value_part | skip_part;
        }

        pub const Iterator = struct {
            list: *Self,
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

            pub fn replaceAndSkipNext(it: *Iterator, new_value: T) void {
                std.debug.assert(it.index + 1 < it.list.data.len);

                // Set the skip bits to 1, preserving the original value for now.
                it.list.set_skip(it.index, 1);

                // Now, replace the value part, keeping the new skip bits.
                // 1. Get the raw data which now has the skip bits set.
                const raw_data = it.list.data[it.index];
                // 2. Isolate the just-set skip bits.
                const skip_part = raw_data & ~@as(T, VALUE_MASK);
                // 3. Combine with the new value.
                it.list.data[it.index] = skip_part | new_value;
            }
        };

        /// Returns an iterator that traverses the list, respecting skip values.
        pub fn iterator(self: *Self) Iterator {
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

test "replace pairs" {
    const allocator = testing.allocator;
    const MyList = SkippingList(u32, 8);
    const source_data = [_]u32{ 10, 20, 10, 20, 50, 60, 70, 10, 20, 0, 0 };
    var list = try MyList.init(allocator, &source_data);
    defer list.deinit();

    // --- Phase 1: Modify the list ---
    // Replace every pair of (10, 20) with a single 50.
    var mut_it = list.iterator();
    while (mut_it.index < list.data.len - 1) {
        const current_val = list.get_value(mut_it.index);
        const next_val = list.get_value(mut_it.index + 1);

        if (current_val == 10 and next_val == 20) {
            // Replace the current item (10) with 50 and set its skip to 1
            // to jump over the next item (20).
            mut_it.replaceAndSkipNext(50);
        }
        _ = mut_it.next(); // Advance the iterator
    }

    // --- Phase 2: Verify the result ---
    // The new logical sequence of values should be: { 50, 50, 50, 60, 70, 50, 0, 0 }
    var sum: u32 = 0;
    var final_it = list.iterator();
    while (final_it.next()) |value| {
        sum += value;
    }

    const expected_sum: u32 = 50 + 50 + 50 + 60 + 70 + 50 + 0 + 0; // 330
    try std.testing.expectEqual(expected_sum, sum);
}

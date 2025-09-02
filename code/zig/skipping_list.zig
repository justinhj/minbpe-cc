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
        const T_BITS: T = @typeInfo(T).int.bits;
        const SHIFT_AMOUNT: T = T_BITS - skip_bits;
        const VALUE_MASK: T = std.math.maxInt(T) >> skip_bits;
        const MAX_SKIP_VALUE: T = (@as(T, 1) << skip_bits) - 1;
        const MAX_VALUE: T = (@as(T, 1) << T_BITS - skip_bits) - 1;

        allocator: std.mem.Allocator,
        data: []T,
        size: usize,

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
                .size = sourceData.len,
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

        // Set the value of this element without changing the skip bits
        fn set_value(self: *Self, index: usize, value: T) void {
            std.debug.assert(value <= MAX_VALUE);
            const skip_part = self.data[index] & ~VALUE_MASK;
            self.data[index] = skip_part | value;
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

            // Looks at the next element without advancing the iterator.
            // Returns null if at the end.
            pub fn peek(it: *Iterator) ?T {
                if (it.findNextIndex()) |next_idx| {
                    return it.list.get_value(next_idx);
                }
                return null;
            }

            /// Like peek but look at the next next element
            /// peekN may be useful but I don't need it
            pub fn peekpeek(it: *Iterator) ?T {
                if (it.findNextIndex()) |first_idx| {
                    var temp_it = it.*;
                    temp_it.index = first_idx;
                    if (temp_it.findNextIndex()) |second_idx| {
                        return temp_it.list.get_value(second_idx);
                    }
                }
                return null;
            }

            /// Returns the value of the next element and advances the iterator
            /// by `1 + skip_amount`. Returns `null` at the end.
            pub fn next(it: *Iterator) ?T {
                if (it.findNextIndex()) |next_idx| {
                    // std.debug.print("Iterator next: moving to index {}\n", .{next_idx});
                    it.index = next_idx;
                    return it.list.get_value(next_idx);
                } else {
                    // Mark the iterator as finished by setting the index to the end
                    // to prevent re-scanning on subsequent calls.
                    // std.debug.print("Iterator next: the end {}\n", .{it.list.data.len});
                    it.index = it.list.data.len;
                    return null;
                }
            }

            // Helper to find the index of the next non-deleted element
            // Input... the iterator is at the current position stored as index
            //   we expect index to be a valid non skipped item (high bits zero) or
            //   the initial state (maxInt)
            //   or it could be at the end of the collection (>= len)
            // Output... advance the index by one and repeat until we find a non-skipped item
            //   or reach the end of the collection
            pub fn findNextIndex(it: *const Iterator) ?usize {
                const initial_state = it.index == std.math.maxInt(usize);

                // Already at or past end? Nothing left
                if (!initial_state and it.index + 1 >= it.list.data.len) {
                    return null;
                }

                // Where to start scanning
                var idx: usize = if (initial_state) 0 else it.index + 1;

                // Scan forward until non-skipped or end
                while (idx < it.list.data.len) {
                    const skip_amount = it.list.get_skip(idx);
                    if (skip_amount == 0) {
                        return idx; // found usable element
                    }
                    idx += @as(usize, @intCast(skip_amount));
                }

                return null;
            }

            /// Replaces the current value with `new_value` and sets the skip bits
            pub fn replaceAndSkipNext(it: *Iterator, new_value: T) void {
                if (it.index == std.math.maxInt(usize)) {
                    // Can't replace before the first next() call
                    return;
                }

                if (it.index >= it.list.data.len) {
                    // Can't replace after the end
                    return;
                }

                it.list.set_value(it.index, new_value);

                if (it.findNextIndex()) |next_idx| {
                    it.list.set_skip(next_idx, 1);
                    it.list.size -= 1;
                }
            }
        };

        pub fn get_size(self: Self) usize {
            return self.size;
        }

        /// Returns an iterator that traverses the list, respecting skip values.
        pub fn iterator(self: *Self) Iterator {
            return Iterator{
                .list = self,
                .index = std.math.maxInt(usize), // Special initial state
            };
        }

        pub const DebugIterator = struct {
            list: *const Self,
            index: usize,

            pub fn next(it: *DebugIterator) ?T {
                if (it.index >= it.list.data.len) {
                    return null;
                }
                const raw_value = it.list.data[it.index];
                it.index += 1;
                return raw_value;
            }
        };

        /// Returns a debug iterator that traverses the list, returning raw values.
        pub fn debug_iterator(self: *const Self) DebugIterator {
            return DebugIterator{
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

test "peek, next and peekpeek" {
    const allocator = testing.allocator;

    const MyList = SkippingList(u32, 8);
    const source_data = [_]u32{ 10, 20 };
    var list = try MyList.init(allocator, &source_data);
    defer list.deinit();

    var it = list.iterator();
    try testing.expectEqual(10, it.peek().?);
    try testing.expectEqual(20, it.peekpeek().?);
    try testing.expectEqual(10, it.next().?);
    try testing.expectEqual(20, it.peek().?);
    try testing.expectEqual(null, it.peekpeek());
    try testing.expectEqual(20, it.next().?);
    try testing.expectEqual(null, it.peek());
    try testing.expectEqual(null, it.next());
    try testing.expectEqual(null, it.peek());
    try testing.expectEqual(null, it.peekpeek());
}

test "replaceAndSkipNext" {
    const allocator = testing.allocator;

    const MyList = SkippingList(u32, 8);
    const source_data = [_]u32{ 10, 20, 30, 40, 10, 20 };
    var list = try MyList.init(allocator, &source_data);
    defer list.deinit();

    var it = list.iterator();
    try testing.expectEqual(10, it.next().?);
    it.replaceAndSkipNext(90);

    try testing.expectEqual(30, it.next().?);
    try testing.expectEqual(40, it.next().?);
    it.replaceAndSkipNext(100);
    try testing.expectEqual(20, it.next().?);
    try testing.expectEqual(null, it.next());

    it = list.iterator();
    try testing.expectEqual(0, it.findNextIndex().?);
    try testing.expectEqual(90, it.next().?);
    try testing.expectEqual(30, it.next().?);
    try testing.expectEqual(100, it.next().?);
    try testing.expectEqual(20, it.next().?);
}

test "iterator and skipping" {
    const allocator = testing.allocator;
    const MyList = SkippingList(u32, 8);
    const source_data = [_]u32{ 10, 20, 30, 40, 50 };
    var list = try MyList.init(allocator, &source_data);
    defer list.deinit();

    // Set element at index 1 (value 20) to skip 1 element ahead.
    // The iterator should visit 10, then jump to 30 (skipping 20).
    list.set_skip(1, 1);

    var sum: u32 = 0;
    var it = list.iterator();
    while (it.next()) |value| {
        sum += value;
    }

    // Expected sum is 10 (from index 0) + 40 (from index 3) + 50 (from index 4) = 100
    const expected_sum: u32 = 10 + 30 + 40 + 50;
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
    while (mut_it.next()) |current_val| {
        if (mut_it.peek()) |next_val| {
            if (current_val == 10 and next_val == 20) {
                mut_it.replaceAndSkipNext(50);
            }
        }
    }

    // --- Phase 2: Verify the result ---
    var raw_values = std.ArrayList(u32).init(allocator);
    defer raw_values.deinit();

    var test_it = list.iterator();
    while (test_it.next()) |raw_value| {
        try raw_values.append(raw_value);
    }

    const expected_values = [_]u32{ 50, 50, 50, 60, 70, 50, 0, 0 };
    try testing.expectEqualSlices(u32, &expected_values, raw_values.items);
}

test "debug iterator" {
    const allocator = testing.allocator;
    const MyList = SkippingList(u32, 8);
    const source_data = [_]u32{ 10, 20, 30, 40, 50 };
    var list = try MyList.init(allocator, &source_data);
    defer list.deinit();

    list.set_skip(1, 1); // This should be ignored by the debug iterator

    var raw_values = std.ArrayList(u32).init(allocator);
    defer raw_values.deinit();

    var it = list.debug_iterator();
    while (it.next()) |raw_value| {
        try raw_values.append(raw_value);
    }

    const expected_skip_value = (@as(u32, 1) << 24) | 20;
    const expected_values = [_]u32{ 10, expected_skip_value, 30, 40, 50 };

    try testing.expectEqualSlices(u32, &expected_values, raw_values.items);
}

test "big gap" {
    const allocator = testing.allocator;
    const MyList = SkippingList(u32, 2); // 2 bits for skip, max skip is 4

    // 1. Create a list with numbers 1 to 31
    var source_data_array: [31]u32 = undefined;
    var i: u32 = 0;
    while (i < source_data_array.len) : (i += 1) {
        source_data_array[i] = i + 1;
    }
    var list = try MyList.init(allocator, &source_data_array);
    defer list.deinit();

    // 2. Iteratively update the 8th element and skip the 9th, 16 times
    var j: u32 = 0;
    while (j < 16) : (j += 1) {
        var finder_it = list.iterator();
        var k: u32 = 0;
        var value = finder_it.next();
        while (k < 8 - 1) : (k += 1) {
            value = finder_it.next();
        }
        if (value) |v| {
            finder_it.replaceAndSkipNext(v + 1);
        }
    }

    // 3. Verify the final list.
    // The test modifies the list in a way that causes the iterator to skip over
    // several elements. We verify that the sequence of values produced by the
    // iterator is correct.
    var final_values = std.ArrayList(u32).init(allocator);
    defer final_values.deinit();

    var it = list.iterator();
    while (it.next()) |v| {
        try final_values.append(v);
    }

    const expected_values = [_]u32{
        1, 2, 3, 4, 5, 6, 7, 24, 25, 26, 27, 28, 29, 30, 31,
    };

    try testing.expectEqualSlices(u32, &expected_values, final_values.items);
}

fn mergePairs(
    comptime T: type,
    comptime skip_bits: u4,
    list: *SkippingList(T, skip_bits),
    left: T,
    right: T,
    replacement: T,
) !void {
    var it = list.iterator();
    while (it.next()) |current_val| {
        const next_val = it.peek() orelse break;

        if (current_val == left and next_val == right) {
            it.replaceAndSkipNext(replacement);
        }
    }
}

test "size" {
    const allocator = testing.allocator;

    const MyList = SkippingList(u32, 8);
    const source_data = [_]u32{ 10, 20, 30, 40, 10, 20 };
    var list = try MyList.init(allocator, &source_data);
    defer list.deinit();

    try testing.expectEqual(@as(usize, 6), list.get_size());

    var it = list.iterator();
    _ = it.next(); // 10
    it.replaceAndSkipNext(90); // replaces 10 with 90, skips 20. size should be 5

    try testing.expectEqual(@as(usize, 5), list.get_size());

    _ = it.next(); // 30
    _ = it.next(); // 40
    it.replaceAndSkipNext(100); // replaces 40 with 100, skips 10. size should be 4

    try testing.expectEqual(@as(usize, 4), list.get_size());
}

test "merge pairs" {
    const allocator = testing.allocator;
    const MyList = SkippingList(u32, 8);
    const source_data = [_]u32{ 97, 98, 99, 98, 99, 100, 101 };
    var list = try MyList.init(allocator, &source_data);
    defer list.deinit();

    try mergePairs(u32, 8, &list, 98, 99, 256);

    // Collect the results from the iterator to verify the merge logic
    var final_values = std.ArrayList(u32).init(allocator);
    defer final_values.deinit();
    var it = list.iterator();
    while (it.next()) |v| {
        try final_values.append(v);
    }

    const expected_values = [_]u32{ 97, 256, 256, 100, 101 };
    try testing.expectEqualSlices(u32, &expected_values, final_values.items);

    try mergePairs(u32, 8, &list, 256, 256, 257);

    // Collect the results from the iterator to verify the merge logic
    var final_values2 = std.ArrayList(u32).init(allocator);
    defer final_values2.deinit();
    var it2 = list.iterator();
    while (it2.next()) |v| {
        try final_values2.append(v);
    }

    const expected_values2 = [_]u32{ 97, 257, 100, 101 };
    try testing.expectEqualSlices(u32, &expected_values2, final_values2.items);
}

// ============================================================================
// C API
// ============================================================================

const C_API_T = u32;
const C_API_SKIP_BITS = 8;
const C_SkippingListType = SkippingList(C_API_T, C_API_SKIP_BITS);

pub const CSkippingList = C_SkippingListType;
pub const CSkippingListIterator = CSkippingList.Iterator;


// Can rename later but now for use I for interface
pub const ISkippingListIterator = extern struct {
    list: *CSkippingList,
    index: usize,
};

/// Creates a SkippingList from a C array.
/// The list creates its own copy of the data.
/// The caller owns the returned pointer and must free it with skipping_list_destroy.
/// Returns null on allocation failure.
export fn skipping_list_create(source_data: [*c]const C_API_T, len: usize) ?*CSkippingList {
    // We need an allocator. The C++ side doesn't provide one.
    // We can use a general-purpose allocator.
    var gpa = std.heap.c_allocator;
    const list = gpa.create(CSkippingList) catch return null;

    const slice = source_data[0..len];
    list.* = CSkippingList.init(gpa, slice) catch {
        gpa.destroy(list);
        return null;
    };
    return list;
}

/// Destroys a SkippingList instance.
export fn skipping_list_destroy(list: *CSkippingList) void {
    const allocator = list.allocator;
    list.deinit();
    allocator.destroy(list);
}

/// Creates an iterator for the list.
export fn skipping_list_iterator_create(list: *CSkippingList) ISkippingListIterator {
    const iter = list.iterator();
    const c_iter = ISkippingListIterator{
        .list = iter.list,
        .index = iter.index
    };
    return c_iter;
}

/// Advances the iterator and gets the next value.
/// Returns true if a value was retrieved, false if the end of the list was reached.
export fn skipping_list_iterator_next(c_iter: *ISkippingListIterator, out_value: *C_API_T) bool {
    var iter = CSkippingListIterator{
        .list = c_iter.list,
        .index = c_iter.index
    };
    if (iter.next()) |value| {
        c_iter.index = iter.index;
        c_iter.list = iter.list;
        out_value.* = value;
        return true;
    } else {
        c_iter.index = iter.index;
        c_iter.list = iter.list;
        return false;
    }
}

export fn skipping_list_iterator_peek(c_iter: *ISkippingListIterator, out_value: *C_API_T) bool {
    var iter = CSkippingListIterator{
        .list = c_iter.list,
        .index = c_iter.index
    };
    if (iter.peek()) |value| {
        c_iter.index = iter.index;
        c_iter.list = iter.list;
        out_value.* = value;
        return true;
    } else {
        c_iter.index = iter.index;
        c_iter.list = iter.list;
        return false;
    }
}

/// Replaces the current value in the list with new_value and skips the next element.
export fn skipping_list_iterator_replace_and_skip_next(c_iter: *ISkippingListIterator, new_value: C_API_T) void {
    var iter = CSkippingListIterator{
        .list = c_iter.list,
        .index = c_iter.index
    };
    iter.replaceAndSkipNext(new_value);
    c_iter.index = iter.index;
    c_iter.list = iter.list;
}

/// Returns the number of elements in the list.
export fn skipping_list_size(list: *const CSkippingList) usize {
    return list.get_size();
}

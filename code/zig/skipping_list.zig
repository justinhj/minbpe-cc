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
                const initial_state = it.index == std.math.maxInt(usize);
                // Check for already at the end but not when initial state
                if (!initial_state and it.index + 1 >= it.list.data.len) {
                    return null;
                }
                // Initial state means no skip value to consider, otherwise get the skip value
                const skip_amount = if (initial_state) 0 else it.list.get_skip(it.index);
                var index = if (initial_state) 0 else it.index + @as(usize, @intCast(skip_amount)) + 1;
                // Skips can chain so loop over them
                while (index < it.list.data.len) {
                    const next_skip = it.list.get_skip(index);
                    if (next_skip == 0) {
                        it.index = index;
                        return it.list.get_value(index);
                    } else {
                        index += @as(usize, @intCast(next_skip)) + 1;
                    }
                }
                it.index = index; // Remember we got to the end
                return null;
            }

            // Example of input and output

            // input: skipping list with the content 1 2 1 2 4 5 6, replace 1, 2 with 10
            // output: skipping list content 10, 10, 4, 5, 6

            // it has replaced the target pair
            // it should use only the iterator interface as shown

            // 0 1 2 1 2 3 4

            // current = null
            // peek = 0
            // peek2 = 1

            // check for pair
            // replace and skip next

            // or next

            // skip a 0 b 0 c 0 d 0
            // delete b from a
            // skip a 1 b 0 c 0 d 0
            // iterate gives a c d

            // delete c from a
            // if skip is already non zero we cannot simply skip 1 again
            // find the next non skipped element

            // index = 0
            // follow the skip
            // index = 2
            // is it skipped?
            // no
            // so skip it

            // a 1 b 0 c 1 d 0

            // iterate gives a d
            // delete d from a

            // skip is non zero so find next non skipped

            // index = 3 (d)
            // skip d to 1

            // a 1 b 0 c 1 d 1
            // iterate gives a

            // ok let's try where c is deleted and we start at a again

            // a 0 b 1 c 0 d 0

            // at a delete b

            // a
            // skip is non zero so find next non skipped

            // algorithm
            // when the current element has no skip it means the next element is not deleted by definition

            // Helper to find the index of the next non-deleted element
            pub fn findNextIndex(it: *const Iterator) ?usize {
                const initial_state = it.index == std.math.maxInt(usize);
                if (!initial_state and it.index + 1 >= it.list.data.len) {
                    return null;
                }
                const skip_amount = if (initial_state) 0 else it.list.get_skip(it.index);
                var idx = if (initial_state) 0 else it.index + @as(usize, @intCast(skip_amount)) + 1;
                while (idx < it.list.data.len) {
                    const next_skip = it.list.get_skip(idx);
                    if (next_skip == 0) {
                        return idx;
                    } else {
                        idx += @as(usize, @intCast(next_skip)) + 1;
                    }
                }
                return null;
            }

            /// Replaces the current value with `new_value` and sets the skip bits
            /// TODO consider replacing this with two methods, a set and erase_next
            /// For now this matches my limited use case though
            pub fn replaceAndSkipNext(it: *Iterator, new_value: T) void {
                // TODO these should be user errors but just assert for now
                std.debug.assert(it.index != std.math.maxInt(usize));
                std.debug.assert(it.index + 1 < it.list.data.len);

                it.list.set_value(it.index, new_value);

                if (it.findNextIndex()) |next_idx| {
                    var temp_it = it.*;
                    temp_it.index = next_idx;
                    const next_next_idx_opt = temp_it.findNextIndex();
                    const target_idx = next_next_idx_opt orelse it.list.data.len;

                    var current: usize = it.index;
                    var remaining: isize = @as(isize, @intCast(target_idx)) - @as(isize, @intCast(current)) - 1;
                    while (remaining > 0) : (remaining = @as(isize, @intCast(target_idx)) - @as(isize, @intCast(current)) - 1) {
                        var skip_val: isize = @min(remaining, @as(isize, @intCast(Self.MAX_SKIP_VALUE)));
                        if (remaining == @as(isize, @intCast(Self.MAX_SKIP_VALUE)) + 1) {
                            skip_val = @as(isize, @intCast(Self.MAX_SKIP_VALUE)) - 1;
                        }
                        it.list.set_skip(current, @as(T, @intCast(skip_val)));
                        current += @as(usize, @intCast(skip_val)) + 1;
                    }
                }
            }
        };

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

test "iterator and skipping" {
    const allocator = testing.allocator;
    const MyList = SkippingList(u32, 8);
    const source_data = [_]u32{ 10, 20, 30, 40, 50 };
    var list = try MyList.init(allocator, &source_data);
    defer list.deinit();

    // Set element at index 1 (value 20) to skip 1 element ahead.
    // The iterator should visit 10, then jump to 40 (skipping 20 and 30).
    list.set_skip(1, 1);

    var sum: u32 = 0;
    var it = list.iterator();
    while (it.next()) |value| {
        sum += value;
    }

    // Expected sum is 10 (from index 0) + 40 (from index 3) + 50 (from index 4) = 100
    const expected_sum: u32 = 10 + 40 + 50;
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
        if (mut_it.peek()) |current_val| {
            if (mut_it.peekpeek()) |next_val| {
                if (current_val == 10 and next_val == 20) {
                    // Replace the current item (10) with 50 and set its skip to 1
                    // to jump over the next item (20).
                    mut_it.replaceAndSkipNext(50);
                    continue; // Continue without advancing the iterator
                }
            }
        }
        _ = mut_it.next(); // Advance the iterator
    }

    // --- Phase 2: Verify the result with debug_iterator ---
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

// test "big gap" {
//     const allocator = testing.allocator;
//     const MyList = SkippingList(u32, 2); // 2 bits for skip, max skip is 4

//     // 1. Create a list with numbers 1 to 31
//     var source_data_array: [31]u32 = undefined;
//     var i: u32 = 0;
//     while (i < source_data_array.len) : (i += 1) {
//         source_data_array[i] = i + 1;
//     }
//     var list = try MyList.init(allocator, &source_data_array);
//     defer list.deinit();

//     // 2. Iteratively "delete" the 9th element 16 times
//     var j: u32 = 0;
//     while(j < 16) : (j +=1) {
//         var finder_it = list.iterator();
//         var k: u32 = 0;
//         var value = finder_it.next();
//         while (k < 8 - 1) : (k += 1) {
//             value = finder_it.next();
//         }
//         if (value) |v| {
//             finder_it.replaceAndSkipNext(v + 1);
//         }
//     }

//     // 3. Verify the final list.
//     // The test modifies the list in a way that causes the iterator to skip over
//     // several elements. We verify that the sequence of values produced by the
//     // iterator is correct.
//     var final_values = std.ArrayList(u32).init(allocator);
//     defer final_values.deinit();

//     var it = list.iterator();
//     while (it.next()) |v| {
//         try final_values.append(v);
//     }

//     const expected_values = [_]u32{
//         1, 2, 3, 4, 5, 6, 7, 8, 24, 25, 26, 27, 28, 29, 30, 31,
//     };

//     try testing.expectEqualSlices(u32, &expected_values, final_values.items);
// }

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

// test "merge pairs" {
//     const allocator = testing.allocator;
//     const MyList = SkippingList(u32, 8);
//     const source_data = [_]u32{ 97, 98, 99, 98, 99, 100, 101 };
//     var list = try MyList.init(allocator, &source_data);
//     defer list.deinit();

//     try mergePairs(u32, 8, &list, 98, 99, 256);

//     // Collect the results from the iterator to verify the merge logic
//     var final_values = std.ArrayList(u32).init(allocator);
//     defer final_values.deinit();
//     var it = list.iterator();
//     while (it.next()) |v| {
//         try final_values.append(v);
//     }

//     const expected_values = [_]u32{ 97, 256, 256, 100, 101 };
//     try testing.expectEqualSlices(u32, &expected_values, final_values.items);

//     try mergePairs(u32, 8, &list, 256, 256, 257);

//     // Collect the results from the iterator to verify the merge logic
//     var final_values2 = std.ArrayList(u32).init(allocator);
//     defer final_values2.deinit();
//     var it2 = list.iterator();
//     while (it2.next()) |v| {
//         try final_values2.append(v);
//     }

//     const expected_values2 = [_]u32{ 97, 257, 100, 101 };
//     try testing.expectEqualSlices(u32, &expected_values2, final_values2.items);
// }

// ============================================================================
// C API
// ============================================================================

const C_API_T = u32;
const C_API_SKIP_BITS = 8;
const C_SkippingListType = SkippingList(C_API_T, C_API_SKIP_BITS);

// In Zig, we can use a more descriptive name. C will see the exported name.
pub const CSkippingList = C_SkippingListType;
pub const CSkippingListIterator = CSkippingList.Iterator;

/// Creates a SkippingList from a C array.
/// The list creates its own copy of the data.
/// The caller owns the returned pointer and must free it with skipping_list_destroy.
/// Returns null on allocation failure.
export fn skipping_list_create(source_data: [*c]const C_API_T, len: usize) ?*CSkippingList {
    // We need an allocator. The C++ side doesn't provide one.
    // We can use a general-purpose allocator.
    var gpa = std.heap.page_allocator;
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
/// The caller owns the returned pointer and must free it with skipping_list_iterator_destroy.
/// Returns null on allocation failure.
export fn skipping_list_iterator_create(list: *CSkippingList) ?*CSkippingListIterator {
    const iter = list.iterator();
    const iter_ptr = list.allocator.create(CSkippingListIterator) catch return null;
    iter_ptr.* = iter;
    return iter_ptr;
}

/// Destroys a list iterator.
export fn skipping_list_iterator_destroy(iter: *CSkippingListIterator) void {
    // The iterator contains a pointer to the list, which contains the allocator.
    iter.list.allocator.destroy(iter);
}

/// Advances the iterator and gets the next value.
/// Returns true if a value was retrieved, false if the end of the list was reached.
export fn skipping_list_iterator_next(iter: *CSkippingListIterator, out_value: *C_API_T) bool {
    if (iter.next()) |value| {
        out_value.* = value;
        return true;
    } else {
        return false;
    }
}

export fn skipping_list_iterator_peek(iter: *CSkippingListIterator, out_value: *C_API_T) bool {
    if (iter.peek()) |value| {
        out_value.* = value;
        return true;
    } else {
        return false;
    }
}

/// Replaces the current value in the list with new_value and skips the next element.
export fn skipping_list_iterator_replace_and_skip_next(iter: *CSkippingListIterator, new_value: C_API_T) void {
    iter.replaceAndSkipNext(new_value);
}

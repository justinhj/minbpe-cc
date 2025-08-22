#ifndef SKIPPING_LIST_H
#define SKIPPING_LIST_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Note: Created by Gemini Pro 2.5 because Zig 0.14.0 does not emit headers

// Opaque struct declarations. The internal structure is managed by Zig.
typedef struct CSkippingList CSkippingList;
typedef struct CSkippingListIterator CSkippingListIterator;

/**
 * @brief Creates a SkippingList from a C array.
 *
 * The list creates its own copy of the data.
 * @param source_data A pointer to the source array of uint32_t.
 * @param len The number of elements in the source array.
 * @return A pointer to the newly created list, or NULL on allocation failure.
 * The caller owns this pointer and must free it with skipping_list_destroy.
 */
CSkippingList* skipping_list_create(const uint32_t* source_data, size_t len);

/**
 * @brief Destroys a SkippingList instance and frees its memory.
 *
 * @param list A pointer to the SkippingList to be destroyed.
 */
void skipping_list_destroy(CSkippingList* list);

/**
 * @brief Creates an iterator for the list.
 *
 * @param list A pointer to the SkippingList to iterate over.
 * @return A pointer to the new iterator, or NULL on allocation failure.
 * The caller owns this pointer and must free it with skipping_list_iterator_destroy.
 */
CSkippingListIterator* skipping_list_iterator_create(CSkippingList* list);

/**
 * @brief Destroys a list iterator and frees its memory.
 *
 * @param iter A pointer to the iterator to be destroyed.
 */
void skipping_list_iterator_destroy(CSkippingListIterator* iter);

/**
 * @brief Advances the iterator and gets the next value.
 *
 * @param iter A pointer to the iterator.
 * @param out_value A pointer to a uint32_t where the next value will be stored.
 * @return true if a value was retrieved, false if the end of the list was reached.
 */
bool skipping_list_iterator_next(CSkippingListIterator* iter, uint32_t* out_value);

// Same as next but does not advance the iterator
bool skipping_list_iterator_peek(CSkippingListIterator* iter, uint32_t* out_value);

/**
 * @brief Replaces the value at the iterator's current position and sets it to skip the next element.
 *
 * @param iter A pointer to the iterator.
 * @param new_value The new value to write into the list.
 */
void skipping_list_iterator_replace_and_skip_next(CSkippingListIterator* iter, uint32_t new_value);


#ifdef __cplusplus
}
#endif

#endif // SKIPPING_LIST_H

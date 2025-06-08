#ifndef MINBPE_TOKENIZER_HPP
#define MINBPE_TOKENIZER_HPP

#include <algorithm>
#include <unordered_map>
#include <string>
#include <functional>
#include <forward_list>
#include <vector>
#include <cassert>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <utility>
#include <optional> // Using std::optional
#include <stdexcept> // For std::runtime_error

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h> // Main PCRE2 header

#include "PairCount.h" // Assuming this is a local header

using std::string;
using std::unordered_map;
using std::vector;
using std::optional; // Now using std::optional
using std::cout;
using std::filesystem::path;
using std::pair;
using std::ios;
using std::make_pair;

using namespace MinBpeCC::Util; // Assuming this namespace contains PairCount

namespace MinBpeCC::Tokenizer {
    // Hash function for std::pair<int,int>
    inline std::function<std::size_t(const pair<int,int>&)> pair_int_int_hash =
        [](const pair<int,int>& k) -> std::size_t {
            std::size_t seed = 0;
            std::hash<int> hasher;
            seed ^= hasher(std::get<0>(k)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= hasher(std::get<1>(k)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            return seed;
    };

    class Tokenizer {
    public:
        // Static regular expression patterns
        inline const static std::string GPT2_SPLIT_PATTERN = "'(?:[sdmt]|ll|ve|re)| ?\\p{L}+| ?\\p{N}+| ?[^\\s\\p{L}\\p{N}]+|\\s+(?!\\S)|\\s+";
        inline const static std::string GPT4_SPLIT_PATTERN = "'(?i:[sdmt]|ll|ve|re)|[^\\r\\n\\p{L}\\p{N}]?+\\p{L}+|\\p{N}{1,3}| ?[^\\s\\p{L}\\p{N}]++[\\r\\n]*|\\s*[\\r\\n]|\\s+(?!\\S)|\\s+";

    protected:
        static const auto bucket_size = 10;

        // --- PCRE2 specific members ---
        pcre2_code_8* compiled_pattern_pcre2;     // Compiled PCRE2 pattern
        pcre2_match_data_8* match_data_pcre2;     // Match data block for results

        // PCRE2 context objects for efficient memory management
        pcre2_general_context_8* general_context_pcre2;
        pcre2_compile_context_8* compile_context_pcre2;
        pcre2_match_context_8* match_context_pcre2;
        // --- End PCRE2 specific members ---

        unordered_map<pair<int,int>, int, decltype(pair_int_int_hash)> merges_lookup;
        vector<pair<int,int>> merges;
        vector<vector<int>> vocab;
        string pattern; // The string representation of the regex pattern

        // Helper to convert char to int, handling negative char values
        int char_to_int(char c) {
            return c < 0 ? c + 256 : c;
        }

        // Converts a string to a vector of ints (byte representation)
        std::vector<int> text_to_vector(const string &text) {
            std::vector<int> text_converted;
            text_converted.reserve(text.length()); // Reserve space to prevent reallocations
            for(char c : text) {
                text_converted.push_back(static_cast<int>(char_to_int(c)));
            }
            return text_converted;
        }

        // Initializes the vocabulary with 256 byte tokens
        void initialize_vocab() {
            vocab.clear();
            vocab.reserve(256); // Reserve space for initial bytes
            for(int i = 0; i < 256; i++) {
                vector<int> s;
                s.push_back(i);
                vocab.push_back(s);
            }
        }

        // Converts a vector of vector of ints (chunks) to a vector of forward_list of ints
        auto create_lists(const vector<vector<int>> &chunks) {
            vector<std::forward_list<int>> flists;
            flists.reserve(chunks.size()); // Reserve space
            for(const auto &chunk: chunks) {
                std::forward_list<int> flist;
                // Copy in reverse to use front_inserter efficiently
                std::copy(chunk.rbegin(), chunk.rend(), std::front_inserter(flist));
                flists.push_back(flist);
            }
            return flists;
        }

        // Calculates frequencies of adjacent pairs in the chunks
        auto calculate_freqs(const vector<std::forward_list<int>> &chunks) {
            PairCount freqs;
            for(const auto &chunk: chunks) {
                auto p1 = chunk.begin();
                auto p2 = std::next(p1);
                while(p1 != chunk.end() && p2 != chunk.end()) {
                    auto p = make_pair(*p1, *p2);
                    freqs.increment_freq_count(p);
                    ++p1;
                    ++p2;
                }
            }
            return freqs;
        }

        // Merges a specific pair within a single forward_list, updating frequencies
        void merge(std::forward_list<int> &text, pair<int,int> mp, int new_token, int insert_order, PairCount &freqs) {
            auto verbose = 0; // Control verbosity for debugging
            if(verbose >= 2) {
                cout << "before merge\n";
                for(auto c: text) {
                    cout << c << " ";
                }
                cout << "\n";
            }

            auto [p1_val, p2_val] = mp; // Deconstruct the pair
            auto i0 = text.before_begin();
            auto i1 = text.begin();
            auto i2 = std::next(i1);

            if(i2 == text.end()) {
                // No pairs to merge
                return;
            }

            auto i3 = std::next(i2);

            while(i1 != text.end() && i2 != text.end()) {
                if(verbose >= 1) {
                    cout << "i0 " << (i0 != text.before_begin() ? std::to_string(*i0) : "B_BEGIN")
                         << " i1 " << *i1 << " i2 " << *i2
                         << " i3 " << (i3 == text.end() ? "?" : std::to_string(*i3)) << "\n";
                }

                if(*i1 == p1_val && *i2 == p2_val) {
                    if(verbose >= 1) {
                        cout << "found pair " << p1_val << ", " << p2_val << " replace with " << new_token << "\n";
                    }

                    *i1 = new_token; // Replace the first element of the pair with the new token
                    i2 = text.erase_after(i1); // Erase the second element

                    // Update frequencies: decrement old pairs, increment new ones
                    auto& index_by_key = freqs.get_index_by_key();
                    auto f = index_by_key.find(mp);
                    if(f != freqs.end()) {
                        if(verbose >= 1) {
                            cout << "decrement replaced pair " << std::get<0>(f->pair) << ", " << std::get<1>(f->pair) << "\n";
                        }
                        freqs.decrement_freq_count(f->pair);
                    }

                    if(i0 != text.before_begin()) { // Check if i0 is a valid element (not before_begin())
                        auto prev_pair = make_pair(*i0, p1_val);
                        auto prev = index_by_key.find(prev_pair);
                        if(prev != freqs.end()) {
                            if(verbose >= 1) {
                                cout << "decrement previous pair " << std::get<0>(prev->pair) << ", " << std::get<1>(prev->pair) << "\n";
                            }
                            freqs.decrement_freq_count(prev->pair);
                        }
                        if(verbose >= 1) {
                            cout << "increment new previous pair " << *i0 << ", " << new_token << "\n";
                        }
                        freqs.increment_freq_count(make_pair(*i0, new_token));
                    }

                    if(i2 != text.end()) { // Check if i2 is a valid element (not end())
                        auto next_pair = make_pair(p2_val, *i2); // Original p2_val, not new_token
                        auto next = index_by_key.find(next_pair);
                        if(next != freqs.end()) {
                            if(verbose >= 1) {
                                cout << "decrement next pair " << std::get<0>(next->pair) << ", " << std::get<1>(next->pair) << "\n";
                            }
                            freqs.decrement_freq_count(next->pair);
                        } else {
                            if(verbose >= 1) {
                                cout << "next pair not found " << p2_val << ", " << *i2 << "\n";
                            }
                        }
                        if(verbose >= 1) {
                            cout << "increment new next pair " << new_token << ", " << *i2 << "\n";
                        }
                        freqs.increment_freq_count(make_pair(new_token, *i2));
                    }

                    // Iterators are already adjusted by erase_after,
                    // just make sure i3 points correctly if it's not end()
                    if (i2 != text.end()) {
                        i3 = std::next(i2);
                    } else {
                        i3 = text.end(); // If i2 is now end, i3 is also end
                    }

                    if(verbose >= 1 && i3 == text.end()) {
                        cout << "i3 end\n";
                    }

                } else {
                    // Advance iterators if no merge occurred
                    i0 = i1;
                    i1 = std::next(i1);
                    if(i2 != text.end()) {
                        i2 = std::next(i2);
                        if(i3 != text.end()) {
                            i3 = std::next(i3);
                        }
                    }
                }
            }
            if(verbose >= 2) {
                cout << "after merge\n";
                for(auto c: text) {
                    cout << c << " ";
                }
                cout << "\n";
            }
        }

        // Merges a specific pair across all forward_lists in chunks
        void merge_chunks(vector<std::forward_list<int>> &chunks, pair<int,int> mp, int idx, int insert_order, PairCount &freqs) {
            for(auto &chunk: chunks) {
                merge(chunk, mp, idx, insert_order, freqs);
            }
        }

        // Builds the vocabulary based on merges
        void build_vocab(bool verbose) {
            assert(vocab.size() == 256); // Initial vocab must contain 256 byte tokens
            int idx = 256;
            for(auto mio: merges) {
                vector<int> appended{vocab[std::get<0>(mio)]}; // Copy first part of the merge
                appended.insert(appended.end(),vocab[std::get<1>(mio)].begin(), vocab[std::get<1>(mio)].end()); // Append second part
                vocab.push_back(appended);
                idx++;
            }
            if(verbose) {
                cout << "Loaded vocab with " << merges.size() << " merges, vocab size is " << vocab.size() << "\n";
            }
            // TODO: Special token handling would go here
        }

        // Internal encoding function that applies merges recursively
        // NOTE: This version was directly called in the original encode,
        // but now `encode` will first split with regex, then use this.
        vector<int> internal_internal_encode(const vector<int> &text) {
            if (text.size() < 2) { // Nothing to merge if less than 2 elements
                return text;
            }

            vector<int> out;
            out.reserve(text.size()); // Pre-allocate for efficiency
            int i = 0;
            int merge_count = 0;
            auto len = text.size();

            while(i < len) {
                bool merged_this_iter = false;
                if(i < len - 1) { // Check if there's a pair
                    auto current_pair = make_pair(text[i], text[i + 1]);
                    auto it = merges_lookup.find(current_pair);
                    if(it != merges_lookup.end()) {
                        if (true) { // Set to `verbose` if desired
                            cout << "found pair " << std::get<0>(current_pair) << ", " << std::get<1>(current_pair) << " replace with " << it->second << "\n";
                        }
                        out.push_back(it->second); // Add the new merged token
                        i += 2; // Skip the two merged elements
                        merge_count++;
                        merged_this_iter = true;
                    }
                }

                if (!merged_this_iter) {
                    if (true) { // Set to `verbose` if desired
                        cout << "no pair " << text[i] << " replace with " << text[i] << "\n";
                    }
                    out.push_back(text[i]); // Add the single element
                    i++;
                }
            }

            // Recursively apply merges until no more merges are found
            if(merge_count == 0) {
                return out;
            } else {
                return internal_internal_encode(out);
            }
        }

        // Applies internal encoding to a vector of chunks
        vector<vector<int>> internal_encode(const vector<vector<int>> &chunks) {
            vector<vector<int>> encoded_chunks;
            encoded_chunks.reserve(chunks.size()); // Pre-allocate
            for(const auto &chunk: chunks) {
                encoded_chunks.push_back(internal_internal_encode(chunk));
            }
            return encoded_chunks;
        }

    public:
        // Default constructor
        Tokenizer() : merges_lookup(bucket_size, pair_int_int_hash),
                      compiled_pattern_pcre2(NULL),
                      match_data_pcre2(NULL) {
            // Initialize PCRE2 context objects once
            general_context_pcre2 = pcre2_general_context_create_8(NULL, NULL, NULL);
            compile_context_pcre2 = pcre2_compile_context_create_8(general_context_pcre2);
            match_context_pcre2 = pcre2_match_context_create_8(general_context_pcre2);
        };

        // Constructor with a specific pattern
        Tokenizer(const string &pattern) : merges_lookup(bucket_size, pair_int_int_hash),
                                            pattern(pattern),
                                            compiled_pattern_pcre2(NULL),
                                            match_data_pcre2(NULL) {
            // Initialize PCRE2 context objects once
            general_context_pcre2 = pcre2_general_context_create_8(NULL, NULL, NULL);
            compile_context_pcre2 = pcre2_compile_context_create_8(general_context_pcre2);
            match_context_pcre2 = pcre2_match_context_create_8(general_context_pcre2);

            if (pattern.length() > 0) {
                PCRE2_SPTR pcre2_pattern_str = reinterpret_cast<PCRE2_SPTR>(this->pattern.c_str());
                PCRE2_SIZE erroroffset;
                int errorcode;

                // Flags for PCRE2: PCRE2_UTF for UTF-8 and PCRE2_UCP for Unicode character properties
                // Based on the GPT patterns, these are essential for correct matching.
                uint32_t options = PCRE2_UTF | PCRE2_UCP;

                // If your patterns (e.g., GPT4_SPLIT_PATTERN_OLD) use `(?i:)`, add PCRE2_CASELESS.
                // If they use `(?s)` (dot matches newline), add PCRE2_DOTALL.
                // If they use `(?m)` (multiline anchors), add PCRE2_MULTILINE.
                // For the current `GPT4_SPLIT_PATTERN = "\\p{L}"`, UTF and UCP are sufficient.
                if (this->pattern.find("(?i:") != std::string::npos) {
                    options |= PCRE2_CASELESS;
                }
                // Add more such checks if other options might be dynamically included in `pattern`

                compiled_pattern_pcre2 = pcre2_compile_8(
                    pcre2_pattern_str,           // Pointer to the pattern string
                    (PCRE2_SIZE)this->pattern.length(), // Length of the pattern
                    options,                     // Compile options
                    &errorcode,                  // Where to store error code
                    &erroroffset,                // Where to store error offset
                    compile_context_pcre2        // Compile context
                );

                if (compiled_pattern_pcre2 == NULL) {
                    PCRE2_UCHAR buffer[256];
                    pcre2_get_error_message_8(errorcode, buffer, sizeof(buffer));
                    throw std::runtime_error("PCRE2 pattern compilation failed: " + std::string(reinterpret_cast<char*>(buffer)));
                }

                // Create match data block for the compiled pattern
                match_data_pcre2 = pcre2_match_data_create_from_pattern_8(compiled_pattern_pcre2, general_context_pcre2);
                if (match_data_pcre2 == NULL) {
                    throw std::runtime_error("PCRE2 match data creation failed.");
                }
            }
        };

        // Destructor to free PCRE2 allocated memory
        ~Tokenizer() {
            if (compiled_pattern_pcre2 != NULL) {
                pcre2_code_free_8(compiled_pattern_pcre2);
            }
            if (match_data_pcre2 != NULL) {
                pcre2_match_data_free_8(match_data_pcre2);
            }
            if (compile_context_pcre2 != NULL) {
                pcre2_compile_context_free_8(compile_context_pcre2);
            }
            if (match_context_pcre2 != NULL) {
                pcre2_match_context_free_8(match_context_pcre2);
            }
            if (general_context_pcre2 != NULL) {
                pcre2_general_context_free_8(general_context_pcre2);
            }
        }

        // Trains the tokenizer given input text and desired vocabulary size
        void train(const string &text, const int vocab_size, const bool verbose) {
            assert(vocab_size >= 256); // Must have at least initial byte tokens

            merges.clear();
            merges.reserve(vocab_size - 256); // Pre-allocate space for merges

            vector<vector<int>> chunks;

            if (compiled_pattern_pcre2 != NULL) {
                // If a regex pattern is provided, split the text into chunks using PCRE2
                PCRE2_SPTR subject = reinterpret_cast<PCRE2_SPTR>(text.c_str());
                PCRE2_SIZE subject_length = text.length();
                PCRE2_SIZE offset = 0; // Current offset in the subject string
                int rc; // Return code from pcre2_match

                while ((rc = pcre2_match_8(
                            compiled_pattern_pcre2, // Compiled PCRE2 pattern
                            subject,                // Subject string
                            subject_length,         // Length of subject string
                            offset,                 // Start offset for matching
                            0,                      // Options (e.g., PCRE2_NOTEMPTY)
                            match_data_pcre2,       // Match data block
                            match_context_pcre2     // Match context
                        )) >= 0) { // If a match was found (rc >= 0 indicates number of matched substrings)

                    // Get the ovector, which contains start and end offsets of matched substrings
                    PCRE2_SIZE *ovector = pcre2_get_ovector_pointer_8(match_data_pcre2);

                    // Extract the matched substring using the offsets of the primary match (ovector[0] and ovector[1])
                    PCRE2_SIZE matched_len = ovector[1] - ovector[0];
                    std::string matched_text(reinterpret_cast<const char*>(subject + ovector[0]), matched_len);

                    std::cout << "Matched text: " << matched_text << "\n"; // Debugging output

                    // Convert the matched substring to your internal vector<int> representation
                    chunks.push_back(text_to_vector(matched_text));

                    // Advance the offset to the end of the current match to find the next one
                    offset = ovector[1];

                    // Important: Prevent infinite loop if an empty match is found at the end of the subject
                    if (offset == subject_length) {
                        break;
                    }
                }

                // Handle PCRE2 matching errors (excluding PCRE2_ERROR_NOMATCH, which is expected)
                if (rc < 0 && rc != PCRE2_ERROR_NOMATCH) {
                    PCRE2_UCHAR buffer[256];
                    pcre2_get_error_message_8(rc, buffer, sizeof(buffer));
                    throw std::runtime_error("PCRE2 matching failed during training: " + std::string(reinterpret_cast<char*>(buffer)));
                }
            } else {
                // If no split pattern, treat the whole text as a single chunk
                chunks.push_back(text_to_vector(text));
            }

            // Continue with BPE algorithm
            auto flists = create_lists(chunks);
            auto freqs = calculate_freqs(flists);

            for(int i = 256; i < vocab_size; i++) {
                const auto& index_by_count = freqs.get_index_by_count();
                if(!index_by_count.empty()) {
                    auto max = *index_by_count.begin(); // Get the most frequent pair
                    auto [p1,p2] = max.pair;

                    if(verbose) {
                        cout << "merge pair " << p1 << ", " << p2 << " with new token " << i << " count " << max.count << "\n";
                    }

                    merges.push_back(max.pair); // Store the merged pair
                    merges_lookup[max.pair] = i; // Add to lookup table
                    merge_chunks(flists, max.pair, i, max.count, freqs); // Apply merge to all chunks
                } else {
                    break; // No more pairs to merge
                }
            }

            if(verbose) {
                int size = 0;
                for(auto &fl: flists) {
                    size += std::distance(fl.begin(), fl.end());
                }
                cout << "Length of training text " << text.length() << ". After merges " << size << ".\n";
            }
        };

        // Encodes input text into a sequence of tokens
        vector<int> encode(const string &text, const bool verbose) {
            vector<vector<int>> text_chunks;

            if (compiled_pattern_pcre2 != NULL) {
                // Split the text into chunks using PCRE2, similar to training
                PCRE2_SPTR subject = reinterpret_cast<PCRE2_SPTR>(text.c_str());
                PCRE2_SIZE subject_length = text.length();
                PCRE2_SIZE offset = 0;
                int rc;

                while ((rc = pcre2_match_8(
                            compiled_pattern_pcre2,
                            subject,
                            subject_length,
                            offset,
                            0,
                            match_data_pcre2,
                            match_context_pcre2
                        )) >= 0) {
                    PCRE2_SIZE *ovector = pcre2_get_ovector_pointer_8(match_data_pcre2);
                    PCRE2_SIZE matched_len = ovector[1] - ovector[0];
                    std::string matched_text(reinterpret_cast<const char*>(subject + ovector[0]), matched_len);

                    text_chunks.push_back(text_to_vector(matched_text));

                    offset = ovector[1];
                    if (offset == subject_length) {
                        break;
                    }
                }
                if (rc < 0 && rc != PCRE2_ERROR_NOMATCH) {
                    PCRE2_UCHAR buffer[256];
                    pcre2_get_error_message_8(rc, buffer, sizeof(buffer));
                    throw std::runtime_error("PCRE2 matching failed in encode: " + std::string(reinterpret_cast<char*>(buffer)));
                }
            } else {
                // If no split pattern, treat the whole text as a single chunk
                text_chunks.push_back(text_to_vector(text));
            }

            // Apply merges to each chunk
            auto encoded_chunks = internal_encode(text_chunks);

            // Flatten the encoded chunks into a single token vector
            vector<int> out;
            for(const auto &chunk: encoded_chunks) {
                out.insert(out.end(), chunk.begin(), chunk.end());
            }

            if(verbose) {
                cout << "Encoded input text (length " << text.length() << ") to " << out.size() << " tokens\n";
            }
            return out;
        };

        // Decodes a sequence of tokens back into a string
        string decode(const vector<int> &tokens, const bool verbose) {
            if(verbose) {
                cout << "Decoding " << tokens.size() << " tokens\n";
            }
            string text = "";
            for(int tkn : tokens) {
                // Ensure token is within valid vocabulary range
                if (tkn < 0 || tkn >= vocab.size()) {
                    std::cerr << "Warning: Attempted to decode invalid token ID: " << tkn << "\n";
                    continue; // Skip invalid tokens
                }

                // cout << "token " << tkn << "\n"; // Debugging
                for(int c_int : vocab[tkn]) {
                    // Convert integer back to character, ensuring it's in byte range
                    text.push_back(static_cast<char>(c_int));
                }
            }
            return text;
        };

        // Loads tokenizer model from a file
        bool load(const path &path, const bool verbose) {
            std::ifstream input_file(path, ios::in);
            if(input_file.is_open()) {
                string version;
                std::getline(input_file, version);
                if(version != "minbpe v1") {
                    std::cerr << "Unexpected version: " << version << "\n";
                    return false;
                }

                // Clear existing merges and initialize vocab for loading
                merges_lookup.clear();
                merges.clear();
                initialize_vocab();

                // Read pattern string and recompile PCRE2 pattern
                std::getline(input_file, pattern);

                // Free old PCRE2 pattern and match data if they exist
                if (compiled_pattern_pcre2 != NULL) {
                    pcre2_code_free_8(compiled_pattern_pcre2);
                    compiled_pattern_pcre2 = NULL;
                }
                if (match_data_pcre2 != NULL) {
                    pcre2_match_data_free_8(match_data_pcre2);
                    match_data_pcre2 = NULL;
                }

                if (this->pattern.length() > 0) {
                    PCRE2_SPTR pcre2_pattern_str = reinterpret_cast<PCRE2_SPTR>(this->pattern.c_str());
                    PCRE2_SIZE erroroffset_load;
                    int errorcode_load;
                    uint32_t options = PCRE2_UTF | PCRE2_UCP; // Assuming Unicode features are always desired

                    // Add dynamic options based on pattern content (e.g., case insensitivity)
                    if (this->pattern.find("(?i:") != std::string::npos) {
                        options |= PCRE2_CASELESS;
                    }

                    compiled_pattern_pcre2 = pcre2_compile_8(
                        pcre2_pattern_str,
                        (PCRE2_SIZE)this->pattern.length(),
                        options,
                        &errorcode_load,
                        &erroroffset_load,
                        compile_context_pcre2
                    );

                    if (compiled_pattern_pcre2 == NULL) {
                        PCRE2_UCHAR buffer[256];
                        pcre2_get_error_message_8(errorcode_load, buffer, sizeof(buffer));
                        std::cerr << "PCRE2 compilation failed on load: " << reinterpret_cast<char*>(buffer) << "\n";
                        return false;
                    }

                    match_data_pcre2 = pcre2_match_data_create_from_pattern_8(compiled_pattern_pcre2, general_context_pcre2);
                    if (match_data_pcre2 == NULL) {
                        std::cerr << "PCRE2 match data creation failed on load.\n";
                        return false;
                    }
                }

                // Read special token count (currently always 0)
                int num_special;
                input_file >> num_special;
                // For now, skip special token data if any, as it's not implemented
                for(int i = 0; i < num_special; i++) {
                    // TODO: read special tokens if implemented
                }

                // Read merges
                int idx1, idx2;
                int current_token_idx = 256; // Merged tokens start from 256
                while(input_file >> idx1 >> idx2) {
                    merges.push_back(make_pair(idx1, idx2));
                    merges_lookup[make_pair(idx1, idx2)] = current_token_idx;
                    current_token_idx++;
                }

                if(verbose) {
                    cout << "Read input model from " << path << "\n";
                }
                build_vocab(verbose); // Rebuild vocab based on loaded merges
                input_file.close();
                return true;
            } else {
                std::cerr << "Failed to open file for loading: " << path << "\n";
                return false;
            }
        };

        // Saves tokenizer model to a file
        bool save(const path &path, bool write_vocab) {
            assert(merges.size() > 0); // Must have trained merges to save

            std::ofstream output_file(path, ios::out);
            if (output_file.is_open()) {
                cout << "Writing model...\n";
                output_file << "minbpe v1" << std::endl; // Version
                output_file << pattern << std::endl;     // Regex pattern string
                output_file << 0 << std::endl;           // Special token count (currently 0)
                // Write merges
                for(const auto &m: merges) {
                    output_file << std::get<0>(m) << ' ' << std::get<1>(m) << "\n";
                }
                output_file.close();

                if (write_vocab) {
                    initialize_vocab(); // Ensure vocab is fresh
                    build_vocab(false); // Build vocab without verbose output for saving

                    // Create the .vocab file path
                    std::string vocab_path_str = path.string() + ".vocab";
                    std::ofstream vocab_file(vocab_path_str, std::ios::out);

                    if (!vocab_file.is_open()) {
                        std::cerr << "Failed to open .vocab file for writing: " << vocab_path_str << std::endl;
                        return false;
                    }

                    // Write the tokens to the .vocab file
                    int token_id = 0; // Token IDs start from 0 for byte tokens
                    for (const auto &v : vocab) {
                        vocab_file << std::setw(6) << std::left << token_id << ": \""; // Use left alignment for token ID
                        for (int c_int : v) {
                            if (c_int >= 32 && c_int <= 126) { // Printable ASCII characters
                                vocab_file << (char)c_int;
                            } else {
                                vocab_file << "�"; // Replacement character for non-printable/non-ASCII
                            }
                        }
                        vocab_file << "\"\n";
                        token_id++;
                    }
                    vocab_file.close();
                }
                cout << "Complete.\n";
                return true;
            } else {
                std::cerr << "Unable to open file for saving: " << path << std::endl;
                return false;
            }
        }
    };
}
#endif

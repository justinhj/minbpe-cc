#ifndef MINBPE_TOKENIZER_HPP
#define MINBPE_TOKENIZER_HPP

#include <algorithm>
#include <unordered_map>
#include <string>
#include <functional>
#include <forward_list>
#include <memory>
#include <vector>
#include <cassert>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <utility>
#include <optional> // Using std::optional
#include <stdexcept> // For std::runtime_error
#include <cstdint>

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
    using Token = uint32_t; // Here it is safe to change to uint16_t and other types as needed, 
                            // but be sure to not have any special tokens that exceed the range.
    using TokenPair = std::pair<Token, Token>;

    // Hash function for std::pair<int,int>
    inline std::function<std::size_t(const TokenPair&)> pair_token_hash =
        [](const TokenPair& k) -> std::size_t {
            std::size_t seed = 0;
            std::hash<Token> hasher;
            seed ^= hasher(std::get<0>(k)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= hasher(std::get<1>(k)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            return seed;
    };

    class Tokenizer {
    public:
        enum CONFLICT_RESOLUTION {
            FIRST,
            LEXICAL
        };
    public:
        inline const static std::string GPT2_SPLIT_PATTERN = "'(?:[sdmt]|ll|ve|re)| ?\\p{L}+| ?\\p{N}+| ?[^\\s\\p{L}\\p{N}]+|\\s+(?!\\S)|\\s+";
        inline const static std::string GPT4_SPLIT_PATTERN = "'(?i:[sdmt]|ll|ve|re)|[^\\r\\n\\p{L}\\p{N}]?+\\p{L}+|\\p{N}{1,3}| ?[^\\s\\p{L}\\p{N}]++[\\r\\n]*|\\s*[\\r\\n]|\\s+(?!\\S)|\\s+";
    protected:
        static const auto bucket_size = 10;

        std::unordered_map<std::string, Token> special_tokens;
        std::unordered_map<Token, std::string> special_tokens_reverse_lookup;

        pcre2_code_8* compiled_pattern_pcre2;     // Compiled PCRE2 pattern
        pcre2_match_data_8* match_data_pcre2;     // Match data block for results

        pcre2_general_context_8* general_context_pcre2;
        pcre2_compile_context_8* compile_context_pcre2;
        pcre2_match_context_8* match_context_pcre2;

        unordered_map<TokenPair, Token, decltype(pair_token_hash)> merges_lookup;
        vector<TokenPair> merges;
        vector<vector<Token>> vocab;
        string pattern; // The string representation of the regex pattern

        // Helper to convert char to int, handling negative char values
        Token char_to_token(char c) {
            return c < 0 ? c + 256 : c;
        }

        // Same as above but uses string views for performance
        std::vector<Token> text_to_vector(std::string_view text) {
            if (!text.empty() && text[0] == '\0') {
                try {
                    int id = std::stoi(std::string(text.substr(1)));
                    return std::vector<Token>{static_cast<Token>(id)};
                } catch (...) {
                    // Fallback: treat as normal text if parsing fails
                }
            }
            std::vector<Token> text_converted;
            text_converted.reserve(text.length()); // Reserve space to prevent reallocations
            for (char c : text) {
                text_converted.push_back(static_cast<Token>(char_to_token(c)));
            }
            return text_converted;
        }

        // Initializes the vocabulary with 256 byte tokens
        void initialize_vocab() {
            vocab.clear();
            vocab.reserve(256); // Reserve space for initial bytes
            for(int i = 0; i < 256; i++) {
                vector<Token> s;
                s.push_back(i);
                vocab.push_back(s);
            }
        }

        // Converts a vector of vector of ints (chunks) to a vector of forward_list of ints
        auto create_lists(const vector<vector<Token>> &chunks) {
            vector<std::forward_list<Token>> flists;
            flists.reserve(chunks.size()); // Reserve space
            for(const auto &chunk: chunks) {
                std::forward_list<Token> flist;
                // Copy in reverse to use front_inserter efficiently
                std::copy(chunk.rbegin(), chunk.rend(), std::front_inserter(flist));
                flists.push_back(flist);
            }
            return flists;
        }

        // Calculates frequencies of adjacent pairs in the chunks
        std::unique_ptr<PairCount<Token>> calculate_freqs(const vector<std::forward_list<Token>> &chunks, CONFLICT_RESOLUTION conflict_resolution) {
          std::unique_ptr<PairCount<Token>> freqs;
          if (conflict_resolution == CONFLICT_RESOLUTION::FIRST) {
              freqs = std::make_unique<PairCountInsertOrder<Token>>();
          } else { // LEXICAL
              freqs = std::make_unique<PairCountLexicalOrder<Token>>();
          }

            for(const auto &chunk: chunks) {
                auto p1 = chunk.begin();
                auto p2 = std::next(p1);
                while(p1 != chunk.end() && p2 != chunk.end()) {
                    auto p = make_pair(*p1, *p2);
                    freqs->create_or_modify_pair(*p1, *p2, 1);
                    ++p1;
                    ++p2;
                }
            }
            return freqs;
        }

        // Calculate frequencies and first occurrence of all symbol pairs in the corpus
        // This version supports first-occurrence tie-breaking
        void calculate_freqs(const std::vector<std::vector<Token>>& corpus, PairCount<Token>& pair_count) const {
            for (const auto& word : corpus) {
                if (word.size() < 2) continue;
                for (size_t i = 0; i + 1 < word.size(); ++i) {
                    Token a = word[i];
                    Token b = word[i + 1];
                    pair_count.create_or_modify_pair(a, b, 1);
                }
            }
        }

        // Merges a specific pair within a single forward_list then completly update frequencies
        void merge(std::forward_list<Token> &text, TokenPair mp, Token new_token, PairCount<Token> *freqs) {
            auto verbose = 0; // Control verbosity for debugging
            if(verbose >= 2) {
                cout << "before merge\n";
                for(auto c: text) {
                    cout << c << " ";
                }
                cout << "\n";
            }

            auto [p1_val, p2_val] = mp; // Deconstruct the pair
            auto i1 = text.begin();
            auto i2 = std::next(i1);

            while(i1 != text.end() && i2 != text.end()) {
                if(*i1 == p1_val && *i2 == p2_val) {
                    if(verbose >= 1) {
                        cout << "found pair " << p1_val << ", " << p2_val << " replace with " << new_token << "\n";
                    }

                    *i1 = new_token; // Replace the first element of the pair with the new token
                    i2 = text.erase_after(i1); // Erase the second element


                } else {
                    // Advance iterators if no merge occurred
                    i1 = i2;
                    i2 = std::next(i2);
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

        // Merges a specific pair within a single forward_list, updating frequencies incrementally
        void merge_incremental(std::forward_list<Token> &text, TokenPair mp, Token new_token, PairCount<Token> *freqs) {
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
                if(false) { // verbose >= 1) {
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
                    auto f = freqs->get_pair(mp);
                    if(f.has_value()) {
                        if(verbose >= 1) {
                            cout << "decrement replaced pair " << mp.first << ", " << mp.second << "\n";
                        }
                        freqs->create_or_modify_pair(mp.first, mp.second, -1);
                    }

                    if(i0 != text.before_begin()) { // Check if i0 is a valid element (not before_begin())
                        auto prev_pair = make_pair(*i0, p1_val);
                        auto prev = freqs->get_pair(prev_pair);
                        if(prev.has_value()) {
                            if(verbose >= 1) {
                                cout << "decrement previous pair " << *i0 << ", " << p1_val << "\n";
                            }
                            freqs->create_or_modify_pair(*i0, p1_val, -1);
                        }
                        if(verbose >= 1) {
                            cout << "increment new previous pair " << *i0 << ", " << new_token << "\n";
                        }
                        freqs->create_or_modify_pair(*i0, new_token, 1);
                    }

                    if(i2 != text.end()) { // Check if i2 is a valid element (not end())
                        auto next_pair = make_pair(p2_val, *i2); // Original p2_val, not new_token
                        auto next = freqs->get_pair(next_pair);
                        if(next.has_value()) {
                            if(verbose >= 1) {
                                cout << "decrement next pair " << p2_val << ", " << *i2 << "\n";
                            }
                            freqs->create_or_modify_pair(p2_val, *i2, -1);
                        } else {
                            if(verbose >= 1) {
                                cout << "next pair not found " << p2_val << ", " << *i2 << "\n";
                            }
                        }
                        if(verbose >= 1) {
                            cout << "increment new next pair " << new_token << ", " << *i2 << "\n";
                        }
                        freqs->create_or_modify_pair(new_token, *i2, 1);
                    }

                    // Iterators are already adjusted by erase_after,
                    // just make sure i3 points correctly if it's not end()
                    if (i2 != text.end()) {
                        i3 = std::next(i2);
                    } else {
                        i3 = text.end(); // If i2 is now end, i3 is also end
                    }
                } else {
                    // Advance iterators if no merge occurred
                    i0 = i1;
                    i1 = i2;
                    i2 = i3;
                    if (i3 != text.end()) {
                        i3 = std::next(i3);
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
        void merge_chunks(vector<std::forward_list<Token>> &chunks, TokenPair mp, Token idx, PairCount<Token> *freqs,
              CONFLICT_RESOLUTION conflict_resolution) {
            for(auto &chunk: chunks) {
              if (conflict_resolution == CONFLICT_RESOLUTION::FIRST) {
                  merge(chunk, mp, idx, freqs);
              } else if(conflict_resolution == CONFLICT_RESOLUTION::LEXICAL) {
                  merge_incremental(chunk, mp, idx, freqs);
              } else {
                  throw std::runtime_error("Unknown conflict resolution strategy");
              }
            }
        }

        // Internal encoding function that applies merges recursively
        // NOTE: This version was directly called in the original encode,
        // but now `encode` will first split with regex, then use this.
        vector<Token> internal_internal_encode(const vector<Token> &text) {
            if (text.size() < 2) { // Nothing to merge if less than 2 elements
                return text;
            }

            vector<Token> out;
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
                        if (false) { // Set to `verbose` if desired
                            cout << "found pair " << std::get<0>(current_pair) << ", " << std::get<1>(current_pair) << " replace with " << it->second << "\n";
                        }
                        out.push_back(it->second); // Add the new merged token
                        i += 2; // Skip the two merged elements
                        merge_count++;
                        merged_this_iter = true;
                    }
                }

                if (!merged_this_iter) {
                    if (false) { // Set to `verbose` if desired
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
        vector<vector<Token>> internal_encode(const vector<vector<Token>> &chunks) {
            vector<vector<Token>> encoded_chunks;
            encoded_chunks.reserve(chunks.size()); // Pre-allocate
            for(const auto &chunk: chunks) {
                encoded_chunks.push_back(internal_internal_encode(chunk));
            }
            return encoded_chunks;
        }

    public:
        // Default constructor
        Tokenizer() : merges_lookup(bucket_size, pair_token_hash),
                      compiled_pattern_pcre2(NULL),
                      match_data_pcre2(NULL) {
            // Initialize PCRE2 context objects once
            general_context_pcre2 = pcre2_general_context_create_8(NULL, NULL, NULL);
            compile_context_pcre2 = pcre2_compile_context_create_8(general_context_pcre2);
            match_context_pcre2 = pcre2_match_context_create_8(general_context_pcre2);
        };

        // Constructor with a specific pattern
        Tokenizer(const string &pattern) : merges_lookup(bucket_size, pair_token_hash),
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

                // --- Add this for JIT compilation ---
                int jit_errorcode;
                if (pcre2_jit_compile_8(compiled_pattern_pcre2, PCRE2_JIT_COMPLETE) < 0) {
                    // JIT compilation failed or is not supported by PCRE2 build.
                    // It's okay, PCRE2 will fall back to its interpretive engine,
                    // but log a warning if you want.
                    PCRE2_UCHAR buffer[256];
                    pcre2_get_error_message_8(jit_errorcode, buffer, sizeof(buffer));
                    std::cerr << "Warning: PCRE2 JIT compilation failed: " << reinterpret_cast<char*>(buffer) << "\n";
                }
                // --- End JIT addition ---

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

        // Sets the special token map from a single string representing the file contents
        // in the form: 
        //   token1 20000
        //   token2 20001
        void set_special_tokens_from_file(const std::string& input_string) {
          special_tokens.clear();
          special_tokens_reverse_lookup.clear();
          std::istringstream iss(input_string);
          std::string key;
          Token value;
          while (iss >> key >> value) {
              special_tokens[key] = value;
              special_tokens_reverse_lookup[value] = key;
          }
        }

        // Trains the tokenizer given input text and desired vocabulary size
        void train(const string &text, const int vocab_size, const CONFLICT_RESOLUTION conflict_resolution, 
              const bool verbose) {

            assert(vocab_size >= 256); // Must have at least initial byte tokens

            merges.clear();
            merges.reserve(vocab_size - 256); // Pre-allocate space for merges
            initialize_vocab();

            vector<vector<Token>> chunks;

            if (compiled_pattern_pcre2 != NULL) {
              PCRE2_SPTR subject = reinterpret_cast<PCRE2_SPTR>(text.data());
              PCRE2_SIZE subject_length = text.length();
              PCRE2_SIZE offset = 0;

              int rc;
              while (true) {
                  rc = pcre2_match(
                      compiled_pattern_pcre2,
                      subject,
                      subject_length,
                      offset,
                      PCRE2_NO_UTF_CHECK,  // Optional for performance if you're sure input is valid
                      match_data_pcre2,
                      match_context_pcre2
                  );

                  if (rc < 0) {
                      if (rc == PCRE2_ERROR_NOMATCH) break;
                      PCRE2_UCHAR buffer[256];
                      pcre2_get_error_message(rc, buffer, sizeof(buffer));
                      throw std::runtime_error("PCRE2 match error: " + std::string(reinterpret_cast<char*>(buffer)));
                  }

                  PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(match_data_pcre2);
                  PCRE2_SIZE start = ovector[0];
                  PCRE2_SIZE end = ovector[1];

                  // Avoid empty match loops
                  if (start == end) {
                      if (offset >= subject_length) break;
                      offset++;
                      continue;
                  }

                  // Use a string_view to avoid allocation
                  std::string_view matched_view(reinterpret_cast<const char*>(subject + start), end - start);
                  chunks.push_back(text_to_vector(matched_view));  // overload text_to_vector for string_view?

                  offset = end;
              }
            } else {
                // If no split pattern, treat the whole text as a single chunk
                chunks.push_back(text_to_vector(text));
            }
            
            if (verbose) {
                cout << "Split input text into " << chunks.size() << " chunks\n";
            }

            // Continue with BPE algorithm
            auto flists = create_lists(chunks);
            auto freqs = calculate_freqs(flists, conflict_resolution);

            int total_merges = vocab_size - 256;
            int last_percent = -1;
            
            for(Token i = 256; i < vocab_size; i++) {
                auto best = freqs->get_top_pair_count();
                if(best.has_value()) {
                    auto max_pair = *best;
                    auto [p1, p2] = max_pair;
                    vector<Token> appended{vocab[p1]};
                    appended.insert(appended.end(), vocab[p2].begin(), vocab[p2].end());
                    vocab.push_back(appended);
                    if(verbose) {
                        auto freq = freqs->get_pair(max_pair);
                        string new_vocab_str;
                        new_vocab_str.reserve(appended.size());
                        for(auto c: appended) {
                            if(c >= 32 && c < 127) { // Printable ASCII range
                                new_vocab_str += static_cast<char>(c);
                            } else {
                                new_vocab_str += ' ';
                            }
                        }
                        cout << "merge " << (i - 256) + 1 << "/" << total_merges << ": (" <<  p1 << ", " << p2 << ") -> " << i << " (b'" << new_vocab_str << "') had " << (freq.has_value() ? std::to_string(freq.value()) : "0") << " occurrences\n";
                    }
                    merges.push_back(max_pair);
                    merges_lookup[max_pair] = i;
                    merge_chunks(flists, max_pair, i, freqs.get(), conflict_resolution);
                    if(conflict_resolution == CONFLICT_RESOLUTION::FIRST) {
                      // The lexical conflict resolution primarily gets its speed up from
                      // not having to recalculate frequencies after each merge.
                      freqs = std::move(calculate_freqs(flists, conflict_resolution));
                    }
                } else {
                    break;
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

        // Splits input text into a vector of strings, separating regular text and special tokens.
        // Each special token occurrence is replaced with a string like "\0<id>" (null char + token id as string).
        // If no special tokens are found, returns a single string in the vector.
        // Example: "hello <|endoftext|> world" => ["hello ", "\0100257", " world"]
        // TODO this is a naive implementation, it may be more efficient to use a regex or other method
        std::vector<std::string> split_on_special(const std::string& text) {
            std::vector<std::string> result;
            if (special_tokens.empty()) {
                result.push_back(text);
                return result;
            }
            size_t pos = 0;
            size_t last = 0;
            while (pos < text.size()) {
                size_t found_pos = std::string::npos;
                std::string found_token;
                Token found_id = 0;
                // Find the next special token occurrence
                for (const auto& kv : special_tokens) {
                    const std::string& token = kv.first;
                    size_t p = text.find(token, pos);
                    if (p != std::string::npos && (found_pos == std::string::npos || p < found_pos)) {
                        found_pos = p;
                        found_token = token;
                        found_id = kv.second;
                    }
                }
                if (found_pos == std::string::npos) {
                    break;
                }
                // Add text before the special token
                if (found_pos > last) {
                    result.push_back(text.substr(last, found_pos - last));
                }
                // Add the special token marker
                std::string marker(1, '\0');
                marker += std::to_string(found_id);
                result.push_back(marker);
                pos = found_pos + found_token.size();
                last = pos;
            }
            // Add any remaining text
            if (last < text.size()) {
                result.push_back(text.substr(last));
            }
            // If no special tokens were found, return the whole string
            if (result.empty()) {
                result.push_back(text);
            }
            return result;
        }

        // Encodes input text into a sequence of tokens
        vector<Token> encode(const string &text, const bool verbose) {
            auto split_text = split_on_special(text);
            if (verbose) {
                cout << "Splitting input text into " << split_text.size() << " parts\n";
                for(const auto &part : split_text) {
                    auto is_special = part.size() > 0 && part[0] == '\0';
                    cout << "Part: \"" << part << "\" special: " << is_special << "\n";
                }
            }

            vector<vector<Token>> text_chunks;
            if (compiled_pattern_pcre2 != NULL) {
                // For each split part, apply regex splitting if not a special token
                for (const auto& part : split_text) {
                    if (part.size() > 0 && part[0] == '\0') {
                        // Special token, treat as a chunk
                        text_chunks.push_back(text_to_vector(part));
                        continue;
                    }
                    PCRE2_SPTR subject = reinterpret_cast<PCRE2_SPTR>(part.c_str());
                    PCRE2_SIZE subject_length = part.length();
                    PCRE2_SIZE offset = 0;
                    int rc;
                    while(true) {
                        rc = pcre2_match_8(
                                compiled_pattern_pcre2,
                                subject,
                                subject_length,
                                offset,
                                PCRE2_NO_UTF_CHECK,
                                match_data_pcre2,
                                match_context_pcre2
                            );
                        if (rc < 0) {
                            if (rc == PCRE2_ERROR_NOMATCH) break;
                            PCRE2_UCHAR buffer[256];
                            pcre2_get_error_message(rc, buffer, sizeof(buffer));
                            throw std::runtime_error("PCRE2 match error: " + std::string(reinterpret_cast<char*>(buffer)));
                        }
                        PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(match_data_pcre2);
                        PCRE2_SIZE start = ovector[0];
                        PCRE2_SIZE end = ovector[1];
                        if (start == end) {
                            if (offset >= subject_length) break;
                            offset++;
                            continue;
                        }
                        std::string_view matched_view(reinterpret_cast<const char*>(subject + start), end - start);
                        text_chunks.push_back(text_to_vector(matched_view));
                        offset = end;
                    }
                }
            } else {
                // No regex: just convert each split chunk
                for (const auto& part : split_text) {
                    text_chunks.push_back(text_to_vector(part));
                }
            }
            // Apply merges to each chunk
            auto encoded_chunks = internal_encode(text_chunks);
            // Flatten the encoded chunks into a single token vector
            vector<Token> out;
            for(const auto &chunk: encoded_chunks) {
                out.insert(out.end(), chunk.begin(), chunk.end());
            }
            if(verbose) {
                cout << "Encoded input text (length " << text.length() << ") to " << out.size() << " tokens\n";
            }
            return out;
        };

        // Decodes a sequence of tokens back into a string
        string decode(const vector<Token> &tokens, const bool verbose) {
            if(verbose) {
                cout << "Decoding " << tokens.size() << " tokens\n";
            }
            string text = "";
            for(Token tkn : tokens) {

                // Override special tokens with their string representation
                if (special_tokens_reverse_lookup.find(tkn) != special_tokens_reverse_lookup.end()) {
                    text += special_tokens_reverse_lookup[tkn];
                    continue; // Skip further processing for this token
                }

                // Ensure token is within valid vocabulary range
                if (tkn < 0 || tkn >= vocab.size()) {
                    std::cerr << "Warning: Attempted to decode invalid token ID: " << tkn << "\n";
                    continue; // Skip invalid tokens
                }

                // cout << "token " << tkn << "\n"; // Debugging
                for(Token c_int : vocab[tkn]) {
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

                // Read specials token count
                int num_special;
                input_file >> num_special;
                for(int i = 0; i < num_special; i++) {
                    string token;
                    Token id;
                    input_file >> token >> id;
                    special_tokens[token] = id; // Store special tokens
                    special_tokens_reverse_lookup[id] = token; // Reverse lookup for decoding
                    if(verbose) {
                        cout << "Loaded special token: " << token << " with ID " << id << "\n";
                    }
                }

                // Read merges
                Token idx1, idx2;
                Token current_token_idx = 256; // Merged tokens start from 256
                while(input_file >> idx1 >> idx2) {
                    merges.push_back(make_pair(idx1, idx2));
                    merges_lookup[make_pair(idx1, idx2)] = current_token_idx;
                    current_token_idx++;
                }

                if(verbose) {
                    cout << "Read input model from " << path << "\n";
                }

                // Rebuild vocab from loaded merges
                Token idx = 256;
                for(auto mio: merges) {
                    vector<Token> appended{vocab[std::get<0>(mio)]}; // Copy first part of the merge
                    appended.insert(appended.end(),vocab[std::get<1>(mio)].begin(), vocab[std::get<1>(mio)].end()); // Append second part
                    vocab.push_back(appended);
                    if(verbose) {
                        cout << "vocab[" << idx << "] = ";
                        for(auto c: appended) {
                            if(c >= 32 && c < 127) { // Printable ASCII range
                                cout << static_cast<char>(c);
                            } else {
                                cout << "\\x" << std::hex << c; // Non-printable as hex
                            }
                        }
                        cout << "\n";
                    }
                    idx++;
                }
                if(verbose) {
                    cout << "Loaded vocab with " << merges.size() << " merges, vocab size is " << vocab.size() << "\n";
                }

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
                output_file << special_tokens.size() << std::endl;           // Special token count (currently 0)
                // write the special tokens in the same syntax as the input
                for (const auto &st : special_tokens) {
                    output_file << st.first << ' ' << st.second << std::endl; // Write special tokens
                }
                // Write merges
                for(const auto &m: merges) {
                    output_file << std::get<0>(m) << ' ' << std::get<1>(m) << "\n";
                }
                output_file.close();

                if (write_vocab) {
                    // Create the .vocab file path
                    std::string vocab_path_str = path.string() + ".vocab";
                    std::ofstream vocab_file(vocab_path_str, std::ios::out);

                    if (!vocab_file.is_open()) {
                        std::cerr << "Failed to open .vocab file for writing: " << vocab_path_str << std::endl;
                        return false;
                    }

                    // Write the tokens to the .vocab file
                    Token token_id = 0; // Token IDs start from 0 for byte tokens
                    for (const auto &v : vocab) {
                        vocab_file << std::setw(6) << std::left << token_id << ": \""; // Use left alignment for token ID
                        for (Token c_int : v) {
                            if (c_int >= 32 && c_int <= 126) { // Printable ASCII characters
                                vocab_file << (char)c_int;
                            } else {
                                vocab_file << "\ufffd"; // Replacement character for non-printable/non-ASCII
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

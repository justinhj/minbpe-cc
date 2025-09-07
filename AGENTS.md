# AGENTS.md

## Operating procedures for agents
- Make sure the program builds and tests pass before considering a task complete

## CMake is deprecated in this project
- There is no need to maintain or run any CMake files
- All new development should use Zig build system

## Build Commands
- Build with zig: `zig build`
- Build tests: `zig build test`
- Run single test: `zig build test --test-filter "test_name"`

## Code Style Guidelines
- C++23 standard with modern practices
- PascalCase for classes, snake_case for functions/variables
- Source files: PascalCase, executables/tests: snake-case
- Memory management: prefer smart pointers over new/delete
- Use namespaces to organize code
- Error handling: exceptions for exceptional cases, expected/optional for expected failures

## Dependencies
- Boost (regex)
- PCRE2 (regex)
- CLI11 (command line parsing)
- Catch2 (testing)

## Testing
- Tests use Catch2 framework
- Run all tests: `zig build test`
- Run specific test: `zig build test --test-filter "Tokenizer training"`
- End-to-end tests: `./endtoend-test.sh` and `./endtoend-shakespeare-test.sh`

## Naming Conventions
- Classes and types: PascalCase
- Functions, variables, namespaces: snake_case
- Constants: UPPER_SNAKE_CASE
- Files: PascalCase for headers/source, snake-case for executables

## Formatting
- Indent with 4 spaces
- Braces on same line for functions/classes
- Limit lines to 100 characters
- Use vertical spacing to separate logical sections

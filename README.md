# ufmt - Micro String Formatting Library

[![Build Status](https://github.com/vpiotr/ufmt/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/vpiotr/ufmt/actions)
[![C++11](https://img.shields.io/badge/C%2B%2B-11-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B11)
[![Header Only](https://img.shields.io/badge/Header-Only-green.svg)](https://github.com/vpiotr/ufmt)
[![MIT License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

A lightweight, header-only C++11 micro-library for string template formatting with support for custom formatters, named variables, and both local and shared contexts.

## Features

- **Simple API**: Minimal, intuitive interface for string formatting
- **Positional & Named Placeholders**: Support for `{0}`, `{1}` and `{variable_name}` syntax
- **Printf-style Formatting**: Advanced format specifications like `{0:.2f}`, `{1:08x}`
- **Custom Formatters**: Easy integration of user-defined type formatters
- **Context Management**: Local (RAII), shared (thread-safe), and internal contexts
- **Thread Safety**: Safe concurrent access to shared contexts
- **C++11 Compatible**: No external dependencies, works with older compilers
- **Header-Only**: Just include `ufmt.h` - no linking required
- **Optional ustr.h Integration**: Universal type conversion support

## Quick Start

### Basic Usage

```cpp
#include "ufmt/ufmt.h"

// Simple positional formatting
std::string msg = ufmt::format("User {0} has {1} messages", "Alice", 5);
// Result: "User Alice has 5 messages"

// Format specifications
std::string precise = ufmt::format("Pi = {0:.3f}", 3.14159);
// Result: "Pi = 3.142"

// Multiple types
std::string mixed = ufmt::format("Score: {0}, Active: {1}", 87.5, true);
// Result: "Score: 87.500000, Active: true"
```

### Local Context (RAII)

```cpp
// Create local context with custom formatter
auto ctx = ufmt::create_local_context();
ctx->set_formatter<bool>([](bool b) { return b ? "YES" : "NO"; });

std::string result = ctx->format("Rendering: {0}", true);
// Result: "Rendering: YES"

// Named variables
ctx->set_var("user", "Bob");
ctx->set_var("level", 12);
ctx->set_var("score", 87.5);

std::string status = ctx->format("Player {user} reached level {level} with score {score:.1f}");
// Result: "Player Bob reached level 12 with score 87.5"
```

### Shared Context (Thread-Safe)

```cpp
// Named shared context - thread-safe
auto api_ctx = ufmt::get_shared_context("api");
api_ctx->set_var("endpoint", "/users");
api_ctx->set_var("method", "POST");

// Can be used from multiple threads
std::string log_msg = api_ctx->format("Request {method} {endpoint} completed");
// Result: "Request POST /users completed"

// Create new owning shared context
auto ctx = ufmt::create_shared_context();
ctx->set_var("session", "abc123");
std::string msg = ctx->format("Session: {session}");
// Result: "Session: abc123"
```

### Custom Types

```cpp
struct Point {
    int x, y;
    Point(int x, int y) : x(x), y(y) {}
};

// Extend ufmt::to_string for your type
namespace ufmt {
    template<>
    std::string to_string<Point>(const Point& p) {
        return "(" + std::to_string(p.x) + ", " + std::to_string(p.y) + ")";
    }
}

// Usage
Point p(10, 20);
std::string msg = ufmt::format("Position: {0}", p);
// Result: "Position: (10, 20)"
```

## Format Specifications

ufmt supports printf-style format specifications:

### Floating Point
- `{0:.2f}` - 2 decimal places: `3.14`
- `{0:.3e}` - Scientific notation: `3.142e+00`
- `{0:8.2f}` - Width 8, 2 decimals: `    3.14`

### Integers
- `{0:d}` - Decimal: `255`
- `{0:x}` - Hexadecimal: `ff`
- `{0:X}` - Uppercase hex: `FF`
- `{0:o}` - Octal: `377`
- `{0:b}` - Binary: `0b11111111`
- `{0:08d}` - Zero-padded: `00000255`
- `{0:016b}` - Zero-padded binary: `0b0000000011111111`

### Strings
- `{0:10}` - Right-aligned, width 10: `     hello`
- `{0:-10}` - Left-aligned, width 10: `hello     `
- `{0:^10}` - Center-aligned, width 10: `  hello   `
- `{0:.5}` - Truncate to 5 characters: `hello`
- `{0:10.5}` - Right-aligned, width 10, truncate to 5: `     hello`
- `{0:-10.5}` - Left-aligned, width 10, truncate to 5: `hello     `
- `{0:^10.5}` - Center-aligned, width 10, truncate to 5: `  hello   `
- `{0:.3}` - Truncate to 3 characters, ellipsis if >3: `hel` or `he...` (see below)

**Truncation details:**
- If `.precision` is specified for a string, the output is truncated to that length.
- If the precision is greater than 3, the output is truncated and an ellipsis (`...`) is appended (e.g., `{0:.7}` for `"abcdefgh"` gives `"abcd..."`).
- For precision ≤ 3, no ellipsis is added.

## Type Conversion and Bypass Rules

ufmt uses different conversion mechanisms based on context and priority:

### 1. Custom Formatters (Highest Priority)
When a custom formatter is registered, it completely bypasses `to_string`:

```cpp
auto ctx = ufmt::create_local_context();
ctx->set_formatter<bool>([](bool b) { return b ? "YES" : "NO"; });
std::string result = ctx->format("{0}", true); // -> "YES" (bypasses to_string)
```

### 2. Format Specifications
For types with format specifications, specialized formatting functions are used:

```cpp
// Uses format_double_value, NOT to_string
ufmt::format("{0:.2f}", 3.14159); // -> "3.14"

// Uses format_integer_value, NOT to_string  
ufmt::format("{0:x}", 255);  // -> "ff"
ufmt::format("{0:b}", 255);  // -> "0b11111111"

// Uses apply_string_formatting, NOT to_string
ufmt::format("{0:10}", "hello"); // -> "     hello"
```

### 3. String Types (Optimization)
String types are handled directly for efficiency:

```cpp
ufmt::format("{0}", std::string("hello")); // Direct use, no conversion
ufmt::format("{0}", "hello");              // Direct conversion to std::string
```

### 4. Default Conversion
Only when no custom formatter exists and no format specification is provided:

```cpp
// Uses ufmt::to_string<T> or ustr::to_string<T> (if UFMT_USE_USTR is defined)
Point p(10, 20);
std::string msg = ufmt::format("{0}", p); // -> "(10, 20)" via to_string<Point>
```

**Priority Order:**
1. Custom Formatters → Always take precedence
2. Format Specifications → For numeric/string formatting
3. ustr Integration → If `UFMT_USE_USTR` is defined
4. Built-in Optimizations → Direct string handling
5. Default to_string → Final fallback

## Installation

### Header-Only Usage

Simply copy `include/ufmt/ufmt.h` to your project and include it:

```cpp
#include "ufmt/ufmt.h"
```

### CMake Integration

1. **As a subdirectory:**
```cmake
add_subdirectory(ufmt)
target_link_libraries(your_target ufmt)
```

2. **Find package (after installation):**
```cmake
find_package(ufmt REQUIRED)
target_link_libraries(your_target ufmt::ufmt)
```

## Building and Testing

### Prerequisites
- C++11 compatible compiler (GCC 4.8+, Clang 3.3+, MSVC 2015+)
- CMake 3.10+

### Build Commands

```bash
# Complete rebuild
./rebuild.sh

# Run all tests
./run_tests.sh

# Run all demos
./run_demos.sh

# Run ustr.h integration demo
./run_ustr_demo.sh

# Run benchmarks
./run_benchmarks.sh

# Build documentation
./build_docs.sh

# Complete rebuild
./rebuild.sh

# Manual build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
make run_tests
make run_demo
```

## Context Types

ufmt provides three types of formatting contexts:

### 1. Internal Context (Implicit)
- Used automatically by `ufmt::format()` function
- No variable storage or custom formatters
- Maximum performance for simple formatting
- Thread-safe (stateless)

### 2. Local Context (Single-thread)
- Fast, RAII-based context for single-thread use
- Supports variables and custom formatters
- Not thread-safe (by design)
- Available as value type or smart pointer

### 3. Shared Context (Thread-safe)
- Thread-safe context with mutex protection
- Supports variables and custom formatters
- Can be named and shared between threads
- Transparent variable management (main thread vs worker threads)

## API Reference

### Global Functions

```cpp
// Simple formatting with positional arguments
template<typename... Args>
std::string format(const std::string& template_str, Args&&... args);

// Create local context (returns smart pointer for uniform API)
std::unique_ptr<local_context> create_local_context();

// Create shared context (owning)
std::unique_ptr<shared_context> create_shared_context();

// Get/create named shared context (thread-safe)
std::shared_ptr<shared_context> get_shared_context(const std::string& name);
```

### Context Methods

```cpp
class context_base {
public:
    // Format with arguments
    template<typename... Args>
    std::string format(const std::string& template_str, Args&&... args);
    
    // Variable management
    void set_var(const std::string& name, const std::string& value);
    template<typename T>
    void set_var(const std::string& name, const T& value);
    void clear_var(const std::string& name);
    bool has_var(const std::string& name) const;
    
    // Custom formatters
    template<typename T>
    void set_formatter(std::function<std::string(const T&)> formatter);
    template<typename T>
    void clear_formatter();
    template<typename T>
    bool has_formatter() const;
};
```

## Performance

ufmt is designed for efficiency:

- **Zero Allocation**: for simple cases without format specifications
- **Minimal Overhead**: direct string manipulation without parsing trees
- **Template Optimization**: compile-time type handling
- **Lock-Free Reading**: scoped contexts are not thread-safe but very fast
- **Fine-Grained Locking**: shared contexts use mutex only when needed

## Thread Safety

- **Global `format()` function**: Thread-safe
- **Scoped contexts**: NOT thread-safe (single-thread use only)
- **Shared contexts**: Fully thread-safe with internal synchronization
- **Context manager**: Thread-safe context creation and retrieval

## Error Handling

```cpp
try {
    auto result = ufmt::format("Invalid {0:.invalid}", 3.14);
} catch (const ufmt::format_error& e) {
    // Handle formatting errors
} catch (const ufmt::parse_error& e) {
    // Handle template parsing errors  
} catch (const ufmt::argument_error& e) {
    // Handle argument-related errors
}
```

## Examples and Demos

The project includes several comprehensive demos:

- **demo_basic.cpp**: Basic formatting features and API usage
- **demo_multithreading.cpp**: Thread-safe context usage examples  
- **demo_transparent_api.cpp**: Transparent API demonstrations
- **demo_ustr_integration.cpp**: Optional ustr.h integration showcase
- **ufmt_benchmark.cpp**: Performance benchmarking
- **ufmt_multithreading_benchmark.cpp**: Multi-threading performance tests

## Use Cases

1. **Exception Messages**: Detailed error formatting with context
2. **GUI Applications**: User-facing message formatting
3. **Logging**: Structured log message creation
4. **REST APIs**: Consistent response message formatting
5. **CLI Tools**: Command-line output formatting
6. **Configuration**: Template-based configuration strings

## Optional ustr.h Integration

ufmt can optionally integrate with ustr.h for universal type conversion. This allows ufmt to automatically handle any type that ustr.h supports.

### Enabling ustr.h Integration

```cpp
#include "ustr.h"           // Include ustr.h first
#define UFMT_USE_USTR       // Enable ustr integration
#include "ufmt.h"           // ufmt will now use ustr::to_string()

// Now ufmt will use ustr::to_string() for all type conversions
struct CustomType { int value; };
CustomType obj{42};
std::string result = ufmt::format("Object: {0}", obj); // Uses ustr::to_string(obj)
```

**Important Notes:**
- ustr.h must be included **before** ufmt.h
- The `UFMT_USE_USTR` macro must be defined **before** including ufmt.h  
- If `UFMT_USE_USTR` is defined but ustr.h is not properly included, compilation will fail with a clear error
- When ustr.h integration is enabled, ustr::to_string() is used for **all** type conversions

## Design Goals

- **Simplicity**: Minimal API surface, easy to use
- **Performance**: Efficient formatting with minimal overhead
- **Flexibility**: Custom formatters and named variables
- **Thread Safety**: Safe concurrent access where needed
- **Extensibility**: Easy integration with custom types

## Comparison with Alternatives

| Feature | ufmt | std::format (C++20) | fmt library | printf |
|---------|------|-------------------|-------------|--------|
| Header-Only | ✅ | ✅ | ❌ | ✅ |
| C++11 Support | ✅ | ❌ | ✅ | ✅ |
| Named Variables | ✅ | ❌ | ❌ | ❌ |
| Custom Formatters | ✅ | ✅ | ✅ | ❌ |
| Thread-Safe Contexts | ✅ | ❌ | ❌ | ❌ |
| Size | Micro | Large | Large | Small |

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -am 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Author

**Piotr Likus** - [GitHub](https://github.com/vpiotr)

## Acknowledgments

- Inspired by the formatting capabilities of [dlog.h](tmp/dlog/)
- Design influenced by modern formatting libraries while maintaining C++11 compatibility
- Test framework uses the lightweight [utest.h](include/utest/)
- Optional integration with [ustr.h](include/ustr/) for universal type conversion

## Project Scripts

The project includes several convenience scripts for common tasks:

- `./rebuild.sh` - Complete clean rebuild
- `./run_tests.sh` - Run all test suites
- `./run_demos.sh` - Run all demonstration programs
- `./run_ustr_demo.sh` - Demonstrate ustr.h integration
- `./run_benchmarks.sh` - Run performance benchmarks
- `./build_docs.sh` - Generate Doxygen documentation

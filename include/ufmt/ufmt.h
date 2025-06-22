/**
 * @file ufmt.h
 * @brief ufmt - A lightweight, header-only C++11 micro-library for string template formatting
 * 
 * ufmt provides a simplified, extensible alternative to standard formatting libraries 
 * with support for custom formatters, named variables, and both local and shared contexts.
 *
 * Features:
 * - Positional and named placeholder support ({0}, {variable_name})
 * - Printf-style formatting with positional arguments (.2f, d, x, etc.)
 * - Width and justification formatting for strings and numbers  
 * - Custom formatters for user-defined types
 * - Local contexts (RAII) and shared named contexts (thread-safe)
 * - Simple API with minimal overhead
 * - C++11 compatible
 * - Optional ustr.h integration for universal type conversion
 *
 * Basic Usage:
 * @code
 * // Simple formatting (using internal singleton context)
 * std::string msg = ufmt::format("User {0} has {1} messages", "Alice", 5);
 * 
 * // Local context with custom formatter and variables (single-thread)
 * auto ctx = ufmt::create_local_context();
 * ctx.set_formatter<bool>([](bool b) { return b ? "YES" : "NO"; });
 * ctx.set_var("name", "Bob");
 * std::string result = ctx.format("Hello {name}, Active: {0}", true);
 * 
 * // Smart pointer version of local context
 * auto ctx_ptr = ufmt::make_local_context();
 * ctx_ptr->set_var("name", "Bob");
 * ctx_ptr->set_formatter<int>([](int n) { return "NUM:" + std::to_string(n); });
 * std::string result2 = ctx_ptr->format("Hello {name}, Value: {0}", 42);
 * 
 * // Shared context with variables and formatters (thread-safe)
 * auto ctx = ufmt::get_shared_context("app");
 * ctx->set_var("user", "Adam");
 * ctx->set_var("id", 12345);
 * std::string msg = ctx->format("User: {user} (ID: {id})");
 * 
 * // Create new shared context (thread-safe, owning)
 * auto ctx = ufmt::create_shared_context();
 * ctx->set_var("temp", "value");
 * std::string result = ctx->format("Temp: {temp}");
 * 
 * // Optional ustr.h integration
 * // #include "ustr.h"          // Must be included first
 * // #define UFMT_USE_USTR       // Enable ustr integration
 * // #include "ufmt.h"          // Will automatically use ustr::to_string()
 * // custom_type obj;
 * // std::string result = ufmt::format("Object: {0}", obj); // Uses ustr::to_string(obj)
 * @endcode
 *
 * @author Piotr Likus
 * License: MIT
 * @version 1.0
 * @date 2025
 */

#ifndef __UFMT_H__
#define __UFMT_H__

#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <functional>
#include <typeinfo>
#include <typeindex>
#include <memory>
#include <thread>
#include <mutex>
#include <cstdio>
#include <cctype>
#include <stdexcept>

/**
 * @namespace ufmt
 * @brief Main namespace for the ufmt formatting library
 */
namespace ufmt {

/**
 * @defgroup core Core API
 * @brief Core formatting functions and classes
 */

/**
 * @defgroup contexts Context Classes  
 * @brief Formatting context implementations
 */

/**
 * @defgroup exceptions Exception Classes
 * @brief Error handling and exception types
 */

/**
 * @defgroup formatting Format Specifications
 * @brief Format specification parsing and handling
 */

/**
 * @defgroup integration Integration Support
 * @brief Integration with external libraries (ustr.h)
 */

// ========== Forward Declarations ==========

class context_base;
class local_context;
class shared_context;

// ========== Exception Classes ==========

/**
 * @brief Base class for all ufmt formatting errors
 * @ingroup exceptions
 */
class format_error : public std::runtime_error {
public:
    explicit format_error(const std::string& message) : std::runtime_error(message) {}
};

/**
 * @brief Error during template parsing
 * @ingroup exceptions
 */
class parse_error : public format_error {
public:
    parse_error(const std::string& template_str, size_t position, const std::string& reason)
        : format_error("Parse error at position " + std::to_string(position) + 
                      " in template '" + template_str + "': " + reason) {}
};

/**
 * @brief Error with placeholder arguments
 * @ingroup exceptions
 */
class argument_error : public format_error {
public:
    argument_error(const std::string& placeholder, const std::string& reason)
        : format_error("Argument error for placeholder '" + placeholder + "': " + reason) {}
};

// ========== Format Specification ==========

/**
 * @brief Parsed format specification for a placeholder
 * @ingroup formatting
 */
struct FormatSpec {
    std::string name;           ///< Variable name or positional index
    std::string format_string;  ///< Format specification (e.g., ".2f", "08x")
    int width = 0;              ///< Field width
    int precision = -1;         ///< Decimal precision
    bool left_justify = false;  ///< Left justification flag
    char format_type = '\0';    ///< Format type (f, d, x, etc.)
    bool zero_pad = false;      ///< Zero padding flag
};

// ========== Optional ustr.h Integration ==========

// Enable ustr.h integration by defining UFMT_USE_USTR
// Note: When using UFMT_USE_USTR, make sure to include ustr.h before ufmt.h
// The user is responsible for ensuring ustr.h is properly included
#ifdef UFMT_USE_USTR
// We'll attempt to use ustr::to_string - if it's not available, compilation will fail
// with a clear error indicating that ustr.h needs to be included
#endif

// ========== Type System ==========

namespace detail {

/**
 * @brief Default to_string implementation for built-in types
 */
// SFINAE: detect if T is streamable to std::ostream
template<typename T>
class is_streamable {
    template<typename U>
    static auto test(int) -> decltype(std::declval<std::ostream&>() << std::declval<const U&>(), std::true_type());
    template<typename>
    static std::false_type test(...);
public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

// Streamable version
template<typename T>
typename std::enable_if<is_streamable<T>::value, std::string>::type
safe_to_string(const T& value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}
// Fallback version for non-streamable types
template<typename T>
typename std::enable_if<!is_streamable<T>::value, std::string>::type
safe_to_string(const T& value) {
    std::ostringstream ss;
    ss << "[" << typeid(T).name() << " at " << &value << "]";
    return ss.str();
}

// Replace to_string_internal to use safe_to_string
template<typename T>
std::string to_string_internal(const T& value) {
    return safe_to_string(value);
}

template<typename T>
std::string to_string_impl(const T& value) {
#ifdef UFMT_USE_USTR
    // Use ustr::to_string when UFMT_USE_USTR is defined
    // If ustr.h is not properly included, this will result in a compilation error
    return ustr::to_string(value);
#else
    // Use internal stream-based conversion when UFMT_USE_USTR is not defined
    return to_string_internal(value);
#endif
}

// Specializations for specific types
// Note: When UFMT_USE_USTR is defined, ustr::to_string is used by default
// These specializations provide optimized conversions for common types
#ifndef UFMT_USE_USTR
template<>
inline std::string to_string_impl<std::string>(const std::string& value) {
    return value;
}

template<>
inline std::string to_string_impl<const char*>(const char* const& value) {
    return std::string(value);
}

template<>
inline std::string to_string_impl<char*>(char* const& value) {
    return std::string(value);
}

template<>
inline std::string to_string_impl<bool>(const bool& value) {
    return value ? "true" : "false";
}

template<>
inline std::string to_string_impl<int>(const int& value) {
    return std::to_string(value);
}

template<>
inline std::string to_string_impl<long>(const long& value) {
    return std::to_string(value);
}

template<>
inline std::string to_string_impl<long long>(const long long& value) {
    return std::to_string(value);
}

template<>
inline std::string to_string_impl<unsigned int>(const unsigned int& value) {
    return std::to_string(value);
}

template<>
inline std::string to_string_impl<unsigned long>(const unsigned long& value) {
    return std::to_string(value);
}

template<>
inline std::string to_string_impl<unsigned long long>(const unsigned long long& value) {
    return std::to_string(value);
}

template<>
inline std::string to_string_impl<float>(const float& value) {
    return std::to_string(value);
}

template<>
inline std::string to_string_impl<double>(const double& value) {
    return std::to_string(value);
}

template<>
inline std::string to_string_impl<char>(const char& value) {
    return std::string(1, value);
}
#else
// When UFMT_USE_USTR is defined, provide minimal necessary specializations
// for performance and safety, but let ustr::to_string handle most types
template<>
inline std::string to_string_impl<std::string>(const std::string& value) {
    return value; // Direct return for efficiency
}

template<>
inline std::string to_string_impl<const char*>(const char* const& value) {
    return std::string(value); // Direct conversion for safety
}

template<>
inline std::string to_string_impl<char*>(char* const& value) {
    return std::string(value); // Direct conversion for safety
}
#endif

// ========== Formatting Functions (adapted from dlog.h) ==========

/**
 * @brief Format double value with printf-style format specification
 */
inline std::string format_double_value(double value, const std::string& formatSpec) {
    if (formatSpec.empty()) {
        return std::to_string(value);
    }
    
    char buffer[256];
    std::string printfFormat = "%" + formatSpec;
    snprintf(buffer, sizeof(buffer), printfFormat.c_str(), value);
    return std::string(buffer);
}

/**
 * @brief Format integer value with printf-style format specification
 */
inline std::string format_integer_value(long long value, const std::string& formatSpec) {
    if (formatSpec.empty()) {
        return std::to_string(value);
    }
    
    // Check for binary format (b or B)
    if (!formatSpec.empty() && (formatSpec.back() == 'b' || formatSpec.back() == 'B')) {
        // Handle binary formatting
        if (value == 0) {
            return "0b0";
        }
        
        std::string binary;
        unsigned long long uval = static_cast<unsigned long long>(value);
        
        // Convert to binary
        while (uval > 0) {
            binary = (uval & 1 ? '1' : '0') + binary;
            uval >>= 1;
        }
        
        // Apply width padding if specified
        std::string widthSpec = formatSpec.substr(0, formatSpec.length() - 1);
        if (!widthSpec.empty()) {
            try {
                int width = std::stoi(widthSpec);
                if (width > static_cast<int>(binary.length() + 2)) { // +2 for "0b"
                    int padding = width - static_cast<int>(binary.length()) - 2;
                    if (widthSpec[0] == '0') {
                        // Zero padding after "0b"
                        binary = std::string(static_cast<size_t>(padding), '0') + binary;
                    } else {
                        // Space padding before "0b"
                        return std::string(static_cast<size_t>(padding), ' ') + "0b" + binary;
                    }
                }
            } catch (const std::exception&) {
                // Invalid width specification, ignore
            }
        }
        
        return "0b" + binary;
    }
    
    char buffer[256];
    std::string printfFormat = "%" + formatSpec;
    snprintf(buffer, sizeof(buffer), printfFormat.c_str(), value);
    return std::string(buffer);
}

/**
 * @brief Apply string formatting (width and justification)
 */
inline std::string apply_string_formatting(const std::string& value, const std::string& formatSpec) {
    if (formatSpec.empty()) {
        return value;
    }
    
    // Parse format spec: [alignment][width][.precision]
    // alignment: '-' (left), '^' (center), default (right)
    // width: field width
    // precision: max characters (truncate with ellipsis if longer)
    
    bool leftJustified = false;
    bool centerJustified = false;
    int width = 0;
    int maxLen = -1;
    
    std::string spec = formatSpec;
    size_t pos = 0;
    
    // Parse alignment
    if (!spec.empty() && (spec[0] == '-' || spec[0] == '^')) {
        if (spec[0] == '-') {
            leftJustified = true;
        } else if (spec[0] == '^') {
            centerJustified = true;
        }
        pos = 1;
    }
    
    // Parse width and precision
    size_t dotPos = spec.find('.', pos);
    if (dotPos != std::string::npos) {
        // Has precision specifier - for strings this means truncation
        if (dotPos > pos) {
            width = std::atoi(spec.substr(pos, dotPos - pos).c_str());
        }
        maxLen = std::atoi(spec.substr(dotPos + 1).c_str());
    } else {
        // Only width, no truncation
        if (pos < spec.length()) {
            width = std::atoi(spec.substr(pos).c_str());
        }
    }
    
    // Apply truncation with ellipsis if needed (only when .precision is explicitly specified)
    std::string processedValue = value;
    if (maxLen > 0 && static_cast<int>(value.length()) > maxLen) {
        if (maxLen <= 3) {
            // For very short precision, just take first N characters without ellipsis
            processedValue = value.substr(0, static_cast<std::string::size_type>(maxLen));
        } else {
            // For longer precision, truncate and add ellipsis
            processedValue = value.substr(0, static_cast<std::string::size_type>(maxLen - 3)) + "...";
        }
    }
    
    // Apply width and justification
    if (width <= 0 || width <= static_cast<int>(processedValue.length())) {
        return processedValue;
    }
    
    int padding = width - static_cast<int>(processedValue.length());
    
    if (leftJustified) {
        return processedValue + std::string(static_cast<size_t>(padding), ' ');
    } else if (centerJustified) {
        int leftPad = padding / 2;
        int rightPad = padding - leftPad;
        return std::string(static_cast<size_t>(leftPad), ' ') + processedValue + 
               std::string(static_cast<size_t>(rightPad), ' ');
    } else {
        // Right justified (default)
        return std::string(static_cast<size_t>(padding), ' ') + processedValue;
    }
}

/**
 * @brief Default format function for various types
 */
template<typename T>
std::string format_value(const T& value, const std::string& formatSpec) {
    return apply_string_formatting(to_string_impl(value), formatSpec);
}

// Specializations for numeric types
template<>
inline std::string format_value<float>(const float& value, const std::string& formatSpec) {
    return format_double_value(static_cast<double>(value), formatSpec);
}

template<>
inline std::string format_value<double>(const double& value, const std::string& formatSpec) {
    return format_double_value(value, formatSpec);
}

template<>
inline std::string format_value<int>(const int& value, const std::string& formatSpec) {
    return format_integer_value(static_cast<long long>(value), formatSpec);
}

template<>
inline std::string format_value<long>(const long& value, const std::string& formatSpec) {
    return format_integer_value(static_cast<long long>(value), formatSpec);
}

template<>
inline std::string format_value<long long>(const long long& value, const std::string& formatSpec) {
    return format_integer_value(value, formatSpec);
}

template<>
inline std::string format_value<unsigned int>(const unsigned int& value, const std::string& formatSpec) {
    return format_integer_value(static_cast<long long>(value), formatSpec);
}

template<>
inline std::string format_value<unsigned long>(const unsigned long& value, const std::string& formatSpec) {
    return format_integer_value(static_cast<long long>(value), formatSpec);
}

template<>
inline std::string format_value<unsigned long long>(const unsigned long long& value, const std::string& formatSpec) {
    return format_integer_value(static_cast<long long>(value), formatSpec);
}

template<>
inline std::string format_value<char>(const char& value, const std::string& formatSpec) {
    // If format spec looks like integer format, treat as int
    if (!formatSpec.empty() && (formatSpec.back() == 'd' || formatSpec.back() == 'x' || formatSpec.back() == 'o')) {
        return format_integer_value(static_cast<long long>(value), formatSpec);
    }
    
    std::string str(1, value);
    return apply_string_formatting(str, formatSpec);
}

template<>
inline std::string format_value<bool>(const bool& value, const std::string& formatSpec) {
    std::string str = value ? "true" : "false";
    return apply_string_formatting(str, formatSpec);
}

template<>
inline std::string format_value<std::string>(const std::string& value, const std::string& formatSpec) {
    return apply_string_formatting(value, formatSpec);
}

template<>
inline std::string format_value<const char*>(const char* const& value, const std::string& formatSpec) {
    return apply_string_formatting(std::string(value), formatSpec);
}

/**
 * @brief Apply format specification to a string value
 * This function attempts to determine the type from the string and apply appropriate formatting
 */
inline std::string apply_format(const std::string& value, const std::string& formatSpec) {
    if (formatSpec.empty()) {
        return value;
    }
    
    // Parse the format spec to separate alignment/width from type specifier
    // Format: [alignment][width][.precision][type]
    std::string spec = formatSpec;
    
    // Extract alignment (-, ^)
    std::string alignment;
    size_t pos = 0;
    if (!spec.empty() && (spec[0] == '-' || spec[0] == '^')) {
        alignment = spec[0];
        pos = 1;
    }
    
    // Find the type specifier (last non-digit character)
    char typeSpec = '\0';
    std::string numericPart;
    
    // Look for type character (f, d, x, etc.)
    for (size_t i = spec.length(); i > pos; --i) {
        char c = spec[i-1];
        if (!std::isdigit(c) && c != '.') {
            typeSpec = c;
            numericPart = spec.substr(pos, i-1-pos);
            break;
        }
    }
    
    // If no type specifier found, use whole remaining part
    if (typeSpec == '\0') {
        numericPart = spec.substr(pos);
    }
    
    // Check if it's a numeric format and handle accordingly
    if (typeSpec == 'f' || typeSpec == 'F' || typeSpec == 'g' || typeSpec == 'G' || 
        typeSpec == 'e' || typeSpec == 'E') {
        // Floating point format
        try {
            double numValue = std::stod(value);
            
            // Apply alignment if specified (only width, no precision for alignment)
            if (!alignment.empty()) {
                // Extract width and precision parts for separate handling
                std::string widthPart;
                std::string precisionPart;
                size_t dotPos = numericPart.find('.');
                if (dotPos != std::string::npos) {
                    widthPart = numericPart.substr(0, dotPos);
                    precisionPart = numericPart.substr(dotPos); // includes the dot
                } else {
                    widthPart = numericPart;
                }
                
                // Format number with precision only (no width for printf)
                std::string numericFormat = precisionPart + typeSpec;
                std::string formattedNum = format_double_value(numValue, numericFormat);
                
                // Apply alignment and width through string formatting
                return apply_string_formatting(formattedNum, alignment + widthPart);
            } else {
                // No alignment specified, use full printf formatting
                std::string numericFormat = numericPart + typeSpec;
                return format_double_value(numValue, numericFormat);
            }
        } catch (const std::exception&) {
            // If parsing fails, treat as string
            return apply_string_formatting(value, formatSpec);
        }
    }
    
    if (typeSpec == 'd' || typeSpec == 'i' || typeSpec == 'x' || typeSpec == 'X' || 
        typeSpec == 'o' || typeSpec == 'u' || typeSpec == 'b' || typeSpec == 'B') {
        // Integer format
        try {
            long long numValue = std::stoll(value);
            
            // Apply alignment if specified (only width, no precision for alignment)
            if (!alignment.empty()) {
                // Extract width and precision parts for separate handling
                std::string widthPart;
                std::string precisionPart;
                size_t dotPos = numericPart.find('.');
                if (dotPos != std::string::npos) {
                    widthPart = numericPart.substr(0, dotPos);
                    precisionPart = numericPart.substr(dotPos); // includes the dot
                } else {
                    widthPart = numericPart;
                }
                
                // Format number with precision only (no width for printf)
                std::string numericFormat = precisionPart + typeSpec;
                std::string formattedNum = format_integer_value(numValue, numericFormat);
                
                // Apply alignment and width through string formatting
                return apply_string_formatting(formattedNum, alignment + widthPart);
            } else {
                // No alignment specified, use full printf formatting
                std::string numericFormat = numericPart + typeSpec;
                return format_integer_value(numValue, numericFormat);
            }
        } catch (const std::exception&) {
            // If parsing fails, treat as string
            return apply_string_formatting(value, formatSpec);
        }
    }
    
    // Default to string formatting (width/justification)
    return apply_string_formatting(value, formatSpec);
}

} // namespace detail

/**
 * @brief Public to_string function for user-defined types
 * @ingroup integration
 * 
 * This function will automatically use ustr::to_string() when UFMT_USE_USTR is defined,
 * otherwise it falls back to the internal implementation.
 * 
 * Usage:
 * - Without UFMT_USE_USTR: Uses ostringstream or specialized conversions
 * - With UFMT_USE_USTR: Uses ustr::to_string() for universal type support
 */
template<typename T>
std::string to_string(const T& value) {
    return detail::to_string_impl(value);
}

// ========== Base Context Interface ==========

/**
 * @brief Base class for all formatting contexts - provides core formatting logic
 * @ingroup contexts
 */
class format_context_base {
public:
    virtual ~format_context_base() = default;
    
    /**
     * @brief Core formatting method
     * @param template_str Template string with placeholders
     * @param args Variadic arguments to substitute
     * @return Formatted string
     */
    template<typename... Args>
    std::string format(const std::string& template_str, Args&&... args) {
        return format_impl(template_str, std::forward<Args>(args)...);
    }
    
    /**
     * @brief Check if a named variable exists
     * @param name Variable name to check
     * @return true if variable exists
     */
    virtual bool has_var(const std::string& name) const = 0;

protected:
    /**
     * @brief Get named variable value
     * @param name Variable name
     * @return Variable value or empty string if not found
     */
    virtual std::string get_var(const std::string& name) const = 0;
    
    /**
     * @brief Check if formatter exists implementation
     */
    virtual bool has_formatter_impl(std::type_index type) const = 0;
    
    /**
     * @brief Format value using custom formatter if available
     */
    virtual std::string format_value_custom(std::type_index type, const void* value, const std::string& formatSpec) const = 0;

    // Add find_var to base for override
    virtual std::pair<bool, std::string> find_var(const std::string& name) const {
        // Default: use has_var + get_var (may double lock, but only for non-shared_context)
        if (has_var(name)) {
            return {true, get_var(name)};
        }
        return {false, std::string()};
    }

private:
    // Helper function to convert values to string
    template<typename T>
    std::string to_string(const T& value) const {
        // Check if custom formatter exists
        std::type_index type_idx(typeid(T));
        if (has_formatter_impl(type_idx)) {
            return format_value_custom(type_idx, &value, "");
        }
        
        // Use default conversion
        return detail::to_string_impl(value);
    }
    
    template<typename... Args>
    std::string format_impl(const std::string& template_str, Args&&... args) {
        std::string result = template_str;
        
        // Store arguments as strings with their formatters
        std::vector<std::pair<std::string, std::function<std::string(const std::string&)>>> formattedArgs;
        if (sizeof...(args) > 0) {
            add_args_to_vector(formattedArgs, std::forward<Args>(args)...);
        } else {
            add_args_to_vector(formattedArgs); // No-argument version
        }
        
        // Replace {0}, {1}, {0:.2f}, etc. with actual arguments
        for (size_t i = 0; i < formattedArgs.size(); ++i) {
            // Look for formatted placeholders like {0:.2f}
            std::string formatPattern = "{" + std::to_string(i) + ":";
            size_t pos = 0;
            while ((pos = result.find(formatPattern, pos)) != std::string::npos) {
                // Find the closing brace
                size_t endPos = result.find("}", pos);
                if (endPos != std::string::npos) {
                    // Extract format specifier (e.g., ".2f")
                    std::string formatSpec = result.substr(pos + formatPattern.length(), 
                                                          endPos - pos - formatPattern.length());
                    std::string placeholder = result.substr(pos, endPos - pos + 1);
                    
                    // Apply formatting using the stored formatter
                    std::string formattedValue = formattedArgs[i].second(formatSpec);
                    result.replace(pos, placeholder.length(), formattedValue);
                    pos += formattedValue.length();
                } else {
                    break;
                }
            }
            
            // Look for simple placeholders like {0}
            std::string simplePlaceholder = "{" + std::to_string(i) + "}";
            pos = 0;
            while ((pos = result.find(simplePlaceholder, pos)) != std::string::npos) {
                result.replace(pos, simplePlaceholder.length(), formattedArgs[i].first);
                pos += formattedArgs[i].first.length();
            }
        }
        
        {
            // Replace named variables like {variable_name} and {variable_name:.2f}
            size_t pos = 0;
            while ((pos = result.find("{", pos)) != std::string::npos) {
                size_t endPos = result.find("}", pos);
                if (endPos != std::string::npos) {
                    std::string placeholder = result.substr(pos + 1, endPos - pos - 1);
                    // Skip if it's a positional placeholder (already processed)
                    if (placeholder.empty() || std::isdigit(placeholder[0])) {
                        pos = endPos + 1;
                        continue;
                    }
                    // Check if it has a format specifier
                    size_t colonPos = placeholder.find(':');
                    std::string varName = placeholder;
                    std::string formatSpec;
                    if (colonPos != std::string::npos) {
                        varName = placeholder.substr(0, colonPos);
                        formatSpec = placeholder.substr(colonPos + 1);
                    }
                    // Use find_var for optimized lookup (single lock in shared_context)
                    auto found = find_var(varName);
                    if (found.first) {
                        std::string value = found.second;
                        if (!formatSpec.empty()) {
                            value = detail::apply_format(value, formatSpec);
                        }
                        result.replace(pos, endPos - pos + 1, value);
                        pos += value.length();
                    } else {
                        pos = endPos + 1;
                    }
                } else {
                    break;
                }
            }
        }
        
        return result;
    }
    
    // Recursive helper to convert arguments to formatted pairs (C++11 compatible)
    // Base case: no arguments
    void add_args_to_vector(std::vector<std::pair<std::string, std::function<std::string(const std::string&)>>>& /* vec */) {
        // Empty base case - do nothing
    }
    
    template<typename T>
    void add_args_to_vector(std::vector<std::pair<std::string, std::function<std::string(const std::string&)>>>& vec, T&& arg) {
        // Use a different name to avoid shadowing
        std::type_index type_idx(typeid(T));
        std::string stringValue;
        if (has_formatter_impl(type_idx)) {
            stringValue = format_value_custom(type_idx, &arg, ""); // Use custom formatter for default string representation
        } else {
            stringValue = to_string(arg);
        }
        
        // Create formatter function for this argument
        auto formatter = [this, arg](const std::string& formatSpec) -> std::string {
            // Use a different name to avoid shadowing
            std::type_index type_idx_lambda(typeid(T));
            if (has_formatter_impl(type_idx_lambda)) {
                return format_value_custom(type_idx_lambda, &arg, formatSpec);
            }
            return detail::format_value(arg, formatSpec);
        };
        vec.emplace_back(stringValue, formatter);
    }

    template<typename T, typename... Args>
    void add_args_to_vector(std::vector<std::pair<std::string, std::function<std::string(const std::string&)>>>& vec, T&& first, Args&&... rest) {
        add_args_to_vector(vec, std::forward<T>(first));
        add_args_to_vector(vec, std::forward<Args>(rest)...);
    }
};

/**
 * @brief Context interface for contexts that support variables and formatters
 * @ingroup contexts
 */
class context_base : public format_context_base {
public:
    /**
     * @brief Set a named variable (string value)
     * @param name Variable name
     * @param value String value
     */
    virtual void set_var(const std::string& name, const std::string& value) = 0;
    
    /**
     * @brief Set a named variable (any type, converted to string)
     * @param name Variable name  
     * @param value Value of any type
     */
    template<typename T>
    void set_var(const std::string& name, const T& value) {
        set_var(name, to_string(value));
    }
    
    /**
     * @brief Clear a named variable
     * @param name Variable name to clear
     */
    virtual void clear_var(const std::string& name) = 0;
    
    /**
     * @brief Set a custom formatter for a type
     * @param formatter Function to format values of type T
     */
    template<typename T>
    void set_formatter(std::function<std::string(const T&)> formatter) {
        auto wrapper = [formatter](const void* value) -> std::string {
            return formatter(*static_cast<const T*>(value));
        };
        set_formatter_impl(std::type_index(typeid(T)), wrapper);
    }
    
    /**
     * @brief Clear custom formatter for a type
     */
    template<typename T>
    void clear_formatter() {
        clear_formatter_impl(std::type_index(typeid(T)));
    }
    
    /**
     * @brief Check if a custom formatter exists for a type
     */
    template<typename T>
    bool has_formatter() const {
        return has_formatter_impl(std::type_index(typeid(T)));
    }

protected:
    /**
     * @brief Check if current thread is the main thread
     */
    bool is_main_thread() const {
        ensure_main_thread_id_initialized();
        return std::this_thread::get_id() == main_thread_id_;
    }
    
    /**
     * @brief Set formatter implementation (type-erased)
     */
    virtual void set_formatter_impl(std::type_index type, std::function<std::string(const void*)> formatter) = 0;
    
    /**
     * @brief Clear formatter implementation
     */
    virtual void clear_formatter_impl(std::type_index type) = 0;

private:
    // Static members for main thread detection
    static std::thread::id main_thread_id_;
    static bool main_thread_id_initialized_;
    
    // Initialize main thread ID on first use
    void ensure_main_thread_id_initialized() const {
        static std::mutex init_mutex;
        if (!main_thread_id_initialized_) {
            std::lock_guard<std::mutex> lock(init_mutex);
            if (!main_thread_id_initialized_) {
                main_thread_id_ = std::this_thread::get_id();
                main_thread_id_initialized_ = true;
            }
        }
    }
    
    // Helper function to convert values to string using formatters
    template<typename T>
    std::string to_string(const T& value) const {
        // Check if custom formatter exists
        std::type_index type_idx(typeid(T));
        if (has_formatter_impl(type_idx)) {
            return format_value_custom(type_idx, &value, "");
        }
        
        // Use default conversion
        return detail::to_string_impl(value);
    }
};

// ========== Internal Context (Private, No Variable Support) ==========

namespace detail {
/**
 * @brief Internal context for simple formatting without variable support
 * 
 * This is a minimal context implementation used internally for the main format() function.
 * It provides no variable storage or custom formatter support for maximum performance.
 */
class internal_context : public format_context_base {
public:
    internal_context() = default;
    ~internal_context() = default;
    
    // Non-copyable, non-moveable (used as singleton)
    internal_context(const internal_context&) = delete;
    internal_context& operator=(const internal_context&) = delete;
    internal_context(internal_context&&) = delete;
    internal_context& operator=(internal_context&&) = delete;

protected:
    std::string get_var(const std::string& /* name */) const override {
        return std::string(); // No variables available
    }
    
    bool has_var(const std::string& /* name */) const override {
        return false; // No variables supported
    }
    
    bool has_formatter_impl(std::type_index /* type */) const override {
        return false; // No custom formatters supported
    }
    
    std::string format_value_custom(std::type_index /* type */, const void* /* value */, const std::string& /* formatSpec */) const override {
        return std::string(); // No custom formatters available
    }
};
} // namespace detail

// ========== Local Context (Single-Thread with Variables and Formatters) ==========

/**
 * @brief Local context for single-thread formatting (not thread-safe)
 * @ingroup contexts
 * 
 * This context is designed for single-thread use and provides fast
 * performance for local formatting operations with variable support and custom formatters.
 */
class local_context : public context_base {
private:
    std::unordered_map<std::string, std::string> variables_;
    std::unordered_map<std::type_index, std::function<std::string(const void*)>> formatters_;
    
public:
    local_context() = default;
    ~local_context() = default;
    
    // Non-copyable, moveable
    local_context(const local_context&) = delete;
    local_context& operator=(const local_context&) = delete;
    local_context(local_context&&) = default;
    local_context& operator=(local_context&&) = default;
    
    // Bring template set_var into scope
    using context_base::set_var;
    
    // Simple variable management (local storage only)
    void set_var(const std::string& name, const std::string& value) override {
        variables_[name] = value;
    }
    
    void clear_var(const std::string& name) override {
        variables_.erase(name);
    }
    
    bool has_var(const std::string& name) const override {
        return variables_.find(name) != variables_.end();
    }

protected:
    std::string get_var(const std::string& name) const override {
        auto it = variables_.find(name);
        return (it != variables_.end()) ? it->second : std::string();
    }
    
    void set_formatter_impl(std::type_index type, std::function<std::string(const void*)> formatter) override {
        formatters_[type] = formatter;
    }
    
    void clear_formatter_impl(std::type_index type) override {
        formatters_.erase(type);
    }
    
    bool has_formatter_impl(std::type_index type) const override {
        return formatters_.find(type) != formatters_.end();
    }
    
    std::string format_value_custom(std::type_index type, const void* value, const std::string& /* formatSpec */) const override {
        auto it = formatters_.find(type);
        if (it != formatters_.end()) {
            return it->second(value);
        }
        return std::string();
    }
};

// ========== Shared Context (Thread-Safe) ==========

/**
 * @brief Shared context with thread-safe storage and transparent thread-local variables
 * @ingroup contexts
 * 
 * This context provides thread-safe variable and formatter management with a unique
 * transparent thread-local variable system:
 * 
 * - **Main thread**: Variables stored in shared storage (mutex-protected)
 * - **Worker threads**: Variables stored in thread-local storage (lock-free)
 * - **Variable resolution**: Thread-local variables take precedence over shared ones
 * - **Performance**: Worker threads avoid mutex contention for variable operations
 * 
 * This design allows for high-performance multi-threaded formatting where worker
 * threads can override global variables locally without affecting other threads.
 * 
 * @see TRANSPARENT_API.md for detailed documentation
 */
class shared_context : public context_base {
private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::string> variables_;
    std::unordered_map<std::type_index, std::function<std::string(const void*)>> formatters_;
    static thread_local std::unordered_map<std::string, std::string> thread_variables_;
    
public:
    shared_context() = default;
    ~shared_context() = default;
    
    // Bring template set_var into scope
    using context_base::set_var;
    
    // Transparent variable management
    void set_var(const std::string& name, const std::string& value) override {
        if (is_main_thread()) {
            // Main thread writes to shared storage
            std::lock_guard<std::mutex> lock(mutex_);
            variables_[name] = value;
        } else {
            // Worker threads write to thread-local storage
            thread_variables_[name] = value;
        }
    }
    
    void clear_var(const std::string& name) override {
        if (is_main_thread()) {
            // Main thread clears from shared storage
            std::lock_guard<std::mutex> lock(mutex_);
            variables_.erase(name);
        } else {
            // Worker threads clear from thread-local storage
            thread_variables_.erase(name);
        }
    }
    
    bool has_var(const std::string& name) const override {
        // Check thread-local first (no lock needed)
        if (thread_variables_.find(name) != thread_variables_.end()) {
            return true;
        }
        
        // Check shared storage (lock needed)
        std::lock_guard<std::mutex> lock(mutex_);
        return variables_.find(name) != variables_.end();
    }

protected:
    std::string get_var(const std::string& name) const override {
        // Check thread-local variables first (no lock needed)
        auto thread_it = thread_variables_.find(name);
        if (thread_it != thread_variables_.end()) {
            return thread_it->second;
        }
        
        // Fall back to shared variables (lock needed)
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = variables_.find(name);
        return (it != variables_.end()) ? it->second : std::string();
    }
    
    // Optimized: find variable and retrieve value in a single lock (for shared variables)
    bool find_var(const std::string& name, std::string& value) const {
        // Check thread-local variables first (no lock needed)
        auto thread_it = thread_variables_.find(name);
        if (thread_it != thread_variables_.end()) {
            value = thread_it->second;
            return true;
        }
        // Fall back to shared variables (lock needed)
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = variables_.find(name);
        if (it != variables_.end()) {
            value = it->second;
            return true;
        }
        value.clear();
        return false;
    }
    
    void set_formatter_impl(std::type_index type, std::function<std::string(const void*)> formatter) override {
        std::lock_guard<std::mutex> lock(mutex_);
        formatters_[type] = formatter;
    }
    
    void clear_formatter_impl(std::type_index type) override {
        std::lock_guard<std::mutex> lock(mutex_);
        formatters_.erase(type);
    }
    
    bool has_formatter_impl(std::type_index type) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return formatters_.find(type) != formatters_.end();
    }
    
    std::string format_value_custom(std::type_index type, const void* value, const std::string& /* formatSpec */) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = formatters_.find(type);
        if (it != formatters_.end()) {
            return it->second(value);
        }
        return std::string();
    }
    
protected:
    // Override find_var for optimized single-lock access
    std::pair<bool, std::string> find_var(const std::string& name) const override {
        std::string value;
        if (find_var(name, value)) {
            return {true, value};
        }
        return {false, std::string()};
    }
};

// ========== Context Manager (Thread-Safe) ==========

/**
 * @brief Thread-safe manager for named shared contexts
 * @ingroup contexts
 * 
 * Manages a global registry of named shared contexts that can be accessed
 * from multiple threads safely. Contexts are created on-demand and remain
 * alive as long as shared_ptr references exist.
 * 
 * All operations are thread-safe and can be called concurrently from
 * multiple threads.
 */
class context_manager {
private:
    static std::mutex contexts_mutex_;
    static std::unordered_map<std::string, std::shared_ptr<shared_context>> contexts_;
    
public:
    /**
     * @brief Get or create a named shared context
     * @param name Context name
     * @return Shared pointer to the context
     */
    static std::shared_ptr<shared_context> get_context(const std::string& name) {
        std::lock_guard<std::mutex> lock(contexts_mutex_);
        auto it = contexts_.find(name);
        if (it == contexts_.end()) {
            auto ctx = std::make_shared<shared_context>();
            contexts_[name] = ctx;
            return ctx;
        }
        return it->second;
    }
    
    /**
     * @brief Remove a named context
     * @param name Context name to remove
     */
    static void remove_context(const std::string& name) {
        std::lock_guard<std::mutex> lock(contexts_mutex_);
        contexts_.erase(name);
    }
    
    /**
     * @brief Clear all named contexts
     */
    static void clear_all_contexts() {
        std::lock_guard<std::mutex> lock(contexts_mutex_);
        contexts_.clear();
    }
};

// Static member definitions
std::mutex context_manager::contexts_mutex_;
std::unordered_map<std::string, std::shared_ptr<shared_context>> context_manager::contexts_;

// Context base static members
std::thread::id context_base::main_thread_id_;
bool context_base::main_thread_id_initialized_ = false;

// ========== Public API Functions ==========

/**
 * @brief Get singleton internal context for simple formatting operations
 * @return Reference to singleton internal_context (private implementation)
 */
namespace detail {
inline internal_context& get_singleton_internal_context() {
    static internal_context singleton;
    return singleton;
}
} // namespace detail

/**
 * @brief Simple string formatting with positional arguments (using internal singleton context)
 * @ingroup core
 * @param template_str Template string with {0}, {1}, etc. placeholders
 * @param args Arguments to substitute
 * @return Formatted string
 */
template<typename... Args>
std::string format(const std::string& template_str, Args&&... args) {
    return detail::get_singleton_internal_context().format(template_str, std::forward<Args>(args)...);
}

/**
 * @brief Create a new local context (single-thread, smart pointer)
 * @ingroup core
 * @return Unique pointer to local context
 */
inline std::unique_ptr<local_context> create_local_context() {
    return std::unique_ptr<local_context>(new local_context());
}

/**
 * @brief Create a new shared context (thread-safe, owning smart pointer)
 * @ingroup core
 * @return Unique pointer to shared context
 */
inline std::unique_ptr<shared_context> create_shared_context() {
    return std::unique_ptr<shared_context>(new shared_context());
}

/**
 * @brief Get or create a named shared context (thread-safe)
 * @ingroup core
 * @param name Context name
 * @return Shared pointer to context
 */
inline std::shared_ptr<shared_context> get_shared_context(const std::string& name) {
    return context_manager::get_context(name);
}

// ========== Thread-local Storage Definitions ==========

/**
 * @brief Thread-local storage for shared_context variables
 */
thread_local std::unordered_map<std::string, std::string> shared_context::thread_variables_;

} // namespace ufmt

#endif // __UFMT_H__

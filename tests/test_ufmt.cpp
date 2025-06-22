#include "../include/ufmt/ufmt.h"
#include "../include/utest/utest.h"

// Test basic formatting functionality
UTEST_FUNC_DEF(BasicFormatting) {
    // Simple positional arguments
    auto result = ufmt::format("Hello {0}, you have {1} messages", "Alice", 5);
    UTEST_ASSERT_STR_EQUALS(result, "Hello Alice, you have 5 messages");
    
    // Multiple types
    auto result2 = ufmt::format("User: {0}, Score: {1}, Active: {2}", "Bob", 87.5, true);
    UTEST_ASSERT_STR_EQUALS(result2, "User: Bob, Score: 87.500000, Active: true");
    
    // Empty template
    auto result3 = ufmt::format("No placeholders");
    UTEST_ASSERT_STR_EQUALS(result3, "No placeholders");
}

// Test format specifications
UTEST_FUNC_DEF(FormatSpecifications) {
    // Float precision
    auto result1 = ufmt::format("Pi = {0:.3f}", 3.14159);
    UTEST_ASSERT_STR_EQUALS(result1, "Pi = 3.142");
    
    // Integer hex
    auto result2 = ufmt::format("Hex: 0x{0:x}", 255);
    UTEST_ASSERT_STR_EQUALS(result2, "Hex: 0xff");
    
    // Integer decimal with padding
    auto result3 = ufmt::format("ID: {0:08d}", 42);
    UTEST_ASSERT_STR_EQUALS(result3, "ID: 00000042");
    
    // String width and justification
    auto result4 = ufmt::format("Name: '{0:10}' Score: '{1:-8}'", "Bob", "92.3");
    UTEST_ASSERT_STR_EQUALS(result4, "Name: '       Bob' Score: '92.3    '");
}

// Test local context
UTEST_FUNC_DEF(LocalContext) {
    auto ctx = ufmt::create_local_context();
    
    // Set variables
    ctx->set_var("name", "Alice");
    ctx->set_var("age", 25);
    ctx->set_var("score", 87.5);
    
    // Test variable substitution
    auto result1 = ctx->format("User {name} (age {age}) has score {score}");
    UTEST_ASSERT_STR_EQUALS(result1, "User Alice (age 25) has score 87.500000");
    
    // Test mixed positional and named
    auto result2 = ctx->format("Hello {0}, your name is {name}", "Guest");
    UTEST_ASSERT_STR_EQUALS(result2, "Hello Guest, your name is Alice");
    
    // Test variable existence
    UTEST_ASSERT_TRUE(ctx->has_var("name"));
    UTEST_ASSERT_FALSE(ctx->has_var("nonexistent"));
    
    // Test variable clearing
    ctx->clear_var("name");
    UTEST_ASSERT_FALSE(ctx->has_var("name"));
}

// Test custom formatters
UTEST_FUNC_DEF(CustomFormatters) {
    auto ctx = ufmt::create_local_context();
    
    // Set custom bool formatter
    ctx->set_formatter<bool>([](bool b) { return b ? "YES" : "NO"; });
    
    auto result1 = ctx->format("Active: {0}", true);
    UTEST_ASSERT_STR_EQUALS(result1, "Active: YES");
    
    auto result2 = ctx->format("Disabled: {0}", false);
    UTEST_ASSERT_STR_EQUALS(result2, "Disabled: NO");
    
    // Test formatter existence
    UTEST_ASSERT_TRUE(ctx->has_formatter<bool>());
    UTEST_ASSERT_FALSE(ctx->has_formatter<int>());
    
    // Clear formatter
    ctx->clear_formatter<bool>();
    UTEST_ASSERT_FALSE(ctx->has_formatter<bool>());
    
    // Should use default formatting now
    auto result3 = ctx->format("Default: {0}", true);
    UTEST_ASSERT_STR_EQUALS(result3, "Default: true");
}

// Test shared context
UTEST_FUNC_DEF(SharedContext) {
    auto ctx1 = ufmt::get_shared_context("test");
    auto ctx2 = ufmt::get_shared_context("test");
    
    // Should be the same context
    ctx1->set_var("shared_var", "shared_value");
    UTEST_ASSERT_TRUE(ctx2->has_var("shared_var"));
    
    auto result = ctx2->format("Value: {shared_var}");
    UTEST_ASSERT_STR_EQUALS(result, "Value: shared_value");
    
    // Different named contexts should be separate
    auto ctx3 = ufmt::get_shared_context("other");
    UTEST_ASSERT_FALSE(ctx3->has_var("shared_var"));
}

// Test new shared context creation
UTEST_FUNC_DEF(CreateSharedContext) {
    auto ctx1 = ufmt::create_shared_context();
    auto ctx2 = ufmt::create_shared_context();
    
    // Should be different contexts
    ctx1->set_var("ctx1_var", "value1");
    ctx2->set_var("ctx2_var", "value2");
    
    UTEST_ASSERT_TRUE(ctx1->has_var("ctx1_var"));
    UTEST_ASSERT_FALSE(ctx1->has_var("ctx2_var"));
    
    UTEST_ASSERT_TRUE(ctx2->has_var("ctx2_var"));
    UTEST_ASSERT_FALSE(ctx2->has_var("ctx1_var"));
}

// Test main format function (uses internal context without variables)
UTEST_FUNC_DEF(FormatVariants) {
    // Test main format function with positional arguments
    auto result1 = ufmt::format("Value: {0}, Count: {1}", "test", 42);
    UTEST_ASSERT_STR_EQUALS(result1, "Value: test, Count: 42");
    
    // Test format specifications
    auto result2 = ufmt::format("Pi: {0:.2f}, Hex: 0x{1:x}", 3.14159, 255);
    UTEST_ASSERT_STR_EQUALS(result2, "Pi: 3.14, Hex: 0xff");
    
    // Test local context with variables (isolated)
    auto ctx = ufmt::create_local_context();
    ctx->set_var("local_var", "local_value");
    auto result3 = ctx->format("Local: {local_var}, Pos: {0}", "arg");
    UTEST_ASSERT_STR_EQUALS(result3, "Local: local_value, Pos: arg");
    
    // Test shared context with variables (shared)
    auto shared_ctx = ufmt::get_shared_context("test");
    shared_ctx->set_var("shared_var", "shared_value");
    auto result4 = shared_ctx->format("Shared: {shared_var}, Pos: {0}", "arg");
    UTEST_ASSERT_STR_EQUALS(result4, "Shared: shared_value, Pos: arg");
}

// Test type conversions
UTEST_FUNC_DEF(TypeConversions) {
    auto ctx = ufmt::create_local_context();
    
    // Test various numeric types
    ctx->set_var("int_val", 42);
    ctx->set_var("long_val", 123456789L);
    ctx->set_var("float_val", 3.14f);
    ctx->set_var("double_val", 2.71828);
    ctx->set_var("bool_val", true);
    ctx->set_var("char_val", 'A');
    
    auto result = ctx->format("int: {int_val}, long: {long_val}, float: {float_val}, "
                            "double: {double_val}, bool: {bool_val}, char: {char_val}");
    
    // Verify that all types are properly converted to strings
    UTEST_ASSERT_STR_CONTAINS(result, "int: 42");
    UTEST_ASSERT_STR_CONTAINS(result, "long: 123456789");
    UTEST_ASSERT_STR_CONTAINS(result, "float: 3.14");
    UTEST_ASSERT_STR_CONTAINS(result, "double: 2.71828");
    UTEST_ASSERT_STR_CONTAINS(result, "bool: true");
    UTEST_ASSERT_STR_CONTAINS(result, "char: A");
}

// Test edge cases
UTEST_FUNC_DEF(EdgeCases) {
    // Empty format string
    auto result1 = ufmt::format("");
    UTEST_ASSERT_STR_EQUALS(result1, "");
    
    // No arguments
    auto result2 = ufmt::format("Hello World");
    UTEST_ASSERT_STR_EQUALS(result2, "Hello World");
    
    // Malformed placeholders (should be left as-is)
    auto result3 = ufmt::format("Incomplete {0 placeholder", "test");
    UTEST_ASSERT_STR_EQUALS(result3, "Incomplete {0 placeholder");
    
    // Missing arguments (should leave placeholder as-is)
    auto result4 = ufmt::format("Missing {1}", "only_arg0");
    UTEST_ASSERT_STR_EQUALS(result4, "Missing {1}");
    
    // Special characters in template
    auto result5 = ufmt::format("Special chars: {} {{}} {{{0}}}", "test");
    UTEST_ASSERT_STR_CONTAINS(result5, "test");
}

// Custom type test
struct Point {
    int x, y;
    Point(int x, int y) : x(x), y(y) {}
};

// Extend ufmt::to_string for custom type
namespace ufmt {
    template<>
    std::string to_string<Point>(const Point& p) {
        return "(" + std::to_string(p.x) + ", " + std::to_string(p.y) + ")";
    }
}

// Also provide a stream operator to avoid compilation issues
std::ostream& operator<<(std::ostream& os, const Point& p) {
    os << "(" << p.x << ", " << p.y << ")";
    return os;
}

UTEST_FUNC_DEF(CustomTypes) {
    auto ctx = ufmt::create_local_context();
    
    Point p(10, 20);
    ctx->set_var("position", p);
    
    auto result = ctx->format("Current position: {position}");
    UTEST_ASSERT_STR_EQUALS(result, "Current position: (10, 20)");
    
    // Also test with positional arguments
    auto result2 = ufmt::format("Point coordinates: {0}", p);
    UTEST_ASSERT_STR_EQUALS(result2, "Point coordinates: (10, 20)");
}

// Test formatted variables with format specifications
UTEST_FUNC_DEF(FormattedVariables) {
    auto ctx = ufmt::create_local_context();
    
    // Set variables of different types
    ctx->set_var("pi", 3.14159265);
    ctx->set_var("count", 42);
    ctx->set_var("hex_value", 255);
    ctx->set_var("name", "Alice");
    ctx->set_var("score", 87.543);
    
    // Test floating point formatting
    auto result1 = ctx->format("Pi to 2 decimal places: {pi:.2f}");
    UTEST_ASSERT_STR_EQUALS(result1, "Pi to 2 decimal places: 3.14");
    
    // Test floating point formatting with different precision
    auto result2 = ctx->format("Score: {score:.1f}");
    UTEST_ASSERT_STR_EQUALS(result2, "Score: 87.5");
    
    // Test integer hex formatting
    auto result3 = ctx->format("Hex value: 0x{hex_value:x}");
    UTEST_ASSERT_STR_EQUALS(result3, "Hex value: 0xff");
    
    // Test integer with padding
    auto result4 = ctx->format("Count with padding: {count:08d}");
    UTEST_ASSERT_STR_EQUALS(result4, "Count with padding: 00000042");
    
    // Test string formatting with width
    auto result5 = ctx->format("Name: '{name:10}'");
    UTEST_ASSERT_STR_EQUALS(result5, "Name: '     Alice'");
    
    // Test string formatting with left justification
    auto result6 = ctx->format("Name: '{name:-10}'");
    UTEST_ASSERT_STR_EQUALS(result6, "Name: 'Alice     '");
    
    // Test mixed formatted and unformatted variables
    auto result7 = ctx->format("User {name} has score {score:.1f} out of {count}");
    UTEST_ASSERT_STR_EQUALS(result7, "User Alice has score 87.5 out of 42");
    
    // Test multiple format specifications in one string
    auto result8 = ctx->format("Pi: {pi:.3f}, Hex: 0x{hex_value:X}, Count: {count:04d}");
    UTEST_ASSERT_STR_EQUALS(result8, "Pi: 3.142, Hex: 0xFF, Count: 0042");
}

// Test numeric formatting with center justification (demo case)
UTEST_FUNC_DEF(NumericCenterFormatting) {
    auto ctx = ufmt::create_local_context();
    
    // Set up the same data as in the demo
    ctx->set_var("score1", 95.7);
    ctx->set_var("score2", 87.2);
    ctx->set_var("score3", 92.8);
    ctx->set_var("score4", 78.5);
    ctx->set_var("score5", 99.1);
    
    // Test center formatting of floating point numbers
    auto result1 = ctx->format("{score1:^5.1f}");
    UTEST_ASSERT_STR_EQUALS(result1, "95.7 ");  // "95.7" centered in field width 5 (length 4, so 1 space after)
    
    auto result2 = ctx->format("{score2:^6.1f}");
    UTEST_ASSERT_STR_EQUALS(result2, " 87.2 ");  // "87.2" centered in field width 6 (length 4, so 1 space before and after)
    
    // Test with different precisions
    auto result3 = ctx->format("{score1:^8.2f}");
    UTEST_ASSERT_STR_EQUALS(result3, " 95.70  ");  // "95.70" centered in field width 8
    
    // Test left alignment
    auto result4 = ctx->format("{score1:-5.1f}");
    UTEST_ASSERT_STR_EQUALS(result4, "95.7 ");  // "95.7" left-aligned in field width 5
    
    // Test right alignment (default)
    auto result5 = ctx->format("{score1:5.1f}");
    UTEST_ASSERT_STR_EQUALS(result5, " 95.7");  // "95.7" right-aligned in field width 5
}

// Test left justification specifically
UTEST_FUNC_DEF(LeftJustification) {
    auto ctx = ufmt::create_local_context();
    
    ctx->set_var("name", "Alice");
    ctx->set_var("number", 42);
    ctx->set_var("decimal", 3.14);
    
    // String left justification
    auto result1 = ctx->format("Name: '{name:-10}'");
    UTEST_ASSERT_STR_EQUALS(result1, "Name: 'Alice     '");
    
    // Numeric left justification
    auto result2 = ctx->format("Number: '{number:-8}'");
    UTEST_ASSERT_STR_EQUALS(result2, "Number: '42      '");
    
    // Decimal left justification
    auto result3 = ctx->format("Decimal: '{decimal:-10}'");
    UTEST_ASSERT_STR_EQUALS(result3, "Decimal: '3.140000  '");
}

// Test right justification specifically
UTEST_FUNC_DEF(RightJustification) {
    auto ctx = ufmt::create_local_context();
    
    ctx->set_var("name", "Bob");
    ctx->set_var("number", 123);
    ctx->set_var("decimal", 2.71);
    
    // String right justification (default)
    auto result1 = ctx->format("Name: '{name:10}'");
    UTEST_ASSERT_STR_EQUALS(result1, "Name: '       Bob'");
    
    // Numeric right justification
    auto result2 = ctx->format("Number: '{number:8}'");
    UTEST_ASSERT_STR_EQUALS(result2, "Number: '     123'");
    
    // Decimal right justification
    auto result3 = ctx->format("Decimal: '{decimal:10}'");
    UTEST_ASSERT_STR_EQUALS(result3, "Decimal: '  2.710000'");
}

// Test center justification specifically
UTEST_FUNC_DEF(CenterJustification) {
    auto ctx = ufmt::create_local_context();
    
    ctx->set_var("name", "Tom");
    ctx->set_var("number", 7);
    ctx->set_var("decimal", 1.5);
    
    // String center justification
    auto result1 = ctx->format("Name: '{name:^10}'");
    UTEST_ASSERT_STR_EQUALS(result1, "Name: '   Tom    '");
    
    // Numeric center justification
    auto result2 = ctx->format("Number: '{number:^8}'");
    UTEST_ASSERT_STR_EQUALS(result2, "Number: '   7    '");
    
    // Decimal center justification
    auto result3 = ctx->format("Decimal: '{decimal:^10}'");
    UTEST_ASSERT_STR_EQUALS(result3, "Decimal: ' 1.500000 '");
    
    // Test even vs odd padding
    auto result4 = ctx->format("Even: '{name:^9}'");
    UTEST_ASSERT_STR_EQUALS(result4, "Even: '   Tom   '");
}

// Test truncation specifically
UTEST_FUNC_DEF(StringTruncation) {
    auto ctx = ufmt::create_local_context();
    
    ctx->set_var("short", "Hi");
    ctx->set_var("medium", "Hello World");
    ctx->set_var("long", "This is a very long string that needs truncation");
    
    // Basic truncation with ellipsis (explicit .precision)
    auto result1 = ctx->format("Long: '{long:.10}'");
    UTEST_ASSERT_STR_EQUALS(result1, "Long: 'This is...'");
    
    // Truncation without ellipsis (very short .precision)
    auto result2 = ctx->format("Short: '{long:.3}'");
    UTEST_ASSERT_STR_EQUALS(result2, "Short: 'Thi'");
    
    // Truncation with width and alignment
    auto result3 = ctx->format("Aligned: '{long:-15.12}'");
    UTEST_ASSERT_STR_EQUALS(result3, "Aligned: 'This is a...   '");
    
    // No truncation when no .precision specified (just width)
    auto result4 = ctx->format("No trunc: '{long:15}'");
    UTEST_ASSERT_STR_CONTAINS(result4, "This is a very long string that needs truncation");
    
    // No truncation needed when string is shorter than precision
    auto result5 = ctx->format("Normal: '{short:.10}'");
    UTEST_ASSERT_STR_EQUALS(result5, "Normal: 'Hi'");
    
    // Width only (no truncation)
    auto result6 = ctx->format("Width only: '{medium:20}'");
    UTEST_ASSERT_STR_EQUALS(result6, "Width only: '         Hello World'");
}

// Test error handling and edge cases
UTEST_FUNC_DEF(ErrorHandling) {
    auto ctx = ufmt::create_local_context();
    
    // Test malformed placeholders (should be gracefully handled)
    auto result1 = ufmt::format("Incomplete {0 placeholder", "test");
    UTEST_ASSERT_STR_EQUALS(result1, "Incomplete {0 placeholder");
    
    // Test missing variables (should leave placeholder as-is)
    auto result2 = ctx->format("Missing variable: {nonexistent}");
    UTEST_ASSERT_STR_EQUALS(result2, "Missing variable: {nonexistent}");
    
    // Test missing positional arguments (should leave placeholder as-is)
    auto result3 = ufmt::format("Missing argument: {1}", "only_arg0");
    UTEST_ASSERT_STR_EQUALS(result3, "Missing argument: {1}");
    
    // Test empty format string
    auto result4 = ufmt::format("");
    UTEST_ASSERT_STR_EQUALS(result4, "");
    
    // Test format string with no placeholders
    auto result5 = ufmt::format("No placeholders here");
    UTEST_ASSERT_STR_EQUALS(result5, "No placeholders here");
    
    // Test invalid format specifications (should use default formatting)
    auto result6 = ufmt::format("Invalid spec: {0:invalid}", 42);
    UTEST_ASSERT_STR_CONTAINS(result6, "42");  // Should still contain the value
    
    // Test very long strings
    std::string long_string(1000, 'x');
    auto result7 = ufmt::format("Long: {0}", long_string);
    UTEST_ASSERT_STR_CONTAINS(result7, "Long: ");
    UTEST_ASSERT_STR_CONTAINS(result7, long_string);
}

int main() {
    UTEST_PROLOG();
    
    UTEST_FUNC(BasicFormatting);
    UTEST_FUNC(FormatSpecifications);
    UTEST_FUNC(LocalContext);
    UTEST_FUNC(CustomFormatters);
    UTEST_FUNC(SharedContext);
    UTEST_FUNC(CreateSharedContext);
    UTEST_FUNC(FormatVariants);
    UTEST_FUNC(TypeConversions);
    UTEST_FUNC(EdgeCases);
    UTEST_FUNC(CustomTypes);
    UTEST_FUNC(FormattedVariables);
    UTEST_FUNC(NumericCenterFormatting);
    UTEST_FUNC(LeftJustification);
    UTEST_FUNC(RightJustification);
    UTEST_FUNC(CenterJustification);
    UTEST_FUNC(StringTruncation);
    UTEST_FUNC(ErrorHandling);
    
    UTEST_EPILOG();
    return 0;
}

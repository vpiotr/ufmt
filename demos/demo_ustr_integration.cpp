/**
 * @file demo_ustr_integration.cpp
 * @brief Demo showing ufmt integration with ustr.h for universal type conversion
 * 
 * This demo shows how to enable ustr.h integration in ufmt by defining UFMT_USE_USTR.
 * When enabled, ufmt will use ustr::to_string() for all type conversions, allowing
 * automatic handling of complex types, containers, pairs, tuples, and more.
 * 
 * To compile this demo with ustr.h integration:
 * g++ -std=c++11 -DUFMT_USE_USTR -I include -o demos/demo_ustr_integration demos/demo_ustr_integration.cpp
 * 
 * To compile without ustr.h integration (default behavior):
 * g++ -std=c++11 -I include -o demos/demo_ustr_integration demos/demo_ustr_integration.cpp
 */

// Include ustr.h first (only when UFMT_USE_USTR is defined)
#ifdef UFMT_USE_USTR
#include "ustr/ustr.h"
#endif

// Include ufmt.h - will automatically use ustr::to_string() if UFMT_USE_USTR is defined
#include "ufmt/ufmt.h"

#include <iostream>
#include <vector>
#include <map>
#include <utility>

// Custom class to demonstrate ustr integration
struct Point {
    double x, y;
    Point(double x, double y) : x(x), y(y) {}
};

namespace ufmt {
    template<>
    std::string to_string<Point>(const Point& p) {
        return "Point(" + std::to_string(p.x) + ", " + std::to_string(p.y) + ")";
    }
}

int main() {
    std::cout << "=== ufmt with ustr.h Integration Demo ===" << std::endl;
    
#ifdef UFMT_USE_USTR
    std::cout << "Status: ustr.h integration ENABLED" << std::endl;
#else
    std::cout << "Status: ustr.h integration DISABLED (using default ufmt converters)" << std::endl;
#endif
    std::cout << std::endl;
    
    // 1. Basic types work the same way
    std::cout << "1. Basic Types:" << std::endl;
    std::cout << "   " << ufmt::format("Integer: {0}, Float: {1}, Bool: {2}", 42, 3.14, true) << std::endl;
    std::cout << "   " << ufmt::format("Char: '{0}', String: '{1}'", 'A', std::string("hello")) << std::endl;
    std::cout << std::endl;
    
    // 2. Custom object
    std::cout << "2. Custom Object (Point):" << std::endl;
    Point p{10.5, 20.3};
#ifdef UFMT_USE_USTR
    std::cout << "   " << ufmt::format("Point: {0}", p) << std::endl;
#else
    // Without ustr.h, format the Point using the specialized to_string function
    std::cout << "   " << ufmt::format("Point: {0}", ufmt::to_string(p)) << std::endl;
#endif
    std::cout << std::endl;
    
#ifdef UFMT_USE_USTR
    // 3. When ustr.h is available, we can format containers directly
    std::cout << "3. Container Support (with ustr.h):" << std::endl;
    std::vector<int> numbers{1, 2, 3, 4, 5};
    std::cout << "   " << ufmt::format("Vector: {0}", numbers) << std::endl;
    
    std::map<std::string, int> scores{{"Alice", 95}, {"Bob", 87}, {"Charlie", 92}};
    std::cout << "   " << ufmt::format("Map: {0}", scores) << std::endl;
    
    std::pair<std::string, int> result{"Success", 200};
    std::cout << "   " << ufmt::format("Pair: {0}", result) << std::endl;
    std::cout << std::endl;
    
    // 4. Complex nested structures
    std::cout << "4. Complex Nested Structures (with ustr.h):" << std::endl;
    std::vector<Point> points{{1, 2}, {3, 4}, {5, 6}};
    std::cout << "   " << ufmt::format("Points: {0}", points) << std::endl;
    
    std::vector<std::pair<std::string, Point>> namedPoints{
        {"Origin", {0, 0}}, 
        {"Center", {50, 50}}, 
        {"Corner", {100, 100}}
    };
    std::cout << "   " << ufmt::format("Named Points: {0}", namedPoints) << std::endl;
    std::cout << std::endl;
#else
    // 3. Without ustr.h, demonstrate that basic functionality still works
    std::cout << "3. Without ustr.h (basic functionality):" << std::endl;
    std::cout << "   - Containers require custom formatters" << std::endl;
    std::cout << "   - Custom types work with manual specialization" << std::endl;
    std::cout << "   - All basic formatting features available" << std::endl;
    std::cout << std::endl;
#endif
    
    // 5. Context features work the same way regardless of ustr.h
    std::cout << "5. Context Features (work with/without ustr.h):" << std::endl;
    auto ctx = ufmt::create_local_context();
    ctx->set_var("app_name", "MyApp");
    ctx->set_var("version", "1.0.0");
    ctx->set_var("user", "Developer");
    std::string message = ctx->format("Welcome to {app_name} v{version}, {user}!");
    std::cout << "   " << message << std::endl;
    
    // Custom formatter for Points in context
    ctx->set_formatter<Point>([](const Point& p) {
        char buf[64];
        snprintf(buf, sizeof(buf), "[%f,%f]", p.x, p.y);
        return std::string(buf);
    });
    std::cout << "   " << ctx->format("Point with custom formatter: {0}", p) << std::endl;
    std::string point_str = "Point(" + std::to_string(p.x) + ", " + std::to_string(p.y) + ")";
    std::cout << "   " << ctx->format("Point with manual conversion: {0}", point_str) << std::endl;
    std::cout << std::endl;
    
    // 6. Formatting specifications
    std::cout << "6. Format Specifications:" << std::endl;
    std::cout << "   " << ufmt::format("Formatted: {0:.2f}, {1:08x}, {2:-10}", 3.14159, 255, "left") << std::endl;
    std::cout << std::endl;
    
    std::cout << "=== Demo Complete ===" << std::endl;
    std::cout << std::endl;
    
#ifdef UFMT_USE_USTR
    std::cout << "To run without ustr.h integration:" << std::endl;
    std::cout << "g++ -std=c++11 -I include -o demos/demo_ustr_integration demos/demo_ustr_integration.cpp" << std::endl;
#else
    std::cout << "To run with ustr.h integration:" << std::endl;
    std::cout << "g++ -std=c++11 -DUFMT_USE_USTR -I include -o demos/demo_ustr_integration demos/demo_ustr_integration.cpp" << std::endl;
#endif
    
    return 0;
}

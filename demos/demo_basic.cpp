#include "../include/ufmt/ufmt.h"
#include <iostream>

int main() {
    std::cout << "=== ufmt Library Demo ===" << std::endl;
    
    // Basic formatting
    std::cout << "\n1. Basic Formatting:" << std::endl;
    std::string msg1 = ufmt::format("User {0} has {1} messages", "Alice", 5);
    std::cout << msg1 << std::endl;
    
    // Format specifications
    std::cout << "\n2. Format Specifications:" << std::endl;
    std::string msg2 = ufmt::format("Pi = {0:.3f}, Hex = 0x{1:x}", 3.14159, 255);
    std::cout << msg2 << std::endl;
    
    // Scoped context with custom formatter
    std::cout << "\n3. Scoped Context with Custom Formatter:" << std::endl;
    auto ctx = ufmt::create_local_context();
    ctx->set_formatter<bool>([](bool b) { return b ? "YES" : "NO"; });
    std::string msg3 = ctx->format("Rendering enabled: {0}", true);
    std::cout << msg3 << std::endl;
    
    // Scoped context with variables
    std::cout << "\n4. Named Variables:" << std::endl;
    ctx->set_var("user", "Bob");
    ctx->set_var("score", 87.5);
    ctx->set_var("level", 12);
    std::string msg4 = ctx->format("Player {user} reached level {level} with score {score:.1f}");
    std::cout << msg4 << std::endl;
    
    // Shared context
    std::cout << "\n5. Shared Context:" << std::endl;
    auto shared_ctx = ufmt::get_shared_context("api");
    shared_ctx->set_var("endpoint", "/api/users");
    shared_ctx->set_var("method", "POST");
    shared_ctx->set_var("status", 201);
    std::string msg5 = shared_ctx->format("Request {method} {endpoint} returned status {status}");
    std::cout << msg5 << std::endl;
    
    // Mixed positional and named
    std::cout << "\n6. Mixed Positional and Named:" << std::endl;
    std::string msg6 = ctx->format("Hello {0}, your score is {score} and level is {level}", "Charlie");
    std::cout << msg6 << std::endl;
    
    // String formatting with width
    std::cout << "\n7. String Width and Justification:" << std::endl;
    std::string msg7 = ufmt::format("Name: '{0:10}' | Score: '{1:-8}' | Status: '{2:6}'", 
                                   "Alice", "92.3", "OK");
    std::cout << msg7 << std::endl;
    
    // Table formatting demo
    std::cout << "\n8. Markdown-like Table with Advanced Formatting:" << std::endl;
    auto table_ctx = ufmt::create_local_context();
    
    // Set up table data with various types
    struct TableRow {
        std::string name;
        int id;
        double score;
        std::string status;
        std::string description;
    };
    
    std::vector<TableRow> data = {
        {"Alice Johnson", 1001, 95.7, "Active", "Senior Software Engineer with 10+ years experience"},
        {"Bob", 2, 87.2, "On Leave", "Junior Developer"},
        {"Catherine Smith-Williams", 1003, 92.8, "Active", "Team Lead"},
        {"David", 404, 78.5, "Inactive", "Short description"},
        {"Elizabeth Alexandra Mary", 5555, 99.1, "Promoted", "Distinguished Engineer and Architecture Specialist"}
    };
    
    // Table header
    std::cout << "| Name                | ID   | Score | Status   | Description           |" << std::endl;
    std::cout << "|---------------------|------|-------|----------|-----------------------|" << std::endl;
    
    // Table rows with different formatting styles
    for (const auto& row : data) {
        table_ctx->set_var("name", row.name);
        table_ctx->set_var("id", row.id);
        table_ctx->set_var("score", row.score);
        table_ctx->set_var("status", row.status);
        table_ctx->set_var("desc", row.description);
        
        // Format each column with different justification and truncation:
        // - name: left justified, max 19 chars (with ellipsis)
        // - id: right justified, 4 chars
        // - score: center justified, 1 decimal place, 5 chars
        // - status: center justified, max 8 chars
        // - description: left justified, max 21 chars (with ellipsis)
        std::string table_row = table_ctx->format(
            "| {name:-19.19} | {id:4} | {score:^5.1f} | {status:^8.8} | {desc:-21.21} |"
        );
        std::cout << table_row << std::endl;
    }
    
    std::cout << "\nFormatting breakdown:" << std::endl;
    std::cout << "- {name:-19.19}   : Left justify, width 19, truncate to 19 chars with ellipsis" << std::endl;
    std::cout << "- {id:4}         : Right justify, width 4" << std::endl;
    std::cout << "- {score:^5.1f}  : Center justify, width 5, 1 decimal place" << std::endl;
    std::cout << "- {status:^8.8}  : Center justify, width 8, truncate to 8 chars" << std::endl;
    std::cout << "- {desc:-21.21}  : Left justify, width 21, truncate to 21 chars with ellipsis" << std::endl;
    
    std::cout << "\n=== Demo Complete ===" << std::endl;
    return 0;
}

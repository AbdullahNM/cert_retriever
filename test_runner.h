#pragma once
#include <iostream>
#include <string>
#include <functional>
#include <vector>
#include <stdexcept>
#include <sstream>

namespace test_detail {
    struct TestCase {
        std::string name;
        std::function<void()> fn;
    };
    inline std::vector<TestCase>& registry() {
        static std::vector<TestCase> reg;
        return reg;
    }
    struct Registrar {
        Registrar(const std::string& name, std::function<void()> fn) {
            registry().push_back({name, fn});
        }
    };
}

#define TEST(name) \
    void test_##name(); \
    static test_detail::Registrar reg_##name(#name, test_##name); \
    void test_##name()

#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            std::ostringstream _msg; \
            _msg << "FAILED: " << #cond << " (line " << __LINE__ << ")"; \
            throw std::runtime_error(_msg.str()); \
        } \
    } while(0)

#define ASSERT_EQ(a, b) \
    do { \
        auto _a = (a); auto _b = (b); \
        if (_a != _b) { \
            std::ostringstream _msg; \
            _msg << "FAILED: " << #a << " == " << #b << " (" << _a << " != " << _b << ") (line " << __LINE__ << ")"; \
            throw std::runtime_error(_msg.str()); \
        } \
    } while(0)

#define ASSERT_NE(a, b) \
    do { \
        auto _a = (a); auto _b = (b); \
        if (_a == _b) { \
            std::ostringstream _msg; \
            _msg << "FAILED: " << #a << " != " << #b << " (both " << _a << ") (line " << __LINE__ << ")"; \
            throw std::runtime_error(_msg.str()); \
        } \
    } while(0)

inline int run_all_tests() {
    int passed = 0, failed = 0;
    for (auto& t : test_detail::registry()) {
        try {
            t.fn();
            std::cout << "  [PASS] " << t.name << std::endl;
            passed++;
        } catch (const std::exception& e) {
            std::cout << "  [FAIL] " << t.name << " - " << e.what() << std::endl;
            failed++;
        }
    }
    std::cout << "\nResults: " << passed << " passed, " << failed << " failed" << std::endl;
    return failed > 0 ? 1 : 0;
}

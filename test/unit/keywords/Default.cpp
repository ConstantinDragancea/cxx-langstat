// RUN: %clangxx %s -emit-ast -std=c++20 -o %t1.ast
// RUN: %cxx-langstat --analyses=mka -emit-features -in %t1.ast -out %t1.ast.json --
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %t1.ast.json
// RUN: diff %t1.ast.json %s.json
// RUN: rm %t1.ast.json || true

struct Dummy {
};

struct Dummy_default {
    Dummy_default() = default;
    ~Dummy_default() = default;
};

struct DefaultBase {
    DefaultBase() = default;
    DefaultBase(const DefaultBase&) = default;
    DefaultBase(DefaultBase&&) = default;
    DefaultBase& operator=(const DefaultBase&) = default;
    
    friend bool operator==(const DefaultBase&, const DefaultBase&);
    bool operator!=(const DefaultBase&) const = default;
};

bool operator==(const DefaultBase&, const DefaultBase&) = default;
#ifndef GAZER_SUPPORT_SEXPR_H
#define GAZER_SUPPORT_SEXPR_H

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <variant>
#include <vector>

namespace gazer::sexpr
{

class Atom;
class List;

class Value
{
    using VariantTy = std::variant<std::string, std::vector<std::unique_ptr<Value>>>;
public:
    explicit Value(std::string str)
        : mData(str)
    {}

    explicit Value(std::vector<std::unique_ptr<Value>> vec)
        : mData(std::move(vec))
    {}

    Value(const Value&) = delete;
    Value& operator=(const Value&) = delete;

    [[nodiscard]] bool isAtom() const { return mData.index() == 0; }
    [[nodiscard]] bool isList() const { return mData.index() == 1; }

    [[nodiscard]] llvm::StringRef asAtom() const { return std::get<0>(mData); }
    [[nodiscard]] const std::vector<std::unique_ptr<Value>>& asList() const { return std::get<1>(mData); }

    bool operator==(const Value& rhs) const;

    void print(llvm::raw_ostream& os) const;

private:
    VariantTy mData;
};

std::unique_ptr<Value> atom(llvm::StringRef data);
std::unique_ptr<Value> list(std::vector<std::unique_ptr<Value>> data);

std::unique_ptr<Value> parse(llvm::StringRef input);

}

#endif

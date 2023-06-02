
//
// ALOX-CC
//

#include "val_array.hh"
#include "memory.hh"
#include <memory>
#include <vector>

namespace alox {

size_t ValueArray::write(const Value &value) {
    auto result = std::find_if(values.begin(), values.end(),
                               [&value](auto &x) { return valuesEqual(x, value); });
    if (result != values.end()) {
        return std::distance(values.begin(), result);
    }
    values.push_back(value);
    return values.size() - 1;
}

void ValueArray::mark() {
    for (auto &v : values) {
        gc.markValue(v);
    }
}

} // namespace lox
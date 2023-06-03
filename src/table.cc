//
// ALOX-CC
//

#include "table.hh"
#include "memory.hh"
#include "object.hh"
#include "value.hh"

namespace alox {

inline constexpr auto TABLE_MAX_LOAD = 0.75;

void Table::free() {
    delete[] this->entries;
    this->entries = nullptr;
    this->capacity = 0;
}

// NOTE: The "Optimization" chapter has a manual copy of this function.
// If you change it here, make sure to update that copy.
Entry *findEntry(Entry *entries, size_t capacity, ObjString *key) {
    uint32_t index = key->hash & (capacity - 1);
    Entry   *tombstone = nullptr;

    for (;;) {
        Entry *entry = &entries[index];
        if (entry->key == nullptr) {
            if (is<nullptr_t>(entry->value)) {
                // Empty entry.
                return tombstone != nullptr ? tombstone : entry;
            }
            // We found a tombstone.
            if (tombstone == nullptr) {
                tombstone = entry;
            }

        } else if (entry->key->str == key->str) {
            // We found the key.
            return entry;
        }

        index = (index + 1) & (capacity - 1);
    }
}

bool Table::get(ObjString *key, Value *value) {
    if (this->count == 0) {
        return false;
    }

    Entry *entry = findEntry(this->entries, this->capacity, key);
    if (entry->key == nullptr) {
        return false;
    }

    *value = entry->value;
    return true;
}

void Table::adjustCapacity(size_t capacity) {
    auto *entries = new Entry[capacity];
    for (int i = 0; i < capacity; i++) {
        entries[i].key = nullptr;
        entries[i].value = NIL_VAL;
    }

    this->count = 0;
    for (int i = 0; i < this->capacity; i++) {
        Entry *entry = &this->entries[i];
        if (entry->key == nullptr) {
            continue;
        }

        Entry *dest = findEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        this->count++;
    }

    delete[] this->entries;
    this->entries = entries;
    this->capacity = capacity;
}

bool Table::set(ObjString *key, Value value) {
    if (this->count + 1 > this->capacity * TABLE_MAX_LOAD) {
        auto capacity = grow_capacity(this->capacity);
        adjustCapacity(capacity);
    }

    Entry     *entry = findEntry(this->entries, this->capacity, key);
    const bool isNewKey = entry->key == nullptr;
    if (isNewKey && is<nullptr_t>(entry->value)) {
        this->count++;
    }

    entry->key = key;
    entry->value = value;
    return isNewKey;
}

bool Table::del(ObjString *key) {
    if (this->count == 0) {
        return false;
    }

    // Find the entry.
    Entry *entry = findEntry(this->entries, this->capacity, key);
    if (entry->key == nullptr) {
        return false;
    }

    // Place a tombstone in the entry.
    entry->key = nullptr;
    entry->value = value<bool>(true);
    return true;
}

void Table::addAll(const Table &from, Table &to) {
    for (int i = 0; i < from.capacity; i++) {
        Entry *entry = &from.entries[i];
        if (entry->key != nullptr) {
            to.set(entry->key, entry->value);
        }
    }
}

} // namespace alox

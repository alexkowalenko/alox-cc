//
// ALOX-CC
//

#include "table.hh"
#include "memory.hh"
#include "object.hh"
#include "value.hh"

inline constexpr auto TABLE_MAX_LOAD = 0.75;

void Table::free() {
    gc.delete_array<Entry>(this->entries, this->capacity);
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
            if (IS_NIL(entry->value)) {
                // Empty entry.
                return tombstone != nullptr ? tombstone : entry;
            }
            // We found a tombstone.
            if (tombstone == nullptr) {
                tombstone = entry;
            }

        } else if (entry->key == key) {
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
    auto *entries = gc.allocate_array<Entry>(capacity);
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

    gc.delete_array<Entry>(this->entries, this->capacity);
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
    if (isNewKey && IS_NIL(entry->value)) {
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
    entry->value = BOOL_VAL(true);
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

ObjString *Table::findString(const char *chars, int length, uint32_t hash) {
    if (this->count == 0) {
        return nullptr;
    }

    uint32_t index = hash & (this->capacity - 1);
    for (;;) {
        Entry *entry = &this->entries[index];
        if (entry->key == nullptr) {
            // Stop if we find an empty non-tombstone entry.
            if (IS_NIL(entry->value)) {
                return nullptr;
            }
        } else if (entry->key->length == length && entry->key->hash == hash &&
                   memcmp(entry->key->chars, chars, length) == 0) {
            // We found it.
            return entry->key;
        }

        index = (index + 1) & (this->capacity - 1);
    }
}

void Table::removeWhite() {
    for (int i = 0; i < this->capacity; i++) {
        Entry *entry = &this->entries[i];
        if (entry->key != nullptr && !entry->key->obj.isMarked) {
            del(entry->key);
        }
    }
}

void Table::mark() {
    for (int i = 0; i < this->capacity; i++) {
        Entry *entry = &this->entries[i];
        gc.markObject((Obj *)entry->key);
        gc.markValue(entry->value);
    }
}

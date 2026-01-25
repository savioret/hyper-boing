#include "oncehelper.h"

bool OnceHelper::once(const std::string& key)
{
    // Check if key exists in the set
    if (fired.find(key) == fired.end())
    {
        // First time - insert and return true
        fired.insert(key);
        return true;
    }

    // Already fired - return false
    return false;
}

void OnceHelper::reset(const std::string& key)
{
    // Remove the key so once() will return true again
    fired.erase(key);
}

void OnceHelper::clear()
{
    // Clear all fired keys
    fired.clear();
}

bool OnceHelper::hasFired(const std::string& key) const
{
    // Read-only check without modifying state
    return fired.find(key) != fired.end();
}

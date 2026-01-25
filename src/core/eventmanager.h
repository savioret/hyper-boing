#pragma once

#include <vector>
#include <functional>
#include <memory>
#include "gameevent.h"

/**
 * Event listener callback type
 * Takes const reference to event data
 */
using EventListener = std::function<void(const GameEventData&)>;

/**
 * EventManager - Simple global event dispatcher
 *
 * Singleton providing pub/sub event system for gameplay events.
 * Follows the console command pattern with std::function listeners.
 *
 * Features:
 *   - Subscribe to specific event types
 *   - Immediate dispatch (no queuing - keeps it simple)
 *   - Automatic unsubscribe on listener destruction (via ListenerHandle)
 *   - Optional debug logging of all events
 *
 * Usage:
 *   // Subscribe
 *   auto handle = EVENT_MGR.subscribe(GameEventType::BALL_HIT,
 *       [](const GameEventData& data) {
 *           LOG_INFO("Ball hit!");
 *       });
 *
 *   // Fire event
 *   GameEventData event;
 *   event.type = GameEventType::BALL_HIT;
 *   event.ballHit.ball = ball;
 *   event.ballHit.shot = shot;
 *   event.ballHit.shooter = shot->getPlayer();
 *   EVENT_MGR.trigger(event);
 *
 * IMPORTANT: Event listeners should be READ-ONLY operations.
 * Do not modify game state from listeners (no killing objects, no spawning).
 * Safe operations: logging, playing sounds, updating UI, tracking statistics.
 */
class EventManager
{
public:
    /**
     * RAII handle for automatic unsubscription
     * When destroyed, automatically unsubscribes the listener
     */
    class ListenerHandle
    {
    private:
        int subscriptionId;
        EventManager* manager;

        friend class EventManager;

        ListenerHandle(int id, EventManager* mgr)
            : subscriptionId(id), manager(mgr) {}

    public:
        // Default constructor for uninitialized handles
        ListenerHandle()
            : subscriptionId(-1), manager(nullptr) {}

        // Destructor auto-unsubscribes
        ~ListenerHandle()
        {
            if (manager && subscriptionId >= 0)
                manager->unsubscribe(subscriptionId);
        }

        // Move-only (no copies)
        ListenerHandle(const ListenerHandle&) = delete;
        ListenerHandle& operator=(const ListenerHandle&) = delete;

        ListenerHandle(ListenerHandle&& other) noexcept
            : subscriptionId(other.subscriptionId), manager(other.manager)
        {
            other.subscriptionId = -1;
            other.manager = nullptr;
        }

        ListenerHandle& operator=(ListenerHandle&& other) noexcept
        {
            if (this != &other)
            {
                // Unsubscribe current subscription
                if (manager && subscriptionId >= 0)
                    manager->unsubscribe(subscriptionId);

                // Move from other
                subscriptionId = other.subscriptionId;
                manager = other.manager;

                other.subscriptionId = -1;
                other.manager = nullptr;
            }
            return *this;
        }

        // Check if handle is valid
        bool isValid() const
        {
            return manager != nullptr && subscriptionId >= 0;
        }
    };

private:
    static std::unique_ptr<EventManager> s_instance;

    struct Subscription
    {
        int id;                    // Unique subscription ID
        GameEventType eventType;   // Event type to listen for
        EventListener callback;    // Callback function
        bool active;               // For deferred unsubscribe (prevents mid-iteration removal)
    };

    std::vector<Subscription> subscriptions;
    int nextSubscriptionId;
    bool logEvents;  // Debug: log all events to console
    bool firing;     // True when currently firing events (prevents removal during iteration)

    // Private constructor for singleton
    EventManager();

    // Non-copyable
    EventManager(const EventManager&) = delete;
    EventManager& operator=(const EventManager&) = delete;

public:
    /**
     * Singleton accessor
     * @return Reference to the global EventManager instance
     */
    static EventManager& instance();

    /**
     * Destroy the singleton instance
     * Call during application shutdown
     */
    static void destroy();

    /**
     * Subscribe to an event type
     * @param type Event type to listen for
     * @param listener Callback function invoked when event fires
     * @return RAII handle that auto-unsubscribes on destruction
     */
    ListenerHandle subscribe(GameEventType type, EventListener listener);

    /**
     * Unsubscribe a listener
     * @param subscriptionId ID from subscribe() call
     *
     * NOTE: If called during event firing, marks subscription inactive
     * and removes it after firing completes (safe deferred removal)
     */
    void unsubscribe(int subscriptionId);

    /**
     * Fire an event to all subscribers
     * @param eventData Event data with type and type-specific fields
     *
     * NOTE: Fires immediately (not deferred). Safe to call during movement phase
     * because listeners should only read data, not modify game state.
     */
    void trigger(const GameEventData& eventData);

    /**
     * Enable/disable event logging for debugging
     * When enabled, all fired events are logged to console
     * @param enable True to enable logging, false to disable
     */
    void setLogEvents(bool enable);

    /**
     * Get event logging state
     * @return True if event logging is enabled
     */
    bool isLoggingEvents() const { return logEvents; }

    /**
     * Clear all subscriptions
     * Useful for testing or screen transitions
     */
    void clear();

    /**
     * Get number of active subscriptions
     * Useful for debugging
     * @return Count of active subscriptions
     */
    int getSubscriptionCount() const;

private:
    /**
     * Helper: Get event type name for logging
     */
    const char* getEventTypeName(GameEventType type) const;

    /**
     * Helper: Remove inactive subscriptions after firing
     */
    void cleanupInactiveSubscriptions();
};

// Global accessor macro (matches CONSOLE, LOG_* pattern)
#define EVENT_MGR EventManager::instance()

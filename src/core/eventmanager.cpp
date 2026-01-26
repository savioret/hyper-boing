#include "eventmanager.h"
#include "logger.h"
#include <SDL.h>
#include <algorithm>

// Static instance
std::unique_ptr<EventManager> EventManager::s_instance = nullptr;

EventManager::EventManager()
    : nextSubscriptionId(0), logEvents(false), firing(false)
{
}

EventManager& EventManager::instance()
{
    if (!s_instance)
    {
        s_instance = std::unique_ptr<EventManager>(new EventManager());
    }
    return *s_instance;
}

void EventManager::destroy()
{
    s_instance.reset();
}

EventManager::ListenerHandle EventManager::subscribe(GameEventType type, EventListener listener)
{
    Subscription sub;
    sub.id = nextSubscriptionId++;
    sub.eventType = type;
    sub.callback = listener;
    sub.active = true;

    subscriptions.push_back(sub);

    if (logEvents)
    {
        LOG_DEBUG("Event subscription #%d registered for event type: %s",
            sub.id, getEventTypeName(type));
    }

    return ListenerHandle(sub.id, this);
}

void EventManager::unsubscribe(int subscriptionId)
{
    if (firing)
    {
        // Mid-iteration removal - mark inactive for deferred cleanup
        for (Subscription& sub : subscriptions)
        {
            if (sub.id == subscriptionId)
            {
                sub.active = false;
                if (logEvents)
                {
                    LOG_DEBUG("Event subscription #%d marked for removal", subscriptionId);
                }
                return;
            }
        }
    }
    else
    {
        // Safe to remove immediately
        auto it = std::remove_if(subscriptions.begin(), subscriptions.end(),
            [subscriptionId](const Subscription& sub) {
                return sub.id == subscriptionId;
            });

        if (it != subscriptions.end())
        {
            if (logEvents)
            {
                LOG_DEBUG("Event subscription #%d removed", subscriptionId);
            }
            subscriptions.erase(it, subscriptions.end());
        }
    }
}

void EventManager::trigger(const GameEventData& eventData)
{
    if (logEvents)
    {
        LOG_INFO("[EVENT] %s (timestamp=%d)", getEventTypeName(eventData.type), eventData.timestamp);
    }

    firing = true;

    // Invoke all active subscribers for this event type
    for (Subscription& sub : subscriptions)
    {
        if (sub.active && sub.eventType == eventData.type)
        {
            try
            {
                sub.callback(eventData);
            }
            catch (...)
            {
                LOG_ERROR("Exception in event listener for event type: %s",
                    getEventTypeName(eventData.type));
            }
        }
    }

    firing = false;

    // Clean up any subscriptions marked inactive during firing
    cleanupInactiveSubscriptions();
}

void EventManager::setLogEvents(bool enable)
{
    logEvents = enable;
    if (enable)
    {
        LOG_INFO("Event logging enabled");
    }
    else
    {
        LOG_INFO("Event logging disabled");
    }
}

void EventManager::clear()
{
    subscriptions.clear();
    nextSubscriptionId = 0;

    if (logEvents)
    {
        LOG_DEBUG("All event subscriptions cleared");
    }
}

int EventManager::getSubscriptionCount() const
{
    int count = 0;
    for (const Subscription& sub : subscriptions)
    {
        if (sub.active)
            count++;
    }
    return count;
}

const char* EventManager::getEventTypeName(GameEventType type) const
{
    switch (type)
    {
    case GameEventType::LEVEL_CLEAR: return "LEVEL_CLEAR";
    case GameEventType::GAME_OVER: return "GAME_OVER";
    case GameEventType::TIME_SECOND_ELAPSED: return "TIME_SECOND_ELAPSED";
    case GameEventType::STAGE_OBJECT_SPAWNED: return "STAGE_OBJECT_SPAWNED";
    case GameEventType::PLAYER_HIT: return "PLAYER_HIT";
    case GameEventType::PLAYER_REVIVED: return "PLAYER_REVIVED";
    case GameEventType::BALL_HIT: return "BALL_HIT";
    case GameEventType::BALL_SPLIT: return "BALL_SPLIT";
    case GameEventType::PLAYER_SHOOT: return "PLAYER_SHOOT";
    case GameEventType::SCORE_CHANGED: return "SCORE_CHANGED";
    case GameEventType::WEAPON_CHANGED: return "WEAPON_CHANGED";
    case GameEventType::STAGE_STARTED: return "STAGE_STARTED";
    case GameEventType::STAGE_MUSIC_CHANGED: return "STAGE_MUSIC_CHANGED";
    case GameEventType::CONSOLE_COMMAND: return "CONSOLE_COMMAND";
    default: return "UNKNOWN";
    }
}

void EventManager::cleanupInactiveSubscriptions()
{
    auto it = std::remove_if(subscriptions.begin(), subscriptions.end(),
        [](const Subscription& sub) {
            return !sub.active;
        });

    if (it != subscriptions.end())
    {
        subscriptions.erase(it, subscriptions.end());
    }
}

#pragma once

#include "ResourceHandles.h"

#include <string>
#include <unordered_map>

namespace edl {

typedef void (*EventFunction)(edl::Toolchain& toolchain, const std::string& character, float delta); 

enum class EffectAction {
    ADD, // Add rand(min, max) to var
    SUB, // Subtract rand(min, max) from var
    SET, // Set var to rand(min, max), if min and max are set to -1 sets to default?
    SPECIAL // Call the special event that corresponds to var
};

struct Effect {
    EffectAction action; // The action this effect causes
    std::string character; // The character who will be effected, leave as "" for a global effect
    std::string var; // The variable effected / The event triggered
    float min; // The min value of the effect
    float max; // The max value of the effect
};

struct Event {
    std::string name; // The Name of the event
    std::vector<Effect> effects; // A list of effects that will be triggered when the event is triggered (Note: All effects will be triggered)
};

struct SpecialEvent {
    std::string name; // Name of the special event

    float duration; // How long in milliseconds the special event will last, 0 will trigger the start, ongoing, and end functions once and then stop

    EventFunction start; // The function to be called when the event is triggered
    EventFunction ongoing; // The function to be called every frame while the event is active
    EventFunction end; // The function to be calledwhen the event is done
};

struct Card {
    std::string name; // Name of the card
    std::string material; // Name of the material
    std::string defaultEvent; // The default event, will be called if no other event is called
    std::unordered_map<std::string, std::string> eventMap; // A mapping from a character to an event, ie. the event that will be called if this card is given o that character
};

struct Variable {
    std::string name; // Name of the variable
    float defaultValue; // Default value
    float min; // Minimum value, -1 for unbound?
    float max; // Maximum value, -1 for unbound?
    std::string minExceedEvent; // The event triggered if the variable goes below min
    std::string maxExceedEvent; // The event triggered if the variable goes above max
};

struct Character {
    std::string name;
    std::vector<std::string> variables;
};

}
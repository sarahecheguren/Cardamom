#pragma once

#include <json/json.h>

#include <algorithm>
#include <memory>
#include <string>

// Reads and validates the recipe fields of a JSON request body. Adding and
// editing a recipe both use it, so the two share the same rules.
namespace api {

// Optional fields are left empty when they were not provided. The queries
// store an empty value as NULL.
struct RecipeInput {
    std::string name;
    std::string ingredients;
    std::string steps;
    std::string notes;
    std::string imageUrl;
    std::string visibility;
    std::string servings;
    std::string prepMinutes;
    std::string cookMinutes;
};

// Fills `recipe` from `input`. Returns false and sets `error` to a message for
// the user when the body is not a valid recipe.
inline bool readRecipeInput(const std::shared_ptr<Json::Value>& input, RecipeInput& recipe,
                            std::string& error) {
    if (!input || !input->isObject()) {
        error = "Please provide a JSON object.";
        return false;
    }

    auto trim = [](const std::string& value) {
        const auto first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            return std::string();
        }

        const auto last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1);
    };

    if (!(*input)["name"].isString() || !(*input)["ingredient_list"].isString()) {
        error = "Recipe name and ingredients are required.";
        return false;
    }

    recipe.name = trim((*input)["name"].asString());
    recipe.ingredients = trim((*input)["ingredient_list"].asString());

    if (recipe.name.empty() || recipe.name.size() > 200 ||
        recipe.name.find('\0') != std::string::npos) {
        error = "Recipe name is required and must be at most 200 bytes.";
        return false;
    }

    if (recipe.ingredients.empty() || recipe.ingredients.size() > 65535 ||
        recipe.ingredients.find('\0') != std::string::npos) {
        error = "Ingredients are required and must be at most 65535 bytes.";
        return false;
    }

    auto readOptionalText = [&](const char* field, std::string& value) {
        const auto& item = (*input)[field];

        if (item.isNull()) {
            return true;
        }

        if (!item.isString()) {
            return false;
        }

        value = trim(item.asString());
        return value.size() <= 65535 && value.find('\0') == std::string::npos;
    };

    if (!readOptionalText("steps", recipe.steps) || !readOptionalText("notes", recipe.notes)) {
        error = "Steps and notes must be text of at most 65535 bytes each.";
        return false;
    }

    if (!readOptionalText("image_url", recipe.imageUrl)) {
        error = "Image URL must be text.";
        return false;
    }

    if (!recipe.imageUrl.empty()) {
        const auto& url = recipe.imageUrl;
        const auto hostEnd = url.find_first_of("/?#", 8);

        if (url.size() > 2048 || url.compare(0, 8, "https://") != 0 || url.size() <= 8 ||
            hostEnd == 8 ||
            std::any_of(url.begin(), url.end(),
                        [](unsigned char c) { return c <= 32 || c == 127 || c == 92; }) ||
            url.substr(8, hostEnd == std::string::npos ? std::string::npos : hostEnd - 8)
                    .find('@') != std::string::npos) {
            error = "Please enter a valid HTTPS image URL.";
            return false;
        }
    }

    const auto& visibility = (*input)["visibility"];

    if (!visibility.isNull()) {
        if (!visibility.isString() ||
            (visibility.asString() != "private" && visibility.asString() != "public")) {
            error = "Recipe visibility must be private or public.";
            return false;
        }

        recipe.visibility = visibility.asString();
    }

    auto readOptionalNumber = [&](const char* field, unsigned int minimum, std::string& value) {
        const auto& item = (*input)[field];

        if (item.isNull()) {
            return true;
        }

        if (!item.isUInt()) {
            return false;
        }

        const auto number = item.asUInt();
        if (number < minimum || number > 65535) {
            return false;
        }

        value = std::to_string(number);
        return true;
    };

    if (!readOptionalNumber("servings", 1, recipe.servings) ||
        !readOptionalNumber("prep_minutes", 0, recipe.prepMinutes) ||
        !readOptionalNumber("cook_minutes", 0, recipe.cookMinutes)) {
        error =
            "Servings must be an integer from 1 to 65535. "
            "Preparation and cooking times must be integers from 0 to 65535.";
        return false;
    }

    return true;
}

}  // namespace api

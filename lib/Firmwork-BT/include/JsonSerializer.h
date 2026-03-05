//
// Created by Andrew Simmons on 2/25/26.
//

#ifndef FIRMWORK_JSONSERIALIZER_H
#define FIRMWORK_JSONSERIALIZER_H

#include <ArduinoJson.h>
#include <Settings.h>
#include <cmath>

class JsonSerializer
{
public:
    static const char* typeToString(SettingType type)
    {
        switch (type)
        {
            case SettingType::INT:
                return "int";
            case SettingType::FLOAT:
                return "float";
            case SettingType::BOOL:
                return "bool";
            case SettingType::STRING:
                return "string";
        }

        return "unknown";
    }

    static void toJson(const Settings& settings, JsonObject destination)
    {
        const std::vector<SettingField>& fields = settings.getFields();
        for (const SettingField& field : fields)
        {
            switch (field.type)
            {
                case SettingType::INT:
                    destination[field.name] = *static_cast<const int*>(field.ptr);
                    break;
                case SettingType::FLOAT:
                    destination[field.name] = *static_cast<const float*>(field.ptr);
                    break;
                case SettingType::BOOL:
                    destination[field.name] = *static_cast<const bool*>(field.ptr);
                    break;
                case SettingType::STRING:
                    destination[field.name] = *static_cast<const String*>(field.ptr);
                    break;
            }
        }
    }

    static void schemaToJson(const Settings& settings, JsonObject destination)
    {
        const std::vector<SettingField>& fields = settings.getFields();
        for (const SettingField& field : fields)
        {
            JsonObject entry = destination.createNestedObject(field.name);
            entry["type"] = typeToString(field.type);
            if (field.description != nullptr && field.description[0] != '\0')
                entry["description"] = field.description;
        }
    }

    static bool getValue(const Settings& settings, const String& key, JsonVariant destination)
    {
        const SettingField* field = findField(settings, key);
        if (field == nullptr)
            return false;

        switch (field->type)
        {
            case SettingType::INT:
                destination.set(*static_cast<const int*>(field->ptr));
                return true;
            case SettingType::FLOAT:
                destination.set(*static_cast<const float*>(field->ptr));
                return true;
            case SettingType::BOOL:
                destination.set(*static_cast<const bool*>(field->ptr));
                return true;
            case SettingType::STRING:
                destination.set(*static_cast<const String*>(field->ptr));
                return true;
        }

        return false;
    }

    static bool setValue(Settings& settings, const String& key, JsonVariantConst value)
    {
        const SettingField* field = findField(settings, key);
        if (field == nullptr)
            return false;

        bool changed = false;

        switch (field->type)
        {
            case SettingType::INT:
            {
                if (!value.is<int>() && !value.is<long>() && !value.is<float>() && !value.is<double>())
                    return false;

                int* ptr = static_cast<int*>(field->ptr);
                const int parsed = value.as<int>();
                changed = *ptr != parsed;
                *ptr = parsed;
                break;
            }
            case SettingType::FLOAT:
            {
                if (!value.is<float>() && !value.is<double>() && !value.is<int>() && !value.is<long>())
                    return false;

                float* ptr = static_cast<float*>(field->ptr);
                const float parsed = value.as<float>();
                changed = std::fabs(*ptr - parsed) > 0.0001f;
                *ptr = parsed;
                break;
            }
            case SettingType::BOOL:
            {
                if (!value.is<bool>() && !value.is<int>() && !value.is<long>())
                    return false;

                bool* ptr = static_cast<bool*>(field->ptr);
                const bool parsed = value.is<bool>() ? value.as<bool>() : (value.as<int>() != 0);
                changed = *ptr != parsed;
                *ptr = parsed;
                break;
            }
            case SettingType::STRING:
            {
                String* ptr = static_cast<String*>(field->ptr);
                String parsed;

                if (value.is<const char*>())
                    parsed = String(value.as<const char*>());
                else if (value.is<int>() || value.is<long>())
                    parsed = String(value.as<long>());
                else if (value.is<float>() || value.is<double>())
                    parsed = String(value.as<float>(), 4);
                else if (value.is<bool>())
                    parsed = value.as<bool>() ? "true" : "false";
                else
                    return false;

                changed = *ptr != parsed;
                *ptr = parsed;
                break;
            }
        }

        if (changed)
            settings.notifyChanged(key, true);

        return true;
    }

    static bool fromJson(Settings& settings, JsonObjectConst source)
    {
        bool changedAny = false;

        const std::vector<SettingField>& fields = settings.getFields();
        for (const SettingField& field : fields)
        {
            if (!source.containsKey(field.name))
                continue;

            if (setValue(settings, String(field.name), source[field.name]))
                changedAny = true;
        }

        return changedAny;
    }

private:
    static const SettingField* findField(const Settings& settings, const String& key)
    {
        const std::vector<SettingField>& fields = settings.getFields();
        for (const SettingField& field : fields)
        {
            if (key.equals(field.name))
                return &field;
        }

        return nullptr;
    }
};

#endif //FIRMWORK_JSONSERIALIZER_H

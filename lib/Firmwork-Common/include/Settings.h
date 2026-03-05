//
// Created by Andrew Simmons on 2/25/26.
//

#ifndef FIRMWORK_SETTINGS_H
#define FIRMWORK_SETTINGS_H

#include <Arduino.h>
#include <Preferences.h>
#include <cmath>
#include <cstring>
#include <functional>
#include <type_traits>
#include <vector>

enum class SettingType
{
    INT,
    FLOAT,
    BOOL,
    STRING
};

struct SettingField
{
    const char* name;
    const char* description;
    SettingType type;
    void* ptr;
};

class Settings
{
public:
    typedef std::function<void(const String& key)> ChangeCallback;

    template<typename T>
    struct Property
    {
        T value;
        const char* name;
        const char* description;
        Settings* parent;

        Property(Settings* p, const char* n, const T& def, const char* d = "")
            : value(def),
              name(n),
              description(d),
              parent(p)
        {
            static_assert(
                std::is_same<T, int>::value ||
                std::is_same<T, float>::value ||
                std::is_same<T, bool>::value ||
                std::is_same<T, String>::value,
                "SETTING only supports int, float, bool, and String"
            );

            if (this->parent != nullptr)
                this->parent->registerField({this->name, this->description, typeOf(), &this->value});
        }

        operator T&() { return this->value; }
        operator T() const { return this->value; }

        Property& operator=(const T& v)
        {
            this->value = v;
            if (this->parent != nullptr)
                this->parent->notifyChanged(this->name, true);
            return *this;
        }

    private:
        static SettingType typeOf();
    };

    explicit Settings(const char* namespaceName = "settings");
    virtual ~Settings();

    bool begin(const char* namespaceName = nullptr);
    void end();
    bool isOpen() const { return this->preferencesOpen; }

    void saveToPrefs();
    void loadFromPrefs(bool notify = false);

    bool notifyChanged(const String& key, bool persist = true);
    void addOnChangeListener(ChangeCallback callback);

    const std::vector<SettingField>& getFields() const { return this->fields; }

protected:
    void registerField(const SettingField& field);
    virtual void onSettingChanged(const String& key);

private:
    int findFieldIndex(const String& key) const;
    SettingField* findField(const String& key);
    const SettingField* findField(const String& key) const;
    bool persistField(const SettingField& field);
    void notifyListeners(const String& key);

    std::vector<SettingField> fields;
    std::vector<ChangeCallback> globalCallbacks;
    Preferences prefs;
    String ns;
    bool preferencesOpen = false;
};

template<>
inline SettingType Settings::Property<int>::typeOf()
{
    return SettingType::INT;
}

template<>
inline SettingType Settings::Property<float>::typeOf()
{
    return SettingType::FLOAT;
}

template<>
inline SettingType Settings::Property<bool>::typeOf()
{
    return SettingType::BOOL;
}

template<>
inline SettingType Settings::Property<String>::typeOf()
{
    return SettingType::STRING;
}

#define SETTING(type, name, defaultValue, description) \
    Settings::Property<type> name{this, #name, defaultValue, description}

#endif //FIRMWORK_SETTINGS_H

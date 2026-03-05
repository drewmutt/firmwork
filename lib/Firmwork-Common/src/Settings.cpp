//
// Created by Andrew Simmons on 2/25/26.
//

#include <Settings.h>

Settings::Settings(const char* namespaceName)
    : ns(namespaceName == nullptr ? "settings" : namespaceName)
{
}

Settings::~Settings()
{
    this->end();
}

bool Settings::begin(const char* namespaceName)
{
    if (namespaceName != nullptr)
        this->ns = namespaceName;

    if (this->preferencesOpen)
        return true;

    this->preferencesOpen = this->prefs.begin(this->ns.c_str(), false);
    if (this->preferencesOpen)
        this->loadFromPrefs(false);

    return this->preferencesOpen;
}

void Settings::end()
{
    if (!this->preferencesOpen)
        return;

    this->prefs.end();
    this->preferencesOpen = false;
}

void Settings::saveToPrefs()
{
    if (!this->preferencesOpen)
        return;

    for (const SettingField& field : this->fields)
        this->persistField(field);
}

void Settings::loadFromPrefs(bool notify)
{
    if (!this->preferencesOpen)
        return;

    for (const SettingField& field : this->fields)
    {
        bool changed = false;

        switch (field.type)
        {
            case SettingType::INT:
            {
                int* ptr = static_cast<int*>(field.ptr);
                if (!this->prefs.isKey(field.name))
                    continue;
                const int loaded = this->prefs.getInt(field.name);
                changed = *ptr != loaded;
                *ptr = loaded;
                break;
            }
            case SettingType::FLOAT:
            {
                float* ptr = static_cast<float*>(field.ptr);
                if (!this->prefs.isKey(field.name))
                    continue;
                const float loaded = this->prefs.getFloat(field.name);
                changed = std::fabs(*ptr - loaded) > 0.0001f;
                *ptr = loaded;
                break;
            }
            case SettingType::BOOL:
            {
                bool* ptr = static_cast<bool*>(field.ptr);
                if (!this->prefs.isKey(field.name))
                    continue;
                const bool loaded = this->prefs.getBool(field.name);
                changed = *ptr != loaded;
                *ptr = loaded;
                break;
            }
            case SettingType::STRING:
            {
                String* ptr = static_cast<String*>(field.ptr);
                if (!this->prefs.isKey(field.name))
                    continue;
                const String loaded = this->prefs.getString(field.name);
                changed = *ptr != loaded;
                *ptr = loaded;
                break;
            }
        }

        if (notify && changed)
            this->notifyListeners(field.name);
    }
}

bool Settings::notifyChanged(const String& key, bool persist)
{
    SettingField* field = this->findField(key);
    if (field == nullptr)
        return false;

    if (persist && this->preferencesOpen)
        this->persistField(*field);

    this->notifyListeners(key);
    return true;
}

void Settings::addOnChangeListener(ChangeCallback callback)
{
    this->globalCallbacks.push_back(callback);
}

void Settings::registerField(const SettingField& field)
{
    this->fields.push_back(field);
}

void Settings::onSettingChanged(const String& key)
{
    (void)key;
}

int Settings::findFieldIndex(const String& key) const
{
    for (size_t i = 0; i < this->fields.size(); i++)
    {
        if (key.equals(this->fields[i].name))
            return static_cast<int>(i);
    }

    return -1;
}

SettingField* Settings::findField(const String& key)
{
    const int index = this->findFieldIndex(key);
    if (index < 0)
        return nullptr;

    return &this->fields[static_cast<size_t>(index)];
}

const SettingField* Settings::findField(const String& key) const
{
    const int index = this->findFieldIndex(key);
    if (index < 0)
        return nullptr;

    return &this->fields[static_cast<size_t>(index)];
}

bool Settings::persistField(const SettingField& field)
{
    switch (field.type)
    {
        case SettingType::INT:
            return this->prefs.putInt(field.name, *static_cast<const int*>(field.ptr)) == sizeof(int32_t);

        case SettingType::FLOAT:
            return this->prefs.putFloat(field.name, *static_cast<const float*>(field.ptr)) == sizeof(float);

        case SettingType::BOOL:
            return this->prefs.putBool(field.name, *static_cast<const bool*>(field.ptr)) == sizeof(bool);

        case SettingType::STRING:
        {
            const String& value = *static_cast<const String*>(field.ptr);
            return this->prefs.putString(field.name, value) == value.length();
        }
    }

    return false;
}

void Settings::notifyListeners(const String& key)
{
    for (ChangeCallback& callback : this->globalCallbacks)
        callback(key);

    this->onSettingChanged(key);
}

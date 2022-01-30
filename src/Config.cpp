//
// Created by zhangfuwen on 2022/1/27.
//

#include "Config.h"

Config * Config::instance = nullptr;
std::string Config::GetString(const std::string &name) {
    std::string val;
    auto akId = ibus_config_get_value(m_config, CONF_SECTION, name.c_str());
    if (akId != nullptr) {
        auto nameVal = g_variant_get_string(akId, nullptr);
        if (nameVal == nullptr) {
            FUN_ERROR("failed to get variant");
            val = "";
        } else {
            val = nameVal;
            FUN_DEBUG("value:%s", nameVal);
        }
    } else {
        FUN_ERROR("failed to get config value for %s", name.c_str());
        val = "";
        SetString(name, "");
    }
    return val;
}

void Config::SetString(const std::string& name, const std::string& val) {
    auto ret = ibus_config_set_value(m_config, CONF_SECTION, name.c_str(), g_variant_new_string(val.c_str()));
    if (!ret) {
        FUN_ERROR("failed to set config %s", name.c_str());
    }
}

// static
void Config::OnValueChanged(IBusConfig *config, gchar *section, gchar *name, GVariant *value, gpointer user_data) {
    FUN_TRACE("Entry");
        auto * self = (Config*)user_data;
        FUN_DEBUG("section:%s, name:%s", section, name);

    // to make thing easier, all watched configs are of string type
    auto nameVal = g_variant_get_string(value, nullptr);
    if (nameVal == nullptr) {
        FUN_ERROR("failed to get variant");
        return;
    }

    if (std::string(name) == CONF_NAME_ID) {
        self->m_opts->speechAkId = nameVal;
    } else if (std::string(name) == CONF_NAME_SECRET) {
        self->m_opts->speechSecret = nameVal;
    } else if(std::string(name) == CONF_NAME_FAST_INPUT_RELOAD) {
        self->m_opts->dictFastPendingReload = true;
    }
    FUN_TRACE("Exit");
}


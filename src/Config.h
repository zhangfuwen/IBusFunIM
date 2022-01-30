//
// Created by zhangfuwen on 2022/1/27.
//

#ifndef AUDIO_IME_CONFIG_H
#define AUDIO_IME_CONFIG_H
#include <ibus.h>
#include <string>
#include "common_log.h"
#include "RuntimeOptions.h"
#include "common.h"

class Config {
public:
    static void init(IBusBus *bus, RuntimeOptions * opts) {
        delete instance;
        instance = new Config(bus, opts);
    }
    static Config *getInstance() {
        return instance;
    }
    explicit Config(IBusBus *bus, RuntimeOptions * opts) : m_opts(opts) {
        m_config = ibus_bus_get_config(bus);
        g_object_ref_sink(m_config);

        FUN_DEBUG("ibus config %p", m_config);
        opts->speechAkId = GetString(CONF_NAME_ID);
        opts->speechSecret = GetString(CONF_NAME_SECRET);
        opts->wubi_table = GetString(CONF_NAME_WUBI);
        auto pinyin = GetString(CONF_NAME_PINYIN);
        if (pinyin.empty()) { // not configured yet
            pinyin = "true";
            SetString(CONF_NAME_PINYIN, pinyin);
        }
        opts->pinyin = (pinyin == "true");
        auto s = GetString(CONF_NAME_ORIENTATION);
        opts->lookupTableOrientation = IBUS_ORIENTATION_HORIZONTAL;
        if(!s.empty()) {
            auto ret = atoi(s.c_str());
            if(ret >= 0 && ret <= 2) {
                opts->lookupTableOrientation = static_cast<IBusOrientation>(ret);
            }
        }
        auto dictFastEnable = GetString(CONF_NAME_FAST_INPUT_ENABLED);
        if( dictFastEnable.empty() ) {
            SetString(CONF_NAME_FAST_INPUT_ENABLED, "true");
            dictFastEnable = "true";
        }
        opts->dictFastEnabled = dictFastEnable == "true";
        ibus_config_watch(m_config, CONF_SECTION, CONF_NAME_ID);
        ibus_config_watch(m_config, CONF_SECTION, CONF_NAME_SECRET);
        ibus_config_watch(m_config, CONF_SECTION, CONF_NAME_FAST_INPUT_RELOAD);
        g_signal_connect(m_config, "value-changed", G_CALLBACK(OnValueChanged), this);
        FUN_INFO("config value-changed signal connected");
    }
    ~Config() {
        g_object_unref(m_config);
    }
    [[nodiscard]] std::string GetString(const std::string &name);
    void SetString(const std::string& name, const std::string& val);
public:
private:
    IBusConfig *m_config;
    RuntimeOptions *m_opts;

    static Config * instance;
    static void OnValueChanged(IBusConfig *config, gchar *section, gchar *name, GVariant *value, gpointer user_data);
};

#endif // AUDIO_IME_CONFIG_H

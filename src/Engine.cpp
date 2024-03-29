//
// Created by zhangfuwen on 2022/1/22.
//

#include <cctype>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <glib-object.h>
#include <glib.h>
#include <ibus.h>
#include <map>
#include <pulse/simple.h>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "Config.h"
#include "DictWubi.h"
#include "Engine.h"
#include "common.h"
#include "common_log.h"
#include "DictFast.h"

using namespace std::placeholders;

Wubi *Engine::s_wubi = nullptr;
pinyin::DictPinyin *Engine::s_pinyin = nullptr;
DictFast *Engine::s_dictFast = nullptr;

Engine::Engine(IBusEngine *engine) {
    FUN_INFO("constructor");
    m_options = RuntimeOptions::get();
    m_engine = engine;
    // Setup Lookup table
    if (!s_wubi) {
        s_wubi = new Wubi(wubi86DictPath);
    }

    if (!s_pinyin) {
        s_pinyin = new pinyin::DictPinyin();
    }

    if (!s_dictFast) {
        s_dictFast = new DictFast();
    }

    m_speechRecognizer = new DictSpeech(this, m_options);
    m_lookupTable = new LookupTable(engine, m_options->lookupTableOrientation);
    PropertiesInit();
}

IBusEngine *Engine::getIBusEngine() { return m_engine; }

Engine::~Engine() {
    FUN_INFO("destructor");
    g_object_unref(m_props);
    delete m_lookupTable;
    g_object_unref(m_engine);
}

void Engine::OnCompleted(std::string text) {
    engine_commit_text(m_engine, ibus_text_new_from_string(text.c_str()));
    ibus_engine_update_preedit_text(m_engine, ibus_text_new_from_string(""), 0,
                                    false);
    m_options->speechOn = false;
    UpdateInputMode();
}

void Engine::OnFailed() {
    engine_commit_text(m_engine, ibus_text_new_from_string(""));
    ibus_engine_update_preedit_text(m_engine, ibus_text_new_from_string(""), 0,
                                    false);
    m_options->speechOn = false;
    UpdateInputMode();
}

void Engine::OnPartialResult(std::string text) {
    ibus_engine_update_preedit_text(
            m_engine, ibus_text_new_from_string(text.c_str()), 0, TRUE);
}

void Engine::Enable() { FUN_TRACE(""); }

void Engine::Disable() { FUN_TRACE(""); }

void Engine::IBusUpdateIndicator(long recordingTime) {
    ibus_engine_update_auxiliary_text(
            m_engine,
            ibus_text_new_from_string(IBusMakeIndicatorMsg(recordingTime).c_str()),
            TRUE);
}

// early return ?
// return value
std::pair<bool, bool> Engine::ProcessSpeech(guint keyval, guint keycode,
                                            guint state) {
    DictSpeech::Status status = m_speechRecognizer->GetStatus();
    if ((state & IBUS_CONTROL_MASK) && keyval == IBUS_KEY_grave) {
        FUN_DEBUG("status = %d", status);
        if (status == DictSpeech::WAITING) {
            FUN_DEBUG("waiting");
            return {true, TRUE};
        }
        if (status != DictSpeech::RECODING) {
            std::thread t1([&]() { m_speechRecognizer->Start(); });
            t1.detach();
            m_options->speechOn = true;
            UpdateInputMode();
        } else {
            m_speechRecognizer->Stop();
        }
        m_lookupTable->Show();
        ibus_engine_show_preedit_text(m_engine);
        ibus_engine_show_auxiliary_text(m_engine);
        return {true, true};
    }
    if (state & IBUS_CONTROL_MASK) {
        return {true, false};
    }

    // other key inputs
    if (status != DictSpeech::IDLE) {
        // don't respond to other key inputs when m_recording or m_waiting
        return {true, true};
    }
    return {false, false};
}

gboolean Engine::ProcessKeyEvent(guint keyval, guint keycode, guint state) {
    if (state & IBUS_RELEASE_MASK) { // only respond to key down
        return FALSE;
    }

    FUN_DEBUG("engine_process_key_event keycode: %d, keyval:%x", keycode, keyval);

    if (m_speechRecognizer != nullptr) {
        auto ret = ProcessSpeech(keyval, keycode, state);
        if (ret.first) { // early return
            return ret.second;
        }
    }

    if (state & IBUS_LOCK_MASK) {         // previously chinese mode
        if (keyval == IBUS_KEY_Caps_Lock) { // chinese to english mode
            FUN_INFO("caps lock pressed");
            m_options->capsOn = false;
            UpdateInputMode();
            Clear();
            return true;
        }

        if (!m_input.empty()) {
            auto ret = LookupTableNavigate(keyval);
            if (ret) {
                return true;
            }
        }

        // punctuation handlring
        bool isPunctuationHandled = handlePunctuation(keyval);
        if (isPunctuationHandled) {
            return true;
        }

        if (keyval == IBUS_KEY_space || keyval == IBUS_KEY_Return ||
            isdigit((char) (keyval)) || keycode == 1) {
            if (m_input.empty()) {
                return false;
            }
            int index = isdigit((char) (keyval)) ? (int) (keyval - IBUS_KEY_0) : -1;
            candidateSelected(m_lookupTable->GetGlobalCursor(index), keycode == 1);
            return true;
        }

        FUN_DEBUG("keyval %x, m_input.size:%lu", keyval, m_input.size());
        if (keyval == IBUS_KEY_BackSpace && !m_input.empty()) { // delete
            m_input = m_input.substr(0, m_input.size() - 1);
        } else if ((char) keyval == keyval && isalpha((char) keyval)) { // append new
            m_input += (char) tolower((int) keyval);
        } else { // don't know how to handle it
            return false;
        }
        // chinese mode
        ibus_engine_update_auxiliary_text(
                m_engine, ibus_text_new_from_string(m_input.c_str()), true);

        m_lookupTable->Clear();
        FUN_INFO("dictFast enable %p, enabled:%d", s_dictFast, m_options->dictFastEnabled);
        std::string out;
        if (m_options->dictFastEnabled && s_dictFast) {
            out = s_dictFast->Query(m_input);
            FUN_DEBUG("dictFast query %s:%s", m_input.c_str(), out.empty() ? "none" : out.c_str());
            if (!out.empty()) {
                FUN_INFO("dict query %s, %s", m_input.c_str(), out.c_str());
                m_lookupTable->Append(ibus_text_new_from_string(out.c_str()), false);
            }
        }
        WubiPinyinQuery(m_input);
        if (!out.empty() && m_lookupTable->Size() > 1) {
            m_lookupTable->CursorDown();
        }
        m_lookupTable->Update();
        m_lookupTable->Show();

        return true;

    } else {
        if (keyval == IBUS_KEY_Caps_Lock) { // english to chinese
            m_options->capsOn = true;
            UpdateInputMode();
        } else {
            // caps lock off and this key is not caps lock
            m_options->capsOn = false;
            UpdateInputMode();
        }

        // english mode
        m_lookupTable->Hide();
        ibus_engine_hide_preedit_text(m_engine);
        ibus_engine_hide_auxiliary_text(m_engine);
        return false;
    }
}

bool Engine::handlePunctuation(guint keyval) const {
    bool isPunctuationHandled = false;
    auto unicode = ibus_keyval_to_unicode(keyval);
    if (unicode != 0 && ((char) unicode) == unicode) {
        char c = (char) unicode;
        auto punctuation_en = R"(!"#$%&'()*+,-./:;<=>?@[\]^_`{|}~)"sv;
        gunichar punctuation_cn[] = {
                L'！', L'“', L'＃', L'￥', L'％', L'＆', L'‘', L'（',
                L'）', L'＊', L'＋', L'，', L'－', L'。', L'／', L'：',
                L'；', L'《', L'＝', L'》', L'？', L'＠', L'「', L'、',
                L'」', L'…', L'—', L'｀', L'『', L'｜', L'』', L'～'};
        auto pos = punctuation_en.find(c);
        if (pos != std::string_view::npos) {
            auto cnChar = punctuation_cn[pos];
            if (U"…—"sv.find(cnChar) != std::string_view::npos) { // use two chars
                ibus_engine_commit_text(m_engine, ibus_text_new_from_unichar(cnChar));
                ibus_engine_commit_text(m_engine, ibus_text_new_from_unichar(cnChar));
            } else {
                ibus_engine_commit_text(m_engine, ibus_text_new_from_unichar(cnChar));
            }
            isPunctuationHandled = true;
        }
    }
    return isPunctuationHandled;
}

void Engine::WubiPinyinQuery(std::string input) { // get pinyin candidates
    unsigned int nPinyinCandidates = 0;
    if (m_options->pinyin) {
        nPinyinCandidates = s_pinyin->Search(input);
    }
    FUN_DEBUG("num candidates %u for %s", nPinyinCandidates, input.c_str());

    // get wubi candidates
    TrieNode *wubiSubtree = nullptr;
    if (!m_options->wubi_table.empty() && s_wubi) { // no searching , no data
        wubiSubtree = s_wubi->Search(input);
    }
    map<uint64_t, string> m;
    if (wubiSubtree != nullptr && wubiSubtree->isEndOfWord) {
        // best exact match first
        auto it = wubiSubtree->values.rbegin();
        string candidate = it->second;
        auto text = ibus_text_new_from_string(candidate.c_str());
        m_lookupTable->Append(text, false);

        // others
        it++;
        while (it != wubiSubtree->values.rend()) {
            m.insert(*it); // insert to reorder
            it++;
        }
    }
    SubTreeTraversal(m, wubiSubtree); // insert subtree and reorder

    // wubi and pinyin candidate one after another
    int j = 0;
    FUN_DEBUG("map size:%lu", m.size());
    auto it = m.rbegin();
    while (true) {
        if (j >= nPinyinCandidates && it == m.rend()) {
            break;
        }
        if (it != m.rend()) {
            auto value = it->second;
            string &candidate = value;
            auto text = ibus_text_new_from_string(candidate.c_str());
            m_lookupTable->Append(text, false);
            it++;
        }
        if (j < nPinyinCandidates) {
            wstring buffer = s_pinyin->GetCandidate(j);
            glong items_read;
            glong items_written;
            GError *error;
            gunichar *utf32_str =
                    g_utf16_to_ucs4(reinterpret_cast<const gunichar2 *>(buffer.data()),
                                    buffer.size(), &items_read, &items_written, &error);
            auto text = ibus_text_new_from_ucs4(utf32_str);
            m_lookupTable->Append(text, true);
            j++;
        }
    }
}

void Engine::Clear() { // commit input as english
    engine_commit_text(getIBusEngine(),
                       ibus_text_new_from_string(m_input.c_str()));
    m_input = "";
    m_lookupTable->Clear();
    m_lookupTable->Hide();
    ibus_engine_update_auxiliary_text(m_engine, ibus_text_new_from_string(""),
                                      true);
    ibus_engine_hide_preedit_text(m_engine);
    ibus_engine_hide_auxiliary_text(m_engine);
}

void Engine::SwitchWubi() {
    FUN_INFO("Switch wubi table");
    Clear();
    auto x = s_wubi;
    s_wubi = nullptr;
    delete x;
    std::thread([&]() {
        g_object_ref(m_engine);
        // keep engine not destructed
        if (!m_options->wubi_table.empty()) {
            s_wubi = new Wubi(m_options->wubi_table);
        }
        FUN_INFO("done switching wubi table to [%s]",
                 m_options->wubi_table.c_str());
        g_object_unref(m_engine);
    }).detach();
}

void Engine::ReloadFastTable() {
    FUN_INFO("reload fast table");
    Clear();
    auto x = s_dictFast;
    s_dictFast = nullptr;
    delete x;
    std::thread([&]() {
        g_object_ref(m_engine);
        // keep engine not destructed
        if (m_options->dictFastEnabled) {
            s_dictFast = new DictFast();
        }
        FUN_INFO("dictFast reloaded");
        g_object_unref(m_engine);
    }).detach();
}

bool Engine::LookupTableNavigate(guint keyval) {
    bool return1 = false;
    if (keyval == IBUS_KEY_equal || keyval == IBUS_KEY_Right) {
        FUN_DEBUG("equal pressed");
        m_lookupTable->PageDown();
        m_lookupTable->Update();
        return1 = true;
    } else if (keyval == IBUS_KEY_minus || keyval == IBUS_KEY_Left) {
        FUN_DEBUG("minus pressed");
        m_lookupTable->PageUp();
        m_lookupTable->Update();
        return1 = true;
    } else if (keyval == IBUS_KEY_Down) {
        FUN_DEBUG("down pressed");
        m_lookupTable->CursorDown();
        m_lookupTable->Update();
        return1 = true;
    } else if (keyval == IBUS_KEY_Up) {
        FUN_DEBUG("up pressed");
        m_lookupTable->CursorUp();
        m_lookupTable->Update();
        return1 = true;
    }
    return return1;
}

// static
void Engine::OnCandidateClicked(IBusEngine *engine, guint index, guint button,
                                guint state) {
    candidateSelected(index);
}

void Engine::FocusIn() {
    FUN_TRACE("Entry");
    UpdateInputMode();
    if (m_options->dictFastPendingReload) {
        ReloadFastTable();
        m_options->dictFastPendingReload = false;
    }
    ibus_engine_register_properties(m_engine, m_props);
    FUN_TRACE("Exit");
}

void Engine::FocusOut() {
    FUN_TRACE("Entry");
    FUN_TRACE("Exit");
}

std::string makeInputMode(bool pinyin, std::string wubi) {
    if (!wubi.empty() && pinyin) {
        return "混";
    }
    if (pinyin) {
        return "拼";
    }
    if (!wubi.empty()) {
        return "五";
    }
    return "X";
}

void Engine::UpdateInputMode() {
    FUN_TRACE("Entry");
    auto prop = ibus_prop_list_get(m_props, 0);
    if (prop != nullptr) {
        auto name = ibus_property_get_key(prop);
        std::string mode;
        if (m_options->speechOn) {
            mode = "语";
        } else if (!m_options->capsOn) {
            mode = "英";
        } else {
            mode = makeInputMode(m_options->pinyin, m_options->wubi_table);
        }
        FUN_INFO("name:%s, inputMode:%s", name, mode.c_str());
        ibus_property_set_symbol(prop, ibus_text_new_from_string(mode.c_str()));
        ibus_property_set_label(prop, ibus_text_new_from_string(mode.c_str()));
    } else {
        FUN_ERROR("prop is nullptr");
    }
    ibus_engine_update_property(m_engine, prop);
    FUN_TRACE("Exit");
}

IBusProperty *NewProperty(std::string name, IBusPropType type, bool checked, IBusPropList *list = nullptr) {
    const std::string iconBase = "/usr/share/icons/hicolor/scalable/apps/";
#define ICON(name) (iconBase + name).c_str()
    auto prop = ibus_property_new(
            name.c_str(),
            type,
            ibus_text_new_from_string(_(("label_"s + name).c_str())),
            ICON(name + ".png"),
            ibus_text_new_from_string(_(("tooltip_"s + name).c_str())),
            true,
            true,
            checked ? PROP_STATE_CHECKED : PROP_STATE_UNCHECKED,
            list);
    return prop;
#undef ICON
}

void Engine::PropertiesInit() {
    auto prop_list = ibus_prop_list_new();
    auto prop_input_mode = NewProperty("InputMode", PROP_TYPE_NORMAL, true);

    auto prop_pinyin = NewProperty("pinyin", PROP_TYPE_TOGGLE, m_options->pinyin);
    ibus_property_set_symbol(prop_pinyin, ibus_text_new_from_string("ax"));

    auto prop_preference = NewProperty("preference", PROP_TYPE_NORMAL, true);

    auto wubi_prop_sub_list = ibus_prop_list_new();
    auto prop_wubi_table_no = NewProperty("wubi_table_no", PROP_TYPE_RADIO, m_options->wubi_table.empty());
    auto prop_wubi_table_86 = NewProperty("wubi_table_86", PROP_TYPE_RADIO, m_options->wubi_table == wubi86DictPath);
    auto prop_wubi_table_98 = NewProperty("wubi_table_98", PROP_TYPE_RADIO, m_options->wubi_table == wubi98DictPath);
    ibus_prop_list_append(wubi_prop_sub_list, prop_wubi_table_no);
    ibus_prop_list_append(wubi_prop_sub_list, prop_wubi_table_86);
    ibus_prop_list_append(wubi_prop_sub_list, prop_wubi_table_98);

    auto prop_wubi = NewProperty("wubi", PROP_TYPE_MENU, true, wubi_prop_sub_list);

    auto orientation_list = ibus_prop_list_new();
    auto system = NewProperty("system", PROP_TYPE_RADIO, m_options->lookupTableOrientation == IBUS_ORIENTATION_SYSTEM);
    auto horizontal =
            NewProperty("horizontal", PROP_TYPE_RADIO,
                        m_options->lookupTableOrientation == IBUS_ORIENTATION_HORIZONTAL);
    auto vertical =
            NewProperty("vertical", PROP_TYPE_RADIO, m_options->lookupTableOrientation == IBUS_ORIENTATION_VERTICAL);
    ibus_prop_list_append(orientation_list, system);
    ibus_prop_list_append(orientation_list, horizontal);
    ibus_prop_list_append(orientation_list, vertical);
    auto prop_orientation = NewProperty("orientation", PROP_TYPE_MENU, true, orientation_list);

    auto prop_fast_input_enable = NewProperty("fast_input_enabled", PROP_TYPE_TOGGLE, m_options->dictFastEnabled);

    g_object_ref_sink(prop_list);
    ibus_prop_list_append(prop_list, prop_input_mode);
    ibus_prop_list_append(prop_list, prop_wubi);
    ibus_prop_list_append(prop_list, prop_pinyin);
    ibus_prop_list_append(prop_list, prop_orientation);
    ibus_prop_list_append(prop_list, prop_fast_input_enable);
    ibus_prop_list_append(prop_list, prop_preference);
    m_props = prop_list;
}

// static
void Engine::OnPropertyActivate(IBusEngine *engine, const gchar *name,
                                guint state) {
    FUN_TRACE("Entry");
    FUN_INFO("property changed, name:%s, state:%d", name, state);
    auto n = std::string_view(name);
    if (std::string(name) == "preference") {
        g_spawn_command_line_async("fun-setup", nullptr);
        return;
    }

    if (state == 1 && n.starts_with("orientation")) {
        if (n.ends_with("vertical")) {
            m_options->lookupTableOrientation = IBUS_ORIENTATION_VERTICAL;
            m_lookupTable->setOrientation(m_options->lookupTableOrientation);
            Config::getInstance()->SetString(
                    CONF_NAME_ORIENTATION,
                    std::to_string(m_options->lookupTableOrientation));
        } else if (n.ends_with("horizontal")) {
            m_options->lookupTableOrientation = IBUS_ORIENTATION_HORIZONTAL;
            m_lookupTable->setOrientation(m_options->lookupTableOrientation);
            Config::getInstance()->SetString(
                    CONF_NAME_ORIENTATION,
                    std::to_string(m_options->lookupTableOrientation));
        } else if (n.ends_with("system")) {
            m_options->lookupTableOrientation = IBUS_ORIENTATION_SYSTEM;
            m_lookupTable->setOrientation(m_options->lookupTableOrientation);
            Config::getInstance()->SetString(
                    CONF_NAME_ORIENTATION,
                    std::to_string(m_options->lookupTableOrientation));
        }
    }

    if (n == "fast_input_enabled") {
        FUN_INFO("fast_input_enbaled %d", state);
        m_options->dictFastEnabled = state == 1;
        Config::getInstance()->SetString(
                CONF_NAME_FAST_INPUT_ENABLED,
                state == 1 ? "true" : "false"
        );
        ReloadFastTable();
    }

    auto oldOptions = *m_options;
    if (std::string(name) == "wubi_table_no") {
        if (state == 1) {
            m_options->wubi_table = "";
        }
    } else if (std::string(name) == "wubi_table_86") {
        if (state == 1) {
            m_options->wubi_table = wubi86DictPath;
        }
    } else if (std::string(name) == "wubi_table_98") {
        if (state == 1) {
            m_options->wubi_table = wubi98DictPath;
        }
    } else if (std::string(name) == "pinyin") {
        m_options->pinyin = state;
    }
    if (m_options->wubi_table != oldOptions.wubi_table) {
        Config::getInstance()->SetString(CONF_NAME_WUBI, m_options->wubi_table);
        SwitchWubi();
    }

    if (m_options->pinyin != oldOptions.pinyin) {
        Config::getInstance()->SetString(CONF_NAME_PINYIN,
                                         m_options->pinyin ? "true" : "false");
    }
    UpdateInputMode();

    FUN_TRACE("Exit");
}

void Engine::engine_commit_text(IBusEngine *engine, IBusText *text) {
    ibus_engine_commit_text(engine, text);
    ibus_engine_hide_preedit_text(engine);
    ibus_engine_hide_auxiliary_text(engine);
    m_lookupTable->Clear();
    m_lookupTable->Hide();
}

std::string Engine::IBusMakeIndicatorMsg(long recordingTime) {
    std::string msg = "press C-` to toggle record[";
    DictSpeech::Status status = m_speechRecognizer->GetStatus();
    if (status == DictSpeech::RECODING) {
        msg += "m_recording " + std::to_string(recordingTime);
    }
    if (status == DictSpeech::WAITING) {
        msg += "m_waiting";
    }
    msg += "]";
    return msg;
}

void Engine::candidateSelected(guint index, bool ignoreText) {
    auto cand = m_lookupTable->GetCandidateGlobal(index);

    if (cand.isPinyin && s_wubi && cand.text) {
        std::string code = s_wubi->CodeSearch(cand.text->text);
        if (code.empty()) {
            ibus_engine_hide_auxiliary_text(m_engine);
        } else {
            std::string hint = "五笔[" + code + "]";
            ibus_engine_update_auxiliary_text(
                    m_engine, ibus_text_new_from_string(hint.c_str()), true);
            FUN_DEBUG("cursor:%d, text:%s, wubi code:%s - %lu", index,
                      cand.text->text, code.c_str(), code.size());
        }
    } else {
        FUN_DEBUG("cursor:%d, text:%s, is not pinyin", index, cand.text->text);
        ibus_engine_hide_auxiliary_text(m_engine);
    }
    if (!ignoreText) { // which means escape
        ibus_engine_commit_text(m_engine, cand.text);
    }
    m_lookupTable->Clear();
    m_lookupTable->Update();
    m_lookupTable->Hide();
    ibus_engine_hide_preedit_text(m_engine);
    m_input.clear();
}

void Engine::Reset() {
//    ReloadFastTable();
//    SwitchWubi();
}

LookupTable::LookupTable(IBusEngine *engine, IBusOrientation orientation) {
    m_engine = engine;
    m_table = ibus_lookup_table_new(10, 0, TRUE, TRUE);
    FUN_INFO("table %p", m_table);
    g_object_ref_sink(m_table);

    ibus_lookup_table_set_round(m_table, true);
    ibus_lookup_table_set_page_size(m_table, 5);
    ibus_lookup_table_set_orientation(m_table, orientation);
}

LookupTable::~LookupTable() {
    Clear();
    Hide();
    g_object_unref(m_table);
}

void LookupTable::Append(IBusText *text, bool pinyin) {
    ibus_lookup_table_append_candidate(m_table, text);
    m_candidateAttrs.emplace_back(pinyin);
}

void LookupTable::AppendLabel(std::string s) {
    AppendLabel(ibus_text_new_from_string(s.c_str()));
}

void LookupTable::AppendLabel(IBusText *text) {
    ibus_lookup_table_append_label(m_table, text);
}

void LookupTable::Show() { ibus_engine_show_lookup_table(m_engine); }

void LookupTable::Hide() { ibus_engine_hide_lookup_table(m_engine); }

void LookupTable::PageUp() { ibus_lookup_table_page_up(m_table); }

void LookupTable::PageDown() { ibus_lookup_table_page_down(m_table); }

void LookupTable::CursorDown() {
    bool ret = ibus_lookup_table_cursor_down(m_table);
    if (!ret) {
        FUN_ERROR("failed to put cursor down");
    }
}

void LookupTable::CursorUp() {
    bool ret = ibus_lookup_table_cursor_up(m_table);
    if (!ret) {
        FUN_ERROR("failed to put cursor up");
    }
}

Candidate LookupTable::GetCandidateGlobal(guint globalCursor) {
    Candidate cand;
    auto text = ibus_lookup_table_get_candidate(m_table, globalCursor);
    auto attr = m_candidateAttrs[globalCursor];
    cand.isPinyin = attr._isPinyin;
    cand.text = text;
    return cand;
}

guint LookupTable::GetGlobalCursor(int index) {
    guint cursor = ibus_lookup_table_get_cursor_pos(m_table);
    if (index >= 0) {
        guint cursor_page = ibus_lookup_table_get_cursor_in_page(m_table);
        cursor = cursor + (index - cursor_page) - 1;
    }
    return cursor;
}

void LookupTable::Update() {
    ibus_engine_update_lookup_table_fast(m_engine, m_table, true);
}

void LookupTable::Clear() {
    ibus_lookup_table_clear(m_table);
    m_candidateAttrs.clear();
}

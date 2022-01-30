//
// Created by zhangfuwen on 2022/1/30.
//

#ifndef IBUS_FUN_DICTFAST_H
#define IBUS_FUN_DICTFAST_H

#include <configor/json.hpp>
#include <filesystem>
#include <fstream>
#include "common.h"
#include "common_log.h"

class DictFast {
public:
    DictFast() {
        m_dict = load_fast_input_config();
    }

    std::string Query(std::string key) {
        FUN_DEBUG("m_dict size: %d", m_dict.size());
        if(m_dict.contains(key)) {
            auto out = exec(m_dict[key].c_str());
            return trim(out);
        }
        return "";
    }
private:
    std::map<std::string, std::string> m_dict;



};

#endif // IBUS_FUN_DICTFAST_H

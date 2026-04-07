/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "container_builder.h"

namespace gs {

container_builder::container_builder(sc_core::sc_module_name _name)
    : gs::ModuleFactory::ContainerDeferModulesConstruct(_name)
{
    dispatch(std::string(this->name()));
    redirect_socket_params(std::string(this->name()));
    ModulesConstruct();
}

void container_builder::dispatch(const std::string& _name)
{
    if (_name.empty()) {
        SCP_FATAL(()) << "dispatch: function was called with empty string!";
    }

    std::string config_table_prefix;

    auto all_config_params = m_broker.get_unconsumed_preset_values(
        [&_name](const std::pair<std::string, cci_value>& iv) {
            return iv.first.find(_name + ".config.") != std::string::npos;
        });

    for (const auto& param : all_config_params) {
        if (param.first.find(".config.") != std::string::npos) {
            size_t config_pos = param.first.find(".config.");
            config_table_prefix = param.first.substr(0, config_pos + std::string(".config").size());
            break;
        }
    }

    if (config_table_prefix.empty()) {
        SCP_WARN(()) << "No config table found for container_builder" << _name;
        return;
    }

    std::vector<std::string> params_to_ignore;

    for (const auto& param : all_config_params) {
        std::string old_name = param.first;
        cci_value param_value = param.second;

        if (old_name.find(".sockets.") != std::string::npos) {
            size_t sockets_pos = old_name.find(".sockets.");
            std::string socket_name = old_name.substr(sockets_pos + std::string(".sockets.").size());

            if (param_value.is_string()) {
                std::string internal_path = param_value.get_string();
                if (internal_path.length() > 0 && internal_path[0] == '&') {
                    internal_path = internal_path.substr(1);
                    m_socket_redirects[socket_name] = internal_path;
                    params_to_ignore.push_back(old_name);
                    continue;
                }
            }
        }

        std::string new_name = old_name;
        size_t config_pos = old_name.find(config_table_prefix);
        if (config_pos == 0) {
            new_name = _name + old_name.substr(config_table_prefix.length());
            m_broker.set_preset_cci_value(new_name, param_value);
            params_to_ignore.push_back(old_name);
        }
    }

    for (const auto& old_name : params_to_ignore) {
        m_broker.lock_preset_value(old_name);
    }

    m_broker.ignore_unconsumed_preset_values([&config_table_prefix](const std::pair<std::string, cci_value>& iv) {
        return iv.first.find(config_table_prefix) == 0;
    });
}

void container_builder::redirect_socket_params(const std::string& _name)
{
    if (_name.empty()) {
        SCP_FATAL(()) << "redirect_socket_params: function was called with empty string!";
    }

    for (const auto& redirect : m_socket_redirects) {
        const std::string& alias_name = redirect.first;
        const std::string& internal_path = redirect.second;

        std::string alias_prefix = _name + "." + alias_name;
        std::string internal_prefix = _name + "." + internal_path;

        auto alias_params = m_broker.get_unconsumed_preset_values(
            [&alias_prefix](const std::pair<std::string, cci_value>& iv) {
                return iv.first.find(alias_prefix) == 0 && iv.first.length() > alias_prefix.length();
            });

        for (const auto& param : alias_params) {
            std::string alias_param_name = param.first;
            cci_value param_value = param.second;

            std::string suffix = alias_param_name.substr(alias_prefix.length());
            std::string internal_param_name = internal_prefix + suffix;

            m_broker.set_preset_cci_value(internal_param_name, param_value);
            m_broker.lock_preset_value(alias_param_name);
        }

        std::string alias_socket_path = _name + "." + alias_name;
        std::string internal_socket_path = _name + "." + internal_path;

        auto all_bind_params = m_broker.get_unconsumed_preset_values(
            [](const std::pair<std::string, cci_value>& iv) { return iv.first.find(".bind") != std::string::npos; });

        std::string relative_alias_path = alias_socket_path;
        std::string relative_internal_path = internal_socket_path;

        size_t first_dot = _name.find_first_of('.');
        if (first_dot != std::string::npos) {
            std::string parent_prefix = _name.substr(0, first_dot + 1);
            if (alias_socket_path.find(parent_prefix) == 0) {
                relative_alias_path = alias_socket_path.substr(parent_prefix.length());
            }
            if (internal_socket_path.find(parent_prefix) == 0) {
                relative_internal_path = internal_socket_path.substr(parent_prefix.length());
            }
        }

        for (const auto& param : all_bind_params) {
            std::string param_name = param.first;
            cci_value param_value = param.second;

            if (param_value.is_string()) {
                std::string bind_path = param_value.get_string();

                std::string search_paths[] = { "&" + alias_socket_path, alias_socket_path, "&" + relative_alias_path,
                                               relative_alias_path };

                bool matched = false;
                std::string matched_path;
                for (const auto& search_path : search_paths) {
                    if (bind_path == search_path) {
                        matched = true;
                        matched_path = search_path;
                        break;
                    }
                }

                if (matched) {
                    std::string new_bind_path;
                    if (bind_path.find("&platform.") == 0 || bind_path.find("platform.") == 0) {
                        new_bind_path = "&" + internal_socket_path;
                    } else {
                        new_bind_path = "&" + relative_internal_path;
                    }

                    m_broker.set_preset_cci_value(param_name, cci_value(new_bind_path));
                } else if (bind_path.find(';') != std::string::npos) {
                    bool replaced = false;
                    std::string new_bind_path = bind_path;

                    for (const auto& search_path : search_paths) {
                        size_t pos = 0;
                        while ((pos = new_bind_path.find(search_path, pos)) != std::string::npos) {
                            bool valid_match = true;
                            if (pos + search_path.length() < new_bind_path.length()) {
                                char next_char = new_bind_path[pos + search_path.length()];
                                if (next_char != ';' && next_char != ' ' && next_char != '\t' && next_char != '\n') {
                                    valid_match = false;
                                }
                            }

                            if (valid_match) {
                                std::string replacement;
                                if (search_path.find("&platform.") == 0 || search_path.find("platform.") == 0) {
                                    replacement = "&" + internal_socket_path;
                                } else {
                                    replacement = "&" + relative_internal_path;
                                }
                                new_bind_path.replace(pos, search_path.length(), replacement);
                                replaced = true;
                                pos += replacement.length();
                            } else {
                                pos += search_path.length();
                            }
                        }
                        if (replaced) {
                            break;
                        }
                    }

                    if (replaced) {
                        m_broker.set_preset_cci_value(param_name, cci_value(new_bind_path));
                    }
                }
            }
        }
    }

    m_broker.ignore_unconsumed_preset_values([this, &_name](const std::pair<std::string, cci_value>& iv) {
        for (const auto& redirect : m_socket_redirects) {
            const std::string& alias_name = redirect.first;
            std::string alias_prefix = _name + "." + alias_name;
            if (iv.first.find(alias_prefix) == 0) {
                return true;
            }
        }
        return false;
    });
}

std::string container_builder::replace_all(std::string str, const std::string& from, const std::string& to)
{
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos) {
        str.replace(pos, from.length(), to);
        pos += to.length();
    }
    return str;
}

} // namespace gs

typedef gs::container_builder container_builder;
void module_register() { GSC_MODULE_REGISTER_C(container_builder); }
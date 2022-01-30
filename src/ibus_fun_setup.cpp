//
// Created by zhangfuwen on 2022/1/18.
//
#include <gtkmm.h>
#include <gtkmm/window.h>
#include <ibus.h>

#include "common.h"
#include "common_log.h"

#include "configor/json.hpp"

#include <array>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>



int grid_get_num_rows(Gtk::Grid *grid) {
    for (int i = 0; i < 1000; i++) {
        if (grid->get_child_at(0, i) == nullptr) {
            return i;
        }
    }
    return 0;
}


void save_fast_input_config(Gtk::Grid *grid) {
    auto user_dir = get_ibus_fun_user_data_dir();
    if(!std::filesystem::is_directory(user_dir) && !std::filesystem::create_directory(user_dir)) {
        FUN_ERROR("directory \"%s\" does not exist and create failed", user_dir.c_str());
        return;
    }

    auto file_path = user_dir + "fast_input.json";
    if(std::filesystem::exists(file_path) && !std::filesystem::remove(file_path)) {
        FUN_ERROR("file \"%s\" does exist and remove failed", file_path.c_str());
        return;
    }

    std::ofstream ofs(file_path);

    configor::json j;
    for (int i = 0; i < 1000; i++) {
        if (grid->get_child_at(0, i) == nullptr) {
            FUN_ERROR("child \"%d\" does not exist", i);
            break;
        }
        auto keyEntry = (Gtk::Entry *)grid->get_child_at(0, i);
        auto cmdEntry = (Gtk::Entry *)grid->get_child_at(1, i);
        auto keyString = keyEntry ? keyEntry->get_text() : "";
        auto cmdString = cmdEntry ? cmdEntry->get_text() : "";
        if (keyString.empty() || cmdString.empty()) {
            FUN_ERROR("key or cmd is empty");
            continue;
        }
        if (keyString.find_first_not_of("abcdefghijklmnopqrstuvwxyz") != std::string::npos) {
            FUN_ERROR("key is invalid %s", keyString.c_str());
            continue;
        }
        j[keyString] = cmdString;
        FUN_INFO("save key to %s", keyString.c_str());
        FUN_INFO("save cmd to %s", cmdString.c_str());
    }
    try {
        FUN_INFO("writing config file %s", j.dump().c_str());
        ofs << j << std::endl;
        ofs.flush();
    } catch (std::exception &e) {
        FUN_ERROR("file write error, %s", e.what());
    }
}

void grid_add_row(Gtk::Grid *grid, int row_index, std::string key, std::string cmd) {
    FUN_INFO("add row %d, %s, %s", row_index, key.c_str(), cmd.c_str());
    auto entry = Gtk::manage(new Gtk::Entry());
    entry->set_editable();
    entry->set_text(key.c_str());
    grid->attach(*entry, 0, row_index, 1, 1);
    entry->show();
    entry = Gtk::manage(new Gtk::Entry());
    entry->set_editable();
    entry->set_text(cmd.c_str());
    grid->attach(*entry, 1, row_index, 1, 1);
    entry->show();
    auto button = Gtk::manage(new Gtk::Button(_("checkResult")));
    grid->attach(*button, 2, row_index, 1, 1);
    const int i = row_index;
    button->signal_clicked().connect([=]() {
      FUN_INFO("%d", i);
      Gtk::Entry *entry1 = (Gtk::Entry *)grid->get_child_at(0, i);
      Gtk::Entry *entry2 = (Gtk::Entry *)grid->get_child_at(1, i);
      FUN_INFO("%s %s", entry1->get_text().c_str(), entry2->get_text().c_str());
      auto text = entry2->get_text();
      std::string s = text;
      FUN_INFO("text %s", s.c_str());
      GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
      auto dialog = gtk_message_dialog_new(
          nullptr, flags, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "%s", exec(s.c_str()).c_str());
      gtk_dialog_run(GTK_DIALOG(dialog));
      gtk_widget_destroy(dialog);
    });
    button->show();
}

void setup_fast_input(Glib::RefPtr<Gtk::Builder> builder) {
    Gtk::Grid *grid = nullptr;
    builder->get_widget<Gtk::Grid>("fast_input_grid", grid);

    // refresh contents
    auto m = load_fast_input_config();
    FUN_INFO("loaded configuration file, %d items", m.size());
    auto num_rows = grid_get_num_rows(grid);
    for(const auto & [key, cmd] : m) {
        grid_add_row(grid, num_rows, key, cmd);
        num_rows++;
    }

    // save content button
    Gtk::Button *save_button;
    builder->get_widget<Gtk::Button>("fast_input_save_button", save_button);
    save_button->signal_clicked().connect([=]() {
        FUN_INFO("save button_clicked");
        save_fast_input_config(grid);
    });

    // new button
    Gtk::Button *new_button;
    builder->get_widget<Gtk::Button>("fast_input_new_button", new_button);
    new_button->signal_clicked().connect([=]() {
        FUN_INFO("button_clicked");
        auto num_rows = grid_get_num_rows(grid);
        grid_add_row(grid, num_rows, "", "");
    });

}

Glib::RefPtr<Gtk::Application> app;
Glib::RefPtr<Gtk::Builder> builder;

int main(int argc, char *argv[]) {
    app = Gtk::Application::create("fun.xjbcode.ibus-fun.setup");
    builder = Gtk::Builder::create_from_file("/usr/share/ibus/ibus-fun/data/ibus_fun_setup.glade");

    Gtk::Window *win;
    builder->get_widget<Gtk::Window>("win1", win);
    Gtk::Button *but1;
    builder->get_widget<Gtk::Button>("but1", but1);
    Gtk::Label *page1;
    builder->get_widget<Gtk::Label>("page1", page1);
    Gtk::Entry *id_text;
    builder->get_widget<Gtk::Entry>("id_text", id_text);
    Gtk::Entry *secret_text;
    builder->get_widget<Gtk::Entry>("secret_text", secret_text);
    Gtk::Button *but_set_config;
    builder->get_widget<Gtk::Button>("but_set_config", but_set_config);
    Gtk::Label *page2;
    builder->get_widget<Gtk::Label>("page2", page2);

    setup_fast_input(builder);

    IBusBus *g_bus;
    IBusConfig *config;

    but_set_config->signal_clicked().connect([&]() {
        auto id_ustr = id_text->get_text();
        auto secret_ustr = secret_text->get_text();
        auto ret = ibus_config_set_value(config, CONF_SECTION, CONF_NAME_ID, g_variant_new_string(id_ustr.c_str()));
        if (!ret) {
            FUN_ERROR("failed to set value");
        }
        ret = ibus_config_set_value(config, CONF_SECTION, CONF_NAME_SECRET, g_variant_new_string(secret_ustr.c_str()));
        if (!ret) {
            FUN_ERROR("failed to set value");
        }
    });

    but1->signal_clicked().connect([&]() {
        printf("clicked");
        ibus_init();
        g_bus = ibus_bus_new();
        g_object_ref_sink(g_bus);

        FUN_DEBUG("bus %p", g_bus);

        if (!ibus_bus_is_connected(g_bus)) {
            FUN_WARN("not connected to ibus");
            return;
        }
        config = ibus_bus_get_config(g_bus);
        if (!config) {
            FUN_ERROR("failed to get config from bus:%p", g_bus);
            return;
        } else {
            g_object_ref_sink(config);
        }

        auto id = ibus_config_get_value(config, CONF_SECTION, CONF_NAME_ID);
        if (id != nullptr) {
            auto akId = g_variant_get_string(id, nullptr);
            if (akId != nullptr) {
                id_text->set_text(akId);
            } else {
                FUN_ERROR("failed to get akId");
            }
        } else {
            FUN_ERROR("failed to get akId");
        }

        auto secret = ibus_config_get_value(config, CONF_SECTION, CONF_NAME_SECRET);
        if (secret != nullptr) {
            auto akSecret = g_variant_get_string(secret, nullptr);
            if (akSecret != nullptr) {
                secret_text->set_text(akSecret);
            } else {
                FUN_ERROR("failed to get akSecret");
            }
        } else {
            FUN_ERROR("failed to get akSecret");
        }

        if (config) {
            g_object_unref(config);
        }
        g_object_unref(g_bus);
    });
    win->set_title("ibus-fun configuration");

    // return app->run(*win1, argc, argv);
    return app->run(*win, argc, argv);
}
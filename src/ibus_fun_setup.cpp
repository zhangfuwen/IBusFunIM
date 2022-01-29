//
// Created by zhangfuwen on 2022/1/18.
//
#include <gtkmm.h>
#include <gtkmm/window.h>
#include <ibus.h>

#include "common.h"
#include "common_log.h"

#include <array>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

std::string exec(const char *cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

int grid_get_num_rows(Gtk::Grid *grid) {
    for (int i = 0; i < 1000; i++) {
        if (grid->get_child_at(0, i) == nullptr) {
            return i;
        }
    }
    return 0;
}

void updateGridButtons(Gtk::Grid *grid, int num_rows);
int num_rows;
void setup_fast_input(Glib::RefPtr<Gtk::Builder> builder) {
    Gtk::Button *save_button;
    builder->get_widget<Gtk::Button>("fast_input_save_button", save_button);
    save_button->signal_clicked().connect([&]() { FUN_INFO("button_clicked"); });

    Gtk::Grid *grid = nullptr;
    builder->get_widget<Gtk::Grid>("fast_input_grid", grid);
    FUN_INFO("button_clicked %p", grid);
    FUN_INFO("%d %d", grid->get_allocated_width(), grid->get_height());
    num_rows = grid_get_num_rows(grid);

    Gtk::Button *new_button;
    builder->get_widget<Gtk::Button>("fast_input_new_button", new_button);
    new_button->signal_clicked().connect([=]() {
        FUN_INFO("button_clicked");
        FUN_INFO("button_clicked %p", grid);
        Gtk::Grid *grid = nullptr;
        builder->get_widget<Gtk::Grid>("fast_input_grid", grid);
        FUN_INFO("button_clicked %p", grid);
        FUN_INFO("%d %d", grid->get_width(), grid->get_height());
        FUN_INFO("num rows %d", num_rows);
        //        grid->insert_row(num_rows+1);
        //      Gtk::Entry *entry1 = (Gtk::Entry *)grid->get_child_at(0, 0);
        //      Gtk::Entry *entry2 = (Gtk::Entry *)grid->get_child_at(1, 0);
//        Gtk::Label *label = Gtk::manage(new Gtk::Label("File Name :"));
        auto entry = Gtk::manage(new Gtk::Entry());
//        entry->set_text("hello");
        grid->attach(*entry, 0, num_rows++, 1, 1);
      entry->show();
//      grid->add(*label);
//      auto entry = Gtk::manage(new Gtk::Entry());
//      entry->set_text("hello");
//      grid->attach(*entry, 0, 0, 3, 3);
//        grid->insert_row(0);
//      grid->attach(*entry, 1, num_rows);
//      grid->attach(*entry, 2, num_rows);
      //      grid->attach((Gtk::Entry*) entry2->gobj_copy(), num_rows, 1);
    });

    updateGridButtons(grid, num_rows);
}
void updateGridButtons(Gtk::Grid *grid, int num_rows) {
    for (int i = 0; i < num_rows; i++) {
        Gtk::Button *check_result_button = (Gtk::Button *)grid->get_child_at(2, i);
        check_result_button->signal_clicked().connect([=]() {
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
    }
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
//
// Created by zhangfuwen on 2022/1/18.
//
#include <gtkmm.h>
#include <gtkmm/filechooser.h>
#include <gtkmm/filechooserdialog.h>
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

Glib::RefPtr<Gtk::Application> app;
Glib::RefPtr<Gtk::Builder> builder;
Gtk::Window *win;

int grid_get_num_rows(Gtk::Grid *grid) {
    for (int i = 0; i < 1000; i++) {
        if (grid->get_child_at(0, i) == nullptr) {
            return i;
        }
    }
    return 0;
}

void save_fast_input_config(Gtk::Grid *grid, std::string filepath = "") {
    if (filepath.empty()) {
        auto user_dir = get_ibus_fun_user_data_dir();
        if (!std::filesystem::is_directory(user_dir) && !std::filesystem::create_directory(user_dir)) {
            FUN_ERROR("directory \"%s\" does not exist and create failed", user_dir.c_str());
            return;
        }

        filepath = user_dir + "fast_input.json";
    }
    if (std::filesystem::exists(filepath) && !std::filesystem::remove(filepath)) {
        FUN_ERROR("file \"%s\" does exist and remove failed", filepath.c_str());
        return;
    }

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
    auto s = j.dump();
    FUN_INFO("writing config file %s, content:%s", filepath.c_str(), s.c_str());
    int fd = open(filepath.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd < 0) {
        FUN_ERROR("failed to open file %s, %s", filepath.c_str(), strerror(errno));
    } else {
        FUN_INFO("file opened ,fd %d", fd);
        int ret = write(fd, s.c_str(), s.size());
        if (ret != s.size()) {
            FUN_ERROR("failed to write to file %d:%d", ret, s.size());
        }
        write(fd, "\n", 1);
        close(fd);
    }
}

void grid_set_header(Gtk::Grid *grid) {
    Gtk::Label *label1 = new Gtk::Label(_("key"));
    Gtk::Label *label2 = new Gtk::Label(_("cmd"));
    Gtk::Label *label3 = new Gtk::Label(_("checkResult"));
    Gtk::Label *label4 = new Gtk::Label(_("delete"));
    grid->attach(*label1, 0, 0);
    grid->attach(*label2, 1, 0);
    grid->attach(*label3, 2, 0);
    grid->attach(*label4, 3, 0);
    grid->show_all_children(true);
}

void grid_add_row(Gtk::Grid *grid, int row_index, std::string key, std::string cmd) {
    FUN_INFO("add row %d, %s, %s", row_index, key.c_str(), cmd.c_str());
    auto entry1 = Gtk::manage(new Gtk::Entry());
    entry1->set_editable();
    entry1->set_text(key.c_str());
    grid->attach(*entry1, 0, row_index, 1, 1);
    entry1->show();
    auto entry2 = Gtk::manage(new Gtk::Entry());
    entry2->set_editable();
    entry2->set_text(cmd.c_str());
    grid->attach(*entry2, 1, row_index, 1, 1);
    entry2->show();

    auto button = Gtk::manage(new Gtk::Button(_("checkResult")));
    grid->attach(*button, 2, row_index, 1, 1);
    const Gtk::Entry *cmd_entry = entry2;
    button->signal_clicked().connect([=]() {
        FUN_INFO("%s", cmd_entry->get_text().c_str());
        auto text = cmd_entry->get_text();
        std::string s = text;
        FUN_INFO("text %s", s.c_str());
        GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
        auto dialog =
            gtk_message_dialog_new(nullptr, flags, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "%s", exec(s.c_str()).c_str());
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    });
    button->show();

    auto delete_button = Gtk::manage(new Gtk::Button(_("delete")));
    grid->attach(*delete_button, 3, row_index);
    const int i = row_index;
    delete_button->signal_clicked().connect(
        [entry1, entry2, button, delete_button, grid]() {
            grid->remove(*entry1);
            grid->remove(*entry2);
            grid->remove(*button);
            grid->remove(*delete_button);
        },
        delete_button);

    delete_button->show();
}

Gtk::Grid *grid = nullptr;
void set_fast_input_export_handler(Glib::RefPtr<Gtk::Builder> &builder);
void set_fast_input_import_handler(Glib::RefPtr<Gtk::Builder> &builder);
void set_fast_input_clear_handler(Glib::RefPtr<Gtk::Builder> &builder);
void setup_fast_input_handlers(Glib::RefPtr<Gtk::Builder> builder) {
    builder->get_widget<Gtk::Grid>("fast_input_grid", grid);

    grid_set_header(grid);
    // refresh contents
    auto m = load_fast_input_config();
    FUN_INFO("loaded configuration file, %d items", m.size());
    auto num_rows = grid_get_num_rows(grid);
    for (const auto &[key, cmd] : m) {
        grid_add_row(grid, num_rows, key, cmd);
        num_rows++;
    }
    // if no data there, add an empty row
    if (num_rows == 1) {
        grid_add_row(grid, num_rows, "", "");
        num_rows = 2;
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

    set_fast_input_clear_handler(builder);

    set_fast_input_import_handler(builder);

    set_fast_input_export_handler(builder);
}
void set_fast_input_clear_handler(Glib::RefPtr<Gtk::Builder> &builder) {
    Gtk::Button *clear_button;
    builder->get_widget<Gtk::Button>("fast_input_clear_button", clear_button);
    clear_button->signal_clicked().connect([=]() {
        auto num_rows = grid_get_num_rows(grid);
        for (int i = 1; i < num_rows; i++) {
            // row 0 contains labels that should be kept
            // row 2 becomes row 1 when row 1 is deleted, so always remove row 1
            grid->remove_row(1);
        }
        // add an empty row for input
        grid_add_row(grid, 1, "", "");
    });
}
void set_fast_input_import_handler(Glib::RefPtr<Gtk::Builder> &builder) {
    Gtk::Button *import_button;
    builder->get_widget<Gtk::Button>("fast_input_import_button", import_button);
    import_button->signal_clicked().connect([=]() {
        auto dialog = new Gtk::FileChooserDialog("Please choose a file", Gtk::FILE_CHOOSER_ACTION_OPEN);
        dialog->set_transient_for(*win);
        dialog->set_modal(true);
        dialog->signal_response().connect([=](int response_id) {
            // Handle the response:
            switch (response_id) {
                case Gtk::RESPONSE_OK: {

                    // Notice that this is a std::string, not a Glib::ustring.
                    auto openedfilename = dialog->get_file()->get_path();
                    FUN_INFO("opening file %s", openedfilename.c_str());
                    auto m = load_fast_input_config(openedfilename);
                    auto num_rows = grid_get_num_rows(grid);
                    for (const auto &[k, v] : m) {
                        grid_add_row(grid, num_rows, k, v);
                        num_rows++;
                    }

                    break;
                }
                case Gtk::RESPONSE_CANCEL: {
                    std::cout << "Cancel clicked." << std::endl;
                    break;
                }
                default: {
                    std::cout << "Unexpected button clicked." << std::endl;
                    break;
                }
            }
            delete dialog;
        });

        // Add response buttons to the dialog:
        dialog->add_button("_Cancel", Gtk::RESPONSE_CANCEL);
        dialog->add_button("_Open", Gtk::RESPONSE_OK);

        // Add filters, so that only certain file types can be selected:

        auto filter_text = Gtk::FileFilter::create();
        filter_text->set_name("Text files");
        filter_text->add_mime_type("text/plain");
        dialog->add_filter(filter_text);

        auto filter_cpp = Gtk::FileFilter::create();
        filter_cpp->set_name("Json files");
        filter_cpp->add_mime_type("application/json");
        filter_cpp->add_mime_type("plain/json");
        dialog->add_filter(filter_cpp);

        auto filter_any = Gtk::FileFilter::create();
        filter_any->set_name("Any files");
        filter_any->add_pattern("*");
        dialog->add_filter(filter_any);

        // Show the dialog and wait for a user response:
        dialog->show();
    });
}
void set_fast_input_export_handler(Glib::RefPtr<Gtk::Builder> &builder) {
    Gtk::Button *export_button;
    builder->get_widget<Gtk::Button>("fast_input_export_button", export_button);
    export_button->signal_clicked().connect([=]() {
        auto dialog = new Gtk::FileChooserDialog("Please choose a file", Gtk::FILE_CHOOSER_ACTION_SAVE);
        dialog->set_transient_for(*win);
        dialog->set_modal(true);
        dialog->signal_response().connect([=](int response_id) {
            // Handle the response:
            switch (response_id) {
                case Gtk::RESPONSE_OK: {

                    // Notice that this is a std::string, not a Glib::ustring.
                    auto openedfilename = dialog->get_file()->get_path();
                    FUN_INFO("saving to file %s", openedfilename.c_str());
                    save_fast_input_config(grid, openedfilename);
                    break;
                }
                case Gtk::RESPONSE_CANCEL: {
                    std::cout << "Cancel clicked." << std::endl;
                    break;
                }
                default: {
                    std::cout << "Unexpected button clicked." << std::endl;
                    break;
                }
            }
            delete dialog;
        });

        // Add response buttons to the dialog:
        dialog->add_button("_Cancel", Gtk::RESPONSE_CANCEL);
        dialog->add_button("_Save", Gtk::RESPONSE_OK);

        // Add filters, so that only certain file types can be selected:

        auto filter_text = Gtk::FileFilter::create();
        filter_text->set_name("Text files");
        filter_text->add_mime_type("text/plain");
        dialog->add_filter(filter_text);

        auto filter_cpp = Gtk::FileFilter::create();
        filter_cpp->set_name("Json files");
        filter_cpp->add_mime_type("application/json");
        filter_cpp->add_mime_type("plain/json");
        dialog->add_filter(filter_cpp);

        auto filter_any = Gtk::FileFilter::create();
        filter_any->set_name("Any files");
        filter_any->add_pattern("*");
        dialog->add_filter(filter_any);

        // Show the dialog and wait for a user response:
        dialog->show();
    });
}

int main(int argc, char *argv[]) {
    app = Gtk::Application::create("fun.xjbcode.ibus-fun.setup");
    builder = Gtk::Builder::create_from_file("/usr/share/ibus/ibus-fun/data/ibus_fun_setup.glade");

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

    setup_fast_input_handlers(builder);

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
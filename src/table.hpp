#pragma once

#include <iostream>

#include <boost/spirit/include/qi.hpp>

#include <gtkmm/columnview.h>
#include <gtkmm/columnviewcolumn.h>
#include <gtkmm/box.h>
#include <gtkmm/entry.h>
#include <gtkmm/button.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/filechooserdialog.h>

class ExcaliburWindow : public Gtk::ApplicationWindow {
public:
    ExcaliburWindow();
    ~ExcaliburWindow() override {};

    void on_action_witnesses_file_open() {
        auto dialog = new Gtk::FileChooserDialog(*this, "Open witnesses file");

        dialog->set_transient_for(*this);
        dialog->set_modal(true);
        dialog->signal_response().connect(sigc::bind(
                    sigc::mem_fun(*this, &ExcaliburWindow::on_witnesses_file_dialog_response), dialog));

        dialog->add_button("_Cancel", Gtk::ResponseType::CANCEL);
        dialog->add_button("_Open", Gtk::ResponseType::OK);

        auto filter_any = Gtk::FileFilter::create();
        filter_any->set_name("Any files");
        filter_any->add_pattern("*");
        dialog->add_filter(filter_any);

        dialog->show();
    }

    void on_witnesses_file_dialog_response(int response_id, Gtk::FileChooserDialog* dialog) {
        std::cout << "Annie <3" << std::endl;
        delete dialog;
    }

    /*void on_witnesses_file_choice() {
        std::cout << "callback" << std::endl;
        auto file = dialog.open_finish();
        std::cout << typeid(file).name() << std::endl;
    }*/
protected:
    Gtk::Entry element_entry;
    Gtk::Box vbox;
    Gtk::ColumnView witnesses;
    Gtk::Button open_button, save_button;
};

ExcaliburWindow::ExcaliburWindow() : witnesses(), element_entry(), vbox(),
                                     open_button("Open"), save_button("Save") {
    set_title("Excalibur Circuit Viewer: pull the bugs from the stone");
    set_resizable(true);
    vbox.set_spacing(10);

    auto css_provider = Gtk::CssProvider::create();
    Glib::ustring css_style = "* { font: 24px Arial; }";
    css_provider->load_from_data(css_style);
    auto context = get_style_context();
    context->add_provider(css_provider, GTK_STYLE_PROVIDER_PRIORITY_USER);

    element_entry.set_placeholder_text("00000000000000000000000000000000000000000000000000000000000000");
    element_entry.set_max_length(64);
    vbox.append(open_button);
    vbox.append(save_button);
    vbox.append(element_entry);
    vbox.append(witnesses);
    set_child(vbox);

    open_button.signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &ExcaliburWindow::on_action_witnesses_file_open)));
}

// MIT License
//
// Copyright (c) 2023 Dmitrii Tabalin <d.tabalin@nil.foundation>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

//#define BOOST_SPIRIT_DEBUG

#include <cstdint>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <sstream>
#include <vector>
#include <map>

#include <boost/spirit/include/qi.hpp>
#include <boost/phoenix/phoenix.hpp>

#include <giomm/liststore.h>

#include <glibmm/value.h>

#include <pangomm/layout.h>

#include <glibmm/property.h>

#include <gtkmm/enums.h>
#include <gtkmm/label.h>
#include <gtkmm/columnview.h>
#include <gtkmm/columnviewcolumn.h>
#include <gtkmm/box.h>
#include <gtkmm/entry.h>
#include <gtkmm/button.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/filedialog.h>
#include <gtkmm/listitemfactory.h>
#include <gtkmm/listitem.h>
#include <gtkmm/textview.h>
#include <gtkmm/signallistitemfactory.h>
#include <gtkmm/noselection.h>
#include <gtkmm/styleprovider.h>
#include <gtkmm/eventcontrollerkey.h>

#include <nil/crypto3/zk/snark/arithmetization/plonk/gate.hpp>
#include <nil/crypto3/zk/snark/arithmetization/plonk/copy_constraint.hpp>
#include <nil/crypto3/zk/snark/arithmetization/plonk/constraint.hpp>
#include <nil/crypto3/zk/math/expression.hpp>
#include <nil/crypto3/zk/snark/arithmetization/plonk/variable.hpp>

#include "parsers.hpp"

template<typename BlueprintFieldType>
struct circuit_container {
    using plonk_constraint_type = nil::crypto3::zk::snark::plonk_constraint<BlueprintFieldType>;
    using gate_type = nil::crypto3::zk::snark::plonk_gate<BlueprintFieldType, plonk_constraint_type>;
    using plonk_copy_constraint_type = nil::crypto3::zk::snark::plonk_copy_constraint<BlueprintFieldType>;

    circuit_sizes sizes;
    std::map<std::size_t, gate_type> gates;
    std::vector<plonk_copy_constraint_type> copy_constraints;
    // Todo: add lookup gates
};

std::string read_line_from_gstream(Glib::RefPtr<Gio::FileInputStream> stream,
                                   gsize predicted_line_size,
                                   gsize file_size,
                                   char* buffer) {
    std::string line;
    auto pos = stream->tell();
    gsize total_read_size = pos;
    if (pos + predicted_line_size >= file_size) {
        predicted_line_size = file_size - pos;
    }
    char* newline = nullptr;
    while (newline == nullptr && total_read_size != file_size) {
        auto read_size = stream->read(buffer, predicted_line_size);
        total_read_size += read_size;
        buffer[read_size] = '\0';
        if (read_size != predicted_line_size && total_read_size != file_size) {
            return std::string("");
        }
        newline = strchr(buffer, '\n');
        if (newline != nullptr) {
            *newline = '\0';
            stream->seek(total_read_size + (newline - buffer) + 1 - read_size, Glib::SeekType::SET);
        }
        line += buffer;
    }

    return line;
}

// Use this to debug in case you have no idea where a widget is
void print_widget_hierarchy(const Gtk::Widget& widget, int depth = 0) {
    std::string indent(depth * 2, ' '); // Indentation based on depth
    std::cout << indent << widget.get_name() << std::endl;

    const Gtk::Widget* child = widget.get_first_child();
    while(child != nullptr) {
        print_widget_hierarchy(*child, depth + 1);
        child = child->get_next_sibling();
    }
}


void print_list_item_hierarchy(const Gtk::ListItem &list_item) {
    const Gtk::Widget* child = list_item.get_child();
    while(child != nullptr) {
        print_widget_hierarchy(*child, 1);
        child = child->get_next_sibling();
    }
}

struct CellState {
    CellState() : state(CellStateFlags::NORMAL) {}
    CellState(uint8_t state_) : state(state_) {}
    enum CellStateFlags : uint8_t {
        NORMAL = 0,
        SELECTED = 1 << 1
    };

    void select() {
        state |= SELECTED;
    }

    void unselect() {
        state &= ~SELECTED;
    }

    bool is_selected() const {
        return state & SELECTED;
    }

    uint8_t state;
};

template <typename BlueprintFieldType>
class RowObject : public Glib::Object {
public:

    using value_type = typename BlueprintFieldType::value_type;
    using integral_type = typename BlueprintFieldType::integral_type;

    static Glib::RefPtr<RowObject> create(const std::vector<integral_type>& row_, std::size_t row_index_) {
        return Glib::make_refptr_for_instance<RowObject>(new RowObject(row_, row_index_));
    }

    const Glib::ustring& to_string(std::size_t index) const {
        return string_cache[index];
    }

    std::size_t get_row_index() const {
        return row_index;
    }

    const value_type get_row_item(std::size_t column_index) const {
        return row[column_index];
    }

    void set_row_item(const value_type& v, std::size_t column_index) {
        row[column_index] = v;

        std::stringstream ss;
        if (column_index != 0) {
            ss << std::hex;
        } else {
            ss << std::dec;
        }
        ss << row[column_index].data;
        string_cache[column_index] = ss.str();
    }

    void set_cell_state(std::size_t column_index, CellState state) {
        cell_states[column_index] = state;
    }

    CellState get_cell_state(std::size_t column_index) const {
        return cell_states[column_index];
    }

    CellState& get_cell_state(std::size_t column_index) {
        return cell_states[column_index];
    }

    void set_widget(std::size_t column_index, Gtk::Button* widget) {
        widgets[column_index] = widget;
    }

    Gtk::Button* get_widget(std::size_t column_index) const {
        return widgets[column_index];
    }

    bool get_widget_loaded(std::size_t column_index) const {
        return widget_loaded[column_index];
    }

    void set_widget_loaded(std::size_t column_index, bool loaded) {
        widget_loaded[column_index] = loaded;
    }

protected:
    RowObject(const std::vector<integral_type>& row_, std::size_t row_index_) :
            row_index(row_index_), cell_states(row_.size(), CellState::CellStateFlags::NORMAL),
            widgets(row_.size(), nullptr), widget_loaded(row_.size(), false) {
        row.reserve(row_.size());
        std::copy(row_.begin(), row_.end(), std::back_inserter(row));
        string_cache.reserve(row.size());

        for (std::size_t i = 0; i < row.size(); ++i) {
            std::stringstream ss;
            if (i != 0) {
                ss << std::hex;
            } else {
                ss << std::dec;
            }
            ss << row[i].data;
            string_cache.push_back(ss.str());
        }
    }
private:
    std::vector<Glib::ustring> string_cache;
    std::vector<value_type> row;
    std::size_t row_index;
    std::vector<CellState> cell_states;
    std::vector<Gtk::Button*> widgets;
    std::vector<bool> widget_loaded;
};

template<typename WidgetType, typename BlueprintFieldType>
struct CellTracker {
    CellTracker() : row(-1), column(-1), row_object(nullptr) {}

    RowObject<BlueprintFieldType>* row_object;
    std::size_t row, column;
};


template<typename BlueprintFieldType>
class ExcaliburWindow : public Gtk::ApplicationWindow {
public:
    using integral_type = typename BlueprintFieldType::integral_type;
    using value_type = typename BlueprintFieldType::value_type;

    ExcaliburWindow() : table(), element_entry(), vbox_prime(), table_window(), vbox_controls(),
                        open_table_button("Open Table"), save_table_button("Save"),
                        open_circuit_button("Open Circuit") {
        set_title("Excalibur Circuit Viewer: pull the bugs from the stone");
        set_resizable(true);

        auto css_provider = Gtk::CssProvider::create();
        Glib::ustring css_style =
            "* { font: 24px Courier; border-radius: unset }"
            "button { margin: 0px; padding: 0px; }"
            "button.selected { background: deepskyblue; }"
            "button.crimson { background: crimson; }";
        css_provider->load_from_data(css_style);
        Gtk::StyleProvider::add_provider_for_display(
            Gdk::Display::get_default(), css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

        element_entry.set_placeholder_text("00000000000000000000000000000000000000000000000000000000000000");
        element_entry.set_max_length(64);

        vbox_prime.set_orientation(Gtk::Orientation::VERTICAL);
        vbox_prime.set_spacing(10);

        vbox_controls.set_spacing(10);
        vbox_controls.set_orientation(Gtk::Orientation::HORIZONTAL);
        vbox_controls.append(open_table_button);
        vbox_controls.append(save_table_button);
        vbox_controls.append(open_circuit_button);
        vbox_controls.append(element_entry);
        vbox_prime.append(vbox_controls);

        table_window.set_child(table);
        table_window.set_size_request(800, 600);
        table_window.set_vexpand(true);
        vbox_prime.append(table_window);
        vbox_prime.set_vexpand(true);

        set_child(vbox_prime);

        auto key_controller = Gtk::EventControllerKey::create();
        // This is a hack: I have no idea how to catch the key press event.
        key_controller->signal_key_released().connect(
            sigc::mem_fun(*this, &ExcaliburWindow::on_entry_key_released), true);
        element_entry.add_controller(key_controller);

        open_table_button.signal_clicked().connect(sigc::mem_fun(*this, &ExcaliburWindow::on_action_table_file_open));
        open_circuit_button.signal_clicked().connect(
            sigc::mem_fun(*this, &ExcaliburWindow::on_action_circuit_file_open));
        save_table_button.signal_clicked().connect(
            sigc::bind<0>(sigc::mem_fun(*this, &ExcaliburWindow::on_action_table_file_save), false));
    }

    ~ExcaliburWindow() override {};

    void on_action_table_file_open() {
        auto file_dialog = Gtk::FileDialog::create();
        file_dialog->set_modal(true);
        file_dialog->set_title("Open table file");
        file_dialog->open(*this,
            sigc::bind<0>(sigc::mem_fun(*this,
                                        &ExcaliburWindow::on_table_file_open_dialog_response),
                          file_dialog));
    }

    void on_action_circuit_file_open() {
        auto file_dialog = Gtk::FileDialog::create();
        file_dialog->set_modal(true);
        file_dialog->set_title("Open circuit file");
        file_dialog->open(*this,
            sigc::bind<0>(sigc::mem_fun(*this,
                                        &ExcaliburWindow::on_circuit_file_open_dialog_response),
                          file_dialog));
    }

    void on_action_table_file_save(bool wide_export) {
        auto file_dialog = Gtk::FileDialog::create();
        file_dialog->set_modal(true);
        file_dialog->set_title("Save table file");
        file_dialog->save(*this,
            sigc::bind<0>(sigc::bind<0>(sigc::mem_fun(*this, &ExcaliburWindow::on_table_file_save_dialog_response),
                                        file_dialog),
                          wide_export));
    }

    void on_setup_column_item(std::size_t column, const Glib::RefPtr<Gtk::ListItem> &list_item) {
        auto button = Gtk::make_managed<Gtk::Button>();
        auto label = Gtk::make_managed<Gtk::Label>();

        label->set_ellipsize(Pango::EllipsizeMode::START);
        label->set_halign(Gtk::Align::END);
        button->set_child(*label);
        list_item->set_child(*button);

        if (column == 0) {
            return;
        }
        button->signal_clicked().connect(
            sigc::bind<0>(sigc::bind<0>(
                sigc::mem_fun(*this, &ExcaliburWindow::on_cell_clicked),
                                        column),
                          list_item));
    }

    void on_bind_column_item(std::size_t column, const Glib::RefPtr<Gtk::ListItem> &list_item) {
        auto button = dynamic_cast<Gtk::Button*>(list_item->get_child());
        if (!button) {
            return;
        }
        auto label = dynamic_cast<Gtk::Label*>(button->get_child());
        if (!label) {
            return;
        }

        auto item = list_item->get_item();
        auto mitem = dynamic_cast<RowObject<BlueprintFieldType>*>(&*item);
        if (!mitem) {
            return;
        }
        mitem->set_widget(column, button);
        mitem->set_widget_loaded(column, true);
        label->set_text(mitem->to_string(column));
        CellState state = mitem->get_cell_state(column);
        if (state.is_selected()) {
            button->add_css_class("selected");
        }
    }

    void on_unbind_column_item(std::size_t column, const Glib::RefPtr<Gtk::ListItem> &list_item) {
        auto item = list_item->get_item();
        auto mitem = dynamic_cast<RowObject<BlueprintFieldType>*>(&*item);
        if (!mitem) {
            return;
        }
        mitem->set_widget_loaded(column, false);
    }

    void on_table_file_open_dialog_response(Glib::RefPtr<Gtk::FileDialog> file_dialog,
                                            std::shared_ptr<Gio::AsyncResult> &res) {
        auto result = file_dialog->open_finish(res);
        auto stream = result->read();
        auto file_info = result->query_info();
        auto file_size = file_info->get_size();

        using boost::spirit::qi::phrase_parse;
        // 200 should be enough for the first row
        const std::size_t first_line_size = file_size < 200 ? file_size : 200;
        char* buffer = new char[first_line_size + 1];

        std::string first_line = read_line_from_gstream(stream, first_line_size, file_size, buffer);
        if (first_line.empty()) {
            std::cerr << "Failed to read the header line." << std::endl;
            delete[] buffer;
            return;
        }

        table_sizes_parser<decltype(first_line.begin())> sizes_parser;
        auto first_line_begin = first_line.begin();

        bool r = phrase_parse(first_line_begin, first_line.end(), sizes_parser, boost::spirit::ascii::space, sizes);
        if (!r || first_line_begin != first_line.end()) {
            std::cerr << "Failed to parse the header line." << std::endl;
            delete[] buffer;
            return;
        }

        delete[] buffer;

        auto predicted_line_size = (file_size - first_line.size()) / sizes.max_size * 2;
        buffer = new char[predicted_line_size + 1];
        table_row_parser<decltype(first_line.begin()), BlueprintFieldType> row_parser(sizes);

        auto store = Gio::ListStore<RowObject<BlueprintFieldType>>::create();

        for (std::uint32_t i = 0; i < sizes.max_size; i++) {
            std::string line = read_line_from_gstream(stream, predicted_line_size, file_size, buffer);
            if (line.empty()) {
                std::cerr << "Failed to read line " << i + 1 << " of the file" << std::endl;
                delete[] buffer;
                return;
            }

            auto line_begin = line.begin();
            std::vector<integral_type> row;
            row.push_back(i);
            r = phrase_parse(line_begin, line.end(), row_parser, boost::spirit::ascii::space, row);
            if (!r || line_begin != line.end()) {
                std::cerr << "Failed to parse line " << i + 1 << " of the file" << std::endl;
                delete[] buffer;
                return;
            }

            store->append(RowObject<BlueprintFieldType>::create(row, i));
        }
        std::cout << "Successfully parsed the file" << std::endl;
        delete[] buffer;
        stream->close();

        std::size_t column_num = 0,
                    column_size = sizes.witnesses_size + sizes.public_inputs_size +
                                  sizes.constants_size + sizes.selectors_size;

        auto get_column_name = [](const table_sizes &sizes, std::size_t i) {
            if (i == 0) {
                return std::string("Row");
            }
            std::stringstream ss;
            auto fixed_width_size = [&ss](std::size_t j) {
                ss << std::setfill('0') << std::setw(4) << j;
                return ss.str();
            };

            if (i < sizes.witnesses_size + 1) {
                return "W" + fixed_width_size(i);
            } else if (i < sizes.witnesses_size + sizes.public_inputs_size + 1) {
                return "P" + fixed_width_size(i - sizes.witnesses_size - 1);
            } else if (i < sizes.witnesses_size + sizes.public_inputs_size + sizes.constants_size + 1) {
                return "C" + fixed_width_size(i - sizes.witnesses_size - sizes.public_inputs_size - 1);
            } else {
                return "S" + fixed_width_size(i - sizes.witnesses_size - sizes.public_inputs_size
                                                - sizes.constants_size - 1);
            }

        };
        for (std::size_t i = 0; i < column_size + 1; i++) {
            auto factory = Gtk::SignalListItemFactory::create();
            factory->signal_setup().connect(
                sigc::bind<0>(sigc::mem_fun(*this, &ExcaliburWindow::on_setup_column_item), i));
            factory->signal_bind().connect(
                sigc::bind<0>(sigc::mem_fun(*this, &ExcaliburWindow::on_bind_column_item), i));
            factory->signal_unbind().connect(
                sigc::bind<0>(sigc::mem_fun(*this, &ExcaliburWindow::on_unbind_column_item), i));

            auto column = Gtk::ColumnViewColumn::create(get_column_name(sizes, i), factory);
            column->set_resizable(true);
            table.append_column(column);
        }

        auto model = Gtk::NoSelection::create(store);
        table.set_model(model);
    }

    void on_circuit_file_open_dialog_response(Glib::RefPtr<Gtk::FileDialog> file_dialog,
                                              std::shared_ptr<Gio::AsyncResult> &res) {
        using plonk_constraint_type = nil::crypto3::zk::snark::plonk_constraint<BlueprintFieldType>;

        auto result = file_dialog->open_finish(res);
        auto stream = result->read();
        auto file_info = result->query_info();
        auto file_size = file_info->get_size();

        using boost::spirit::qi::phrase_parse;
        // 200 should be enough for the first row
        const std::size_t first_line_size = file_size < 200 ? file_size : 200;
        char* buffer = new char[first_line_size + 1];

        std::string first_line = read_line_from_gstream(stream, first_line_size, file_size, buffer);
        if (first_line.empty()) {
            std::cerr << "Failed to read the header line." << std::endl;
            delete[] buffer;
            return;
        }

        circuit_sizes_parser<decltype(first_line.begin())> sizes_parser;
        auto first_line_begin = first_line.begin();

        bool r = phrase_parse(first_line_begin, first_line.end(), sizes_parser, boost::spirit::ascii::space,
                              circuit.sizes);
        if (!r || first_line_begin != first_line.end()) {
            std::cerr << "Failed to parse the header line." << std::endl;
            delete[] buffer;
            return;
        }

        delete[] buffer;

        auto predicted_line_size = (file_size - first_line.size()) / circuit.sizes.gates_size;
        buffer = new char[predicted_line_size + 1];
        for (std::uint32_t i = 0; i < circuit.sizes.gates_size; i++) {
            std::string line = read_line_from_gstream(stream, predicted_line_size, file_size, buffer);
            if (line.empty()) {
                std::cerr << "Failed to header line for " << i + 1 << "'th gate of the file" << std::endl;
                delete[] buffer;
                return;
            }
            gate_header gate_header;
            auto line_begin = line.begin();
            std::cout << line << std::endl;
            gate_header_parser<decltype(line.begin())> header_parser;
            r = phrase_parse(line_begin, line.end(), header_parser, boost::spirit::ascii::space, gate_header);
            if (!r || line_begin != line.end()) {
                std::cerr << "Failed to parse gate header for " << i + 1 << "'th gate of the file" << std::endl;
                delete[] buffer;
                return;
            }

            gate_constraint_parser<decltype(line.begin()), BlueprintFieldType> constraint_parser;
            for (std::size_t j = 0; j < gate_header.constraints_size; j++) {
                plonk_constraint_type constraint;
                line = read_line_from_gstream(stream, predicted_line_size, file_size, buffer);
                if (line.empty()) {
                    std::cerr << "Failed to read line for" << j << "'th constraint for" << i << "'th gate of the file"
                              << std::endl;
                    delete[] buffer;
                    return;
                }
                std::cout << line << std::endl;
                line_begin = line.begin();
                r = phrase_parse(line_begin, line.end(), constraint_parser, boost::spirit::ascii::space, constraint);
                if (!r || line_begin != line.end()) {
                    std::cerr << "Failed to parse gate constraint " << j + 1 << " for " << i + 1
                              << "'th gate of the file" << std::endl;
                    delete[] buffer;
                    return;
                }
                std::cout << constraint << std::endl;
            }
        }

        delete[] buffer;
    }

    void on_table_file_save_dialog_response(Glib::RefPtr<Gtk::FileDialog> file_dialog,
                                            bool wide_export,
                                            std::shared_ptr<Gio::AsyncResult> &res) {
        auto result = file_dialog->save_finish(res);
        auto stream = result->replace();
        if (stream->is_closed()) {
            std::cerr << "Failed to open the file for writing" << std::endl;
            return;
        }
        // Write the header
        std::stringstream header;
        header << "witnesses_size: " << sizes.witnesses_size << " public_inputs_size: " << sizes.public_inputs_size
               << " constants_size: " << sizes.constants_size << " selectors_size: " << sizes.selectors_size
               << " max_size: " << sizes.max_size << "\n";
        stream->write(header.str().c_str(), header.str().size());
        auto model = dynamic_cast<Gtk::NoSelection*>(&*table.get_model());
        if (!model) {
            std::cout << "No model" << std::endl;
            stream->close();
            return;
        }
        auto list_model = model->get_model();
        std::uint32_t width = wide_export ? (BlueprintFieldType::modulus_bits + 4 - 1) / 4 : 0;
        std::size_t hex_sizes = sizes.witnesses_size + sizes.public_inputs_size + sizes.constants_size;
        for (std::size_t i = 0; i < sizes.max_size; i++) {
            std::stringstream row_stream;
            row_stream << std::hex << std::setfill('0');
            auto object_row = model->get_object(i);
            if (!object_row) {
                std::cout << "No object" << std::endl;
                stream->close();
                return;
            }
            auto row = dynamic_cast<RowObject<BlueprintFieldType>*>(&*object_row);
            if (!row) {
                std::cout << "No row" << std::endl;
                stream->close();
                return;
            }
            std::size_t curr_idx = 1;
            for (std::size_t j = 0; j < sizes.witnesses_size; j++) {
                row_stream << std::setw(width) << row->get_row_item(curr_idx++).data << " ";
            }
            row_stream << "| ";
            for (std::size_t j = 0; j < sizes.public_inputs_size; j++) {
                row_stream << std::setw(width) << row->get_row_item(curr_idx++).data << " ";
            }
            row_stream << "| ";
            for (std::size_t j = 0; j < sizes.constants_size; j++) {
                row_stream << std::setw(width)
                           << row->get_row_item(curr_idx++).data
                           << " ";
            }
            row_stream << "| ";
            for (std::size_t j = 0; j < sizes.selectors_size - 1; j++) {
                row_stream << row->get_row_item(curr_idx++).data << " ";
            }
            row_stream << row->get_row_item(curr_idx).data << "\n";
            stream->write(row_stream.str().c_str(), row_stream.str().size());
        }
        stream->close();
    }

    void on_cell_clicked(std::size_t column, const Glib::RefPtr<Gtk::ListItem> &list_item) {
        auto item = list_item->get_item();
        auto mitem = dynamic_cast<RowObject<BlueprintFieldType>*>(&*item);
        if (!mitem) {
            return;
        }
        std::size_t row = mitem->get_row_index();
        auto button = dynamic_cast<Gtk::Button*>(&*list_item->get_child());

        if (selected_cell.row == row && selected_cell.column == column || button == nullptr) {
            return;
        }

        if (selected_cell.row_object != nullptr) {
            auto old_row = selected_cell.row_object;
            CellState& old_row_state = old_row->get_cell_state(selected_cell.column);
            old_row_state.unselect();
            if (old_row->get_widget_loaded(selected_cell.column)) {
                auto button = old_row->get_widget(selected_cell.column);
                button->remove_css_class("selected");
            }
        }

        selected_cell.row = row;
        selected_cell.column = column;
        selected_cell.row_object = mitem;

        button->add_css_class("selected");
        CellState& row_state = mitem->get_cell_state(column);
        row_state.select();

        element_entry.set_text(mitem->to_string(column));
    }

    void on_entry_key_released(guint keyval, guint keycode, Gdk::ModifierType state) {
        // Didn't select any cell or keyval != enter
        if (keyval != 65293 || selected_cell.row_object == nullptr) {
            return;
        }
        std::stringstream ss;
        ss << std::hex << element_entry.get_text();
        integral_type integral_value;
        ss >> integral_value;
        if (ss.fail()) {
            std::cerr << "Failed to parse the value" << std::endl;
            return;
        }
        value_type value = integral_value;
        auto selection_model = table.get_model();
        auto model = dynamic_cast<Gtk::NoSelection*>(&*selection_model);
        if (!model) {
            std::cout << "No model" << std::endl;
            return;
        }

        auto object_row = model->get_object(selected_cell.row);
        if (!object_row) {
            std::cout << "No object" << std::endl;
            return;
        }
        auto row = dynamic_cast<RowObject<BlueprintFieldType>*>(&*object_row);
        if (!row) {
            std::cout << "Failed cast to row" << std::endl;
            return;
        }
        row->set_row_item(value, selected_cell.column);

        if (row->get_widget_loaded(selected_cell.column)) {
            auto button = row->get_widget(selected_cell.column);
            auto label = dynamic_cast<Gtk::Label*>(&*button->get_child());
            if (!label) {
                std::cout << "Failed cast to label" << std::endl;
                return;
            }
            label->set_text(element_entry.get_text());
        }
    }

protected:
    Gtk::Entry element_entry;
    Gtk::Box vbox_prime, vbox_controls;
    Gtk::ScrolledWindow table_window;
    Gtk::ColumnView table;
    Gtk::Button open_table_button, open_circuit_button, save_table_button;
private:
    table_sizes sizes;
    CellTracker<Gtk::Button, BlueprintFieldType> selected_cell;
    circuit_container<BlueprintFieldType> circuit;
};

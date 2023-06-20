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
#include <memory>
#include <sstream>
#include <vector>
#include <utility>

#include <boost/spirit/include/qi.hpp>
#include <boost/phoenix/phoenix.hpp>
#include <boost/variant.hpp>

#include <giomm/liststore.h>

#include <glibmm/value.h>

#include <pangomm/layout.h>

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
#include <gtkmm/listview.h>
#include <gtkmm/selectionmodel.h>

#include <nil/crypto3/zk/snark/arithmetization/plonk/gate.hpp>
#include <nil/crypto3/zk/snark/arithmetization/plonk/copy_constraint.hpp>
#include <nil/crypto3/zk/snark/arithmetization/plonk/constraint.hpp>
#include <nil/crypto3/zk/math/expression.hpp>
#include <nil/crypto3/zk/snark/arithmetization/plonk/variable.hpp>

#include "parsers.hpp"


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

    void deselect() {
        state &= ~SELECTED;
    }

    bool is_selected() const {
        return state & SELECTED;
    }

    uint8_t state;
};

template <typename BlueprintFieldType>
class row_object : public Glib::Object {
public:
    // We have to roll a custom container for this because ArithmetizationParams are constexpr in the assignment table.
    using value_type = typename BlueprintFieldType::value_type;
    using integral_type = typename BlueprintFieldType::integral_type;
    using plonk_copy_constraint_type = nil::crypto3::zk::snark::plonk_copy_constraint<BlueprintFieldType>;
    using plonk_constraint_type = nil::crypto3::zk::snark::plonk_constraint<BlueprintFieldType>;
    using var = nil::crypto3::zk::snark::plonk_variable<BlueprintFieldType>;

    static Glib::RefPtr<row_object> create(const std::vector<integral_type>& row_, std::size_t row_index_) {
        return Glib::make_refptr_for_instance<row_object>(new row_object(row_, row_index_));
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

    static std::size_t get_actual_column_index(var variable, table_sizes &sizes) {
        switch (variable.type) {
            case var::column_type::witness:
                return variable.index + 1;
                break;
            case var::column_type::public_input:
                return variable.index + 1 + sizes.witnesses_size;
                break;
            case var::column_type::constant:
                return variable.index + 1 + sizes.witnesses_size + sizes.public_inputs_size;
                break;
            default:
                std::cerr << "Attempted to add copy constraint to selector column" << std::endl;
                return -1;
        }
    }

    void add_copy_constraint_to_cache(var variable, std::size_t constraint_num,
                                      plonk_copy_constraint_type* constraint,
                                      table_sizes &sizes) {
        if (variable.rotation != row_index) {
            std::cerr << "Attempted to add copy constraint to wrong row" << std::endl;
            return;
        }
        std::size_t actual_column_index = get_actual_column_index(variable, sizes);
        copy_constraints_cache[actual_column_index].push_back(std::make_pair(constraint_num, constraint));
    }

    std::size_t get_copy_constraints_size(std::size_t column_index) const {
        return copy_constraints_cache[column_index].size();
    }

    std::pair<std::size_t, plonk_copy_constraint_type*> get_copy_constraint(std::size_t column_index,
                                                                            std::size_t index) const {
        return copy_constraints_cache[column_index][index];
    }

protected:
    row_object(const std::vector<integral_type>& row_, std::size_t row_index_) :
            row_index(row_index_), cell_states(row_.size(), CellState::CellStateFlags::NORMAL),
            widgets(row_.size(), nullptr), widget_loaded(row_.size(), false),
            copy_constraints_cache(row_.size()),
            constraints_cache(row_.size(), std::vector<std::pair<std::size_t, plonk_constraint_type*>>(0)) {
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
    // Stores all copy constraints which affect the i'th item
    // The size element in pair is for constraint num in container.
    std::vector<std::vector<std::pair<std::size_t, plonk_copy_constraint_type*>>> copy_constraints_cache;
    // Stores all constraints which affect the i'th item, with their selectors
    std::vector<std::vector<std::pair<std::size_t, plonk_constraint_type*>>> constraints_cache;
    std::size_t row_index;
    std::vector<CellState> cell_states;
    std::vector<Gtk::Button*> widgets;
    std::vector<bool> widget_loaded;
    // Using the model to traverse for gate constraints is annoying, we use previous/next pointers to make it easier.s
    row_object *previous, *next;
};

template<typename BlueprintFieldType>
struct circuit_container {
    // We have to roll a custom container for this because ArithmetizationParams are constexpr in the circuit.
    using plonk_constraint_type = nil::crypto3::zk::snark::plonk_constraint<BlueprintFieldType>;
    using plonk_gate_type = nil::crypto3::zk::snark::plonk_gate<BlueprintFieldType, plonk_constraint_type>;
    using plonk_copy_constraint_type = nil::crypto3::zk::snark::plonk_copy_constraint<BlueprintFieldType>;

    circuit_sizes sizes;
    std::vector<plonk_gate_type> gates;
    std::vector<plonk_copy_constraint_type> copy_constraints;
    // TODO: add lookup gates

    // This is used in order to be able to travers from copy constraint to the underlying variable and it's cell.
    // We can utilise this in tandem with links from row_object to constraints to traverse and colour.
    std::vector<std::pair<row_object<BlueprintFieldType>*, row_object<BlueprintFieldType>*>> copy_constraints_links;
};

template<typename BlueprintFieldType>
struct constraint_object : public Glib::Object {
    // A wrapper for displaying a constraint in a view.
    using plonk_constraint_type = nil::crypto3::zk::snark::plonk_constraint<BlueprintFieldType>;
    using plonk_copy_constraint_type = nil::crypto3::zk::snark::plonk_copy_constraint<BlueprintFieldType>;

    static Glib::RefPtr<constraint_object> create(plonk_constraint_type* constraint_) {
        return Glib::make_refptr_for_instance<constraint_object>(new constraint_object(constraint_));
    }

    static Glib::RefPtr<constraint_object> create(plonk_copy_constraint_type* constraint_) {
        return Glib::make_refptr_for_instance<constraint_object>(new constraint_object(constraint_));
    }

    constraint_object(plonk_constraint_type* constraint_) : constraint(constraint_), button(nullptr),
                                                            loaded(false) {
        std::stringstream ss;
        ss << constraint_;
        cached_string = ss.str();
    }
    constraint_object(plonk_copy_constraint_type* constraint_) : constraint(constraint_), button(nullptr),
                                                                 loaded(false) {
        std::stringstream ss;
        ss << "copy " << constraint_->first << " " << constraint_->second;
        cached_string = ss.str();
    }

    Glib::ustring to_string() const {
        return cached_string;
    }

    void select() {
        state.select();
    }

    void deselect() {
        state.deselect();
    }

    bool is_selected() const {
        return state.is_selected();
    }

    boost::variant<plonk_constraint_type*, plonk_copy_constraint_type*> constraint;
    Glib::ustring cached_string;
    CellState state;
    bool loaded;
    Gtk::Button* button;
};

template<typename WidgetType, typename TrackedType>
struct CellTracker {
    CellTracker() : row(-1), column(-1), tracked_object(nullptr) {}

    void clear() {
        row = -1;
        column = -1;
        tracked_object = nullptr;
    }

    TrackedType* tracked_object;
    std::size_t row, column;
};


template<typename BlueprintFieldType>
class ExcaliburWindow : public Gtk::ApplicationWindow {
public:
    using integral_type = typename BlueprintFieldType::integral_type;
    using value_type = typename BlueprintFieldType::value_type;
    using plonk_copy_constraint_type = nil::crypto3::zk::snark::plonk_copy_constraint<BlueprintFieldType>;
    using plonk_constraint_type = nil::crypto3::zk::snark::plonk_constraint<BlueprintFieldType>;
    using plonk_gate_type = nil::crypto3::zk::snark::plonk_gate<BlueprintFieldType, plonk_constraint_type>;
    using var = nil::crypto3::zk::snark::plonk_variable<BlueprintFieldType>;

    ExcaliburWindow() : table_view(), element_entry(), vbox_prime(), table_window(), vbox_controls(),
                        open_table_button("Open Table"), save_table_button("Save"),
                        open_circuit_button("Open Circuit"), constraints_view(),
                        constraints_window() {
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

        table_window.set_child(table_view);
        table_window.set_size_request(800, 600);
        table_window.set_vexpand(true);
        vbox_prime.append(table_window);

        constraints_window.set_child(constraints_view);
        constraints_window.set_size_request(-1, 128);
        vbox_prime.append(constraints_window);
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

    template<typename ObjectType>
    void setup_constraint_view_from_store(const Glib::RefPtr<Gio::ListStore<ObjectType>> &store) {
        // You might be wondering why don't I use the SingleSelect selection mechanism here, and create a button.
        // It's signal is very questionable, and forces me to manually find out which constraint got selected
        // and also what state it used to be in -- so I have to implement the selection tracking mechanism
        // myself regardless. SingleSelection is a trap.
        constraints_view.set_model(Gtk::NoSelection::create(store));
        auto factory = Gtk::SignalListItemFactory::create();
        factory->signal_setup().connect(
            sigc::mem_fun(*this, &ExcaliburWindow::on_setup_constraint), false);
        factory->signal_bind().connect(
            sigc::mem_fun(*this, &ExcaliburWindow::on_bind_constraint), false);
        factory->signal_unbind().connect(
            sigc::mem_fun(*this, &ExcaliburWindow::on_unbind_constraint), false);
        constraints_view.set_factory(factory);
    }

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
        auto mitem = dynamic_cast<row_object<BlueprintFieldType>*>(&*item);
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
        auto mitem = dynamic_cast<row_object<BlueprintFieldType>*>(&*item);
        if (!mitem) {
            return;
        }
        mitem->set_widget_loaded(column, false);
    }

    void on_setup_constraint(const Glib::RefPtr<Gtk::ListItem> &list_item) {
        auto button = Gtk::make_managed<Gtk::Button>();
        auto label = Gtk::make_managed<Gtk::Label>();
        label->set_halign(Gtk::Align::START);
        button->set_child(*label);
        list_item->set_child(*button);

        button->signal_clicked().connect(
            sigc::bind<0>(
                sigc::mem_fun(*this, &ExcaliburWindow::on_constraint_clicked),
                        list_item));
    }

    void on_bind_constraint(const Glib::RefPtr<Gtk::ListItem> &list_item) {
        auto item = list_item->get_item();
        auto mitem = dynamic_cast<constraint_object<BlueprintFieldType>*>(&*item);
        if (!mitem) {
            return;
        }
        auto button = dynamic_cast<Gtk::Button*>(list_item->get_child());
        if (!button) {
            return;
        }
        auto label = dynamic_cast<Gtk::Label*>(button->get_child());
        if (!label) {
            return;
        }
        label->set_text(mitem->to_string());
        if (mitem->is_selected()) {
            button->add_css_class("selected");
        }
        mitem->loaded = true;
        mitem->button = button;
    }

    void on_unbind_constraint(const Glib::RefPtr<Gtk::ListItem> &list_item) {
        auto item = list_item->get_item();
        auto mitem = dynamic_cast<constraint_object<BlueprintFieldType>*>(&*item);
        if (!mitem) {
            return;
        }
        mitem->loaded = false;
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

        auto store = Gio::ListStore<row_object<BlueprintFieldType>>::create();

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

            store->append(row_object<BlueprintFieldType>::create(row, i));
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
        // Carefully remove the already existing columns
        while (table_view.get_columns()->get_n_items() != 0) {
            auto current_columns = table_view.get_columns();
            table_view.remove_column(
                std::dynamic_pointer_cast<Gtk::ColumnViewColumn>(current_columns->get_object(0)));
        }
        // Clear selections as they are no longer relevant
        selected_cell.clear();
        selected_constraint.clear();
        // Clear constraint view
        auto constraint_store = Gio::ListStore<constraint_object<BlueprintFieldType>>::create();
        setup_constraint_view_from_store(constraint_store);

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
            table_view.append_column(column);
        }

        auto model = Gtk::NoSelection::create(store);
        table_view.set_model(model);
    }

    void on_circuit_file_open_dialog_response(Glib::RefPtr<Gtk::FileDialog> file_dialog,
                                              std::shared_ptr<Gio::AsyncResult> &res) {

        auto result = file_dialog->open_finish(res);
        if (table_view.get_columns()->get_n_items() == 0) {
            std::cerr << "Please open the table before opening the circuit!" << std::endl;
            return;
        }
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
        circuit.gates.reserve(circuit.sizes.gates_size);
        for (std::uint32_t i = 0; i < circuit.sizes.gates_size; i++) {
            std::string line = read_line_from_gstream(stream, predicted_line_size, file_size, buffer);
            if (line.empty()) {
                std::cerr << "Failed to header line for " << i + 1 << "'th gate of the file" << std::endl;
                delete[] buffer;
                return;
            }
            gate_header gate_header;
            auto line_begin = line.begin();
            gate_header_parser<decltype(line.begin())> header_parser;
            r = phrase_parse(line_begin, line.end(), header_parser, boost::spirit::ascii::space, gate_header);
            if (!r || line_begin != line.end()) {
                std::cerr << "Failed to parse gate header for " << i + 1 << "'th gate of the file" << std::endl;
                delete[] buffer;
                return;
            }
            std::vector<plonk_constraint_type> constraints;
            constraints.reserve(gate_header.constraints_size);
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
                line_begin = line.begin();
                r = phrase_parse(line_begin, line.end(), constraint_parser, boost::spirit::ascii::space, constraint);
                if (!r || line_begin != line.end()) {
                    std::cerr << "Failed to parse gate constraint " << j + 1 << " for " << i + 1
                              << "'th gate of the file" << std::endl;
                    delete[] buffer;
                    return;
                }
                constraints.push_back(constraint);
            }

            circuit.gates.emplace_back(plonk_gate_type(gate_header.selector_index, constraints));
        }
        std::sort(circuit.gates.begin(), circuit.gates.end(),
                  [](const plonk_gate_type& a, const plonk_gate_type& b)
                    { return a.selector_index < b.selector_index; });

        copy_constraint_parser<decltype(first_line.begin()), BlueprintFieldType> copy_constraint_parser;
        circuit.copy_constraints.reserve(circuit.sizes.copy_constraints_size);
        for (std::size_t i = 0; i < circuit.sizes.copy_constraints_size; i++) {
            plonk_copy_constraint_type constraint;
            std::string line = read_line_from_gstream(stream, predicted_line_size, file_size, buffer);
            if (line.empty()) {
                std::cerr << "Failed to read line for" << i << "'th copy constraint" << std::endl;
                delete[] buffer;
                return;
            }
            auto line_begin = line.begin();
            r = phrase_parse(line_begin, line.end(), copy_constraint_parser, boost::spirit::ascii::space, constraint);
            if (!r || line_begin != line.end()) {
                std::cerr << "Failed to parse copy constraint " << i + 1 << std::endl;
                delete[] buffer;
                return;
            }
            circuit.copy_constraints.push_back(constraint);
        }

        delete[] buffer;

        // Constraint cache building
        auto selection_model = dynamic_cast<Gtk::NoSelection*>(&*table_view.get_model());
        if (!selection_model) {
            std::cerr << "Failed to get selection model" << std::endl;
            return;
        }
        auto model = selection_model->get_model();
        for (std::size_t i = 0; i < circuit.sizes.copy_constraints_size; i++) {
            auto constraint = &circuit.copy_constraints[i];
            std::array<var, 2> variables = {constraint->first, constraint->second};
            for (auto &variable : variables) {
                auto row = dynamic_cast<row_object<BlueprintFieldType>*>(&*model->get_object(variable.rotation));
                row->add_copy_constraint_to_cache(variable, i, constraint, sizes);
            }
        }
    }

    void on_table_file_save_dialog_response(Glib::RefPtr<Gtk::FileDialog> file_dialog,
                                            bool wide_export,
                                            std::shared_ptr<Gio::AsyncResult> &res) {
        auto result = file_dialog->save_finish(res);
        if (table_view.get_columns()->get_n_items() == 0) {
            std::cerr << "No table to save" << std::endl;
            return;
        }
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
        auto model = dynamic_cast<Gtk::NoSelection*>(&*table_view.get_model());
        if (!model) {
            std::cerr << "No model" << std::endl;
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
                std::cerr << "No object" << std::endl;
                stream->close();
                return;
            }
            auto row = dynamic_cast<row_object<BlueprintFieldType>*>(&*object_row);
            if (!row) {
                std::cerr << "No row" << std::endl;
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
        auto mitem = dynamic_cast<row_object<BlueprintFieldType>*>(&*item);
        if (!mitem) {
            return;
        }
        std::size_t row = mitem->get_row_index();
        auto button = dynamic_cast<Gtk::Button*>(&*list_item->get_child());

        if (selected_cell.row == row && selected_cell.column == column || button == nullptr) {
            return;
        }

        if (selected_cell.tracked_object != nullptr) {
            auto old_row = selected_cell.tracked_object;
            CellState& old_row_state = old_row->get_cell_state(selected_cell.column);
            old_row_state.deselect();
            if (old_row->get_widget_loaded(selected_cell.column)) {
                auto button = old_row->get_widget(selected_cell.column);
                button->remove_css_class("selected");
            }
        }

        selected_cell.row = row;
        selected_cell.column = column;
        selected_cell.tracked_object = mitem;

        button->add_css_class("selected");
        CellState& row_state = mitem->get_cell_state(column);
        row_state.select();

        element_entry.set_text(mitem->to_string(column));

        if (selected_constraint.tracked_object != nullptr) {
            selected_constraint.tracked_object->deselect();
            selected_constraint.tracked_object = nullptr;
        }

        auto store = Gio::ListStore<constraint_object<BlueprintFieldType>>::create();
        for (std::size_t i = 0; i < mitem->get_copy_constraints_size(column); i++) {
            auto constraint = mitem->get_copy_constraint(column, i);
            store->append(constraint_object<BlueprintFieldType>::create(constraint.second));
        }
        setup_constraint_view_from_store(store);
    }

    void on_constraint_clicked(const Glib::RefPtr<Gtk::ListItem> &list_item) {
        auto item = list_item->get_item();
        auto mitem = dynamic_cast<constraint_object<BlueprintFieldType>*>(&*item);
        if (selected_constraint.tracked_object == mitem) {
            return;
        }
        if (!mitem) {
            return;
        }
        auto button = dynamic_cast<Gtk::Button*>(&*list_item->get_child());

        if (selected_constraint.tracked_object != nullptr) {
            auto old_constraint = selected_constraint.tracked_object;
            CellState &old_constraint_state = old_constraint->state;
            old_constraint_state.deselect();
            if (old_constraint->loaded) {
                old_constraint->button->remove_css_class("selected");
            }
        }

        button->add_css_class("selected");

        selected_constraint.tracked_object = mitem;
    }

    void on_entry_key_released(guint keyval, guint keycode, Gdk::ModifierType state) {
        // Didn't select any cell or keyval != enter
        if (keyval != 65293 || selected_cell.tracked_object == nullptr) {
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
        auto selection_model = table_view.get_model();
        auto model = dynamic_cast<Gtk::NoSelection*>(&*selection_model);
        if (!model) {
            std::cerr << "No model" << std::endl;
            return;
        }

        auto object_row = model->get_object(selected_cell.row);
        if (!object_row) {
            std::cerr << "No object" << std::endl;
            return;
        }
        auto row = dynamic_cast<row_object<BlueprintFieldType>*>(&*object_row);
        if (!row) {
            std::cerr << "Failed cast to row" << std::endl;
            return;
        }
        row->set_row_item(value, selected_cell.column);

        if (row->get_widget_loaded(selected_cell.column)) {
            auto button = row->get_widget(selected_cell.column);
            auto label = dynamic_cast<Gtk::Label*>(&*button->get_child());
            if (!label) {
                std::cerr << "Failed cast to label" << std::endl;
                return;
            }
            label->set_text(element_entry.get_text());
        }
    }

protected:
    Gtk::Entry element_entry;
    Gtk::Box vbox_prime, vbox_controls;
    Gtk::ScrolledWindow table_window;
    Gtk::ColumnView table_view;
    Gtk::Button open_table_button, open_circuit_button, save_table_button;
    Gtk::ListView constraints_view;
    Gtk::ScrolledWindow constraints_window;
private:
    table_sizes sizes;
    CellTracker<Gtk::Button, row_object<BlueprintFieldType>> selected_cell;
    CellTracker<Gtk::Button, constraint_object<BlueprintFieldType>> selected_constraint;
    circuit_container<BlueprintFieldType> circuit;
};

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

#include <iostream>
#include <cstring>
#include <sstream>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/phoenix/object/construct.hpp>

#include <giomm/liststore.h>
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

struct TableSizes {
    uint32_t witnesses_size;
    uint32_t public_inputs_size;
    uint32_t constants_size;
    uint32_t selectors_size;
    uint32_t max_size;
};

BOOST_FUSION_ADAPT_STRUCT(
    TableSizes,
    (uint32_t, witnesses_size)
    (uint32_t, public_inputs_size)
    (uint32_t, constants_size)
    (uint32_t, selectors_size)
    (uint32_t, max_size)
)

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

template <typename BlueprintFieldType>
class RowObject : public Glib::Object {
public:
    using value_type = typename BlueprintFieldType::value_type;
    using integral_type = typename BlueprintFieldType::integral_type;

    static Glib::RefPtr<RowObject> create(const std::vector<integral_type>& row_, std::size_t row_index_) {
        return Glib::make_refptr_for_instance<RowObject>(new RowObject(row_, row_index_));
    }

    std::string to_string(std::size_t index) const {
        std::stringstream ss;
        ss << std::hex << row[index].data;
        return ss.str();
    }

    std::string to_string_decimal(std::size_t index) const {
        std::stringstream ss;
        ss << std::dec << row[index].data;
        return ss.str();
    }

    std::size_t get_row_index() const {
        return row_index;
    }
protected:
    RowObject(const std::vector<integral_type>& row_, std::size_t row_index_) : row_index(row_index_) {
        std::copy(row_.begin(), row_.end(), std::back_inserter(row));
    }
private:
    std::vector<value_type> row;
    std::size_t row_index;
};

template<typename BlueprintFieldType>
class ExcaliburWindow : public Gtk::ApplicationWindow {
public:
    using integral_type = typename BlueprintFieldType::integral_type;
    using value_type = typename BlueprintFieldType::value_type;

    ExcaliburWindow() : table(), element_entry(), vbox_prime(), table_window(), vbox_controls(),
                        open_button("Open"), save_button("Save"), cur_row(-1), cur_column(-1) {
        set_title("Excalibur Circuit Viewer: pull the bugs from the stone");
        set_resizable(true);

        auto css_provider = Gtk::CssProvider::create();
        Glib::ustring css_style =
            "* { font: 24px Arial; }"
            "textview.selected { background-color: blue; }";
        css_provider->load_from_data(css_style);
        Gtk::StyleProvider::add_provider_for_display(
            Gdk::Display::get_default(), css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

        element_entry.set_placeholder_text("00000000000000000000000000000000000000000000000000000000000000");
        element_entry.set_max_length(64);

        vbox_prime.set_orientation(Gtk::Orientation::VERTICAL);
        vbox_prime.set_spacing(10);

        vbox_controls.set_spacing(10);
        vbox_controls.set_orientation(Gtk::Orientation::HORIZONTAL);
        vbox_controls.append(open_button);
        vbox_controls.append(save_button);
        vbox_controls.append(element_entry);
        vbox_prime.append(vbox_controls);

        table_window.set_child(table);
        table_window.set_size_request(800, 600);
        table_window.set_vexpand(true);
        vbox_prime.append(table_window);
        vbox_prime.set_vexpand(true);

        set_child(vbox_prime);

        open_button.signal_clicked().connect(
            sigc::bind(sigc::mem_fun(*this, &ExcaliburWindow::on_action_table_file_open)));
    }

    ~ExcaliburWindow() override {};

    void on_action_table_file_open() {
        auto file_dialog = Gtk::FileDialog::create();
        file_dialog->set_modal(true);
        file_dialog->set_title("Open table file");
        file_dialog->open(*this,
            sigc::bind<0>(sigc::mem_fun(*this,
                                        &ExcaliburWindow::on_table_file_dialog_response),
                          file_dialog));
    }

    template<typename Iterator>
    struct SizesParser : boost::spirit::qi::grammar<Iterator, TableSizes(), boost::spirit::qi::ascii::space_type> {
        SizesParser() : SizesParser::base_type(start) {
            using boost::spirit::qi::uint_;
            using boost::spirit::qi::lit;
            using boost::phoenix::val;
            using boost::phoenix::construct;

            start = lit("witnesses_size:") > uint_ >
                    lit("public_inputs_size:") > uint_ >
                    lit("constants_size:") > uint_ >
                    lit("selectors_size:") > uint_ >
                    lit("max_size:") > uint_;

            boost::spirit::qi::on_error<boost::spirit::qi::fail>(
                start,
                std::cerr << val("Error! Expecting ") << boost::spirit::qi::_4 << val(" here: \"")
                          << construct<std::string>(boost::spirit::_3, boost::spirit::_2) << val("\"\n")
            );
        }

        boost::spirit::qi::rule<Iterator, TableSizes(), boost::spirit::qi::ascii::space_type> start;
    };

    template<typename Iterator>
    struct TableRowParser : boost::spirit::qi::grammar<Iterator, std::vector<integral_type>,
                                                       boost::spirit::qi::ascii::space_type> {
        TableRowParser(TableSizes sizes)
                    : TableRowParser::base_type(start) {
            using boost::spirit::qi::lit;
            using boost::spirit::qi::repeat;
            using boost::spirit::qi::uint_parser;
            using boost::spirit::qi::uint_;
            using boost::phoenix::val;
            using boost::phoenix::construct;
            BOOST_STATIC_ASSERT(std::numeric_limits<integral_type>::radix == 2);

            auto hex_rule = uint_parser<integral_type, 16, 1, (BlueprintFieldType::modulus_bits + 4 - 1) / 4>();
            start = repeat(sizes.witnesses_size)[hex_rule] > lit('|') >
                    repeat(sizes.public_inputs_size)[hex_rule] > lit('|') >
                    repeat(sizes.constants_size)[hex_rule] > lit('|') >
                    repeat(sizes.selectors_size)[hex_rule];

            boost::spirit::qi::on_error<boost::spirit::qi::fail>(
                start,
                std::cerr << val("Error! Expecting ") << boost::spirit::qi::_4 << val(" here: \"")
                          << construct<std::string>(boost::spirit::_3, boost::spirit::_2) << val("\"\n")
            );
        }

        boost::spirit::qi::rule<Iterator, std::vector<integral_type>, boost::spirit::qi::ascii::space_type> start;
    };

    void on_setup_column_item(const Glib::RefPtr<Gtk::ListItem> &list_item) {
        auto buffer = Gtk::TextBuffer::create();
        auto text_view = Gtk::make_managed<Gtk::TextView>(buffer);
        text_view->set_size_request(-1, 32);
        text_view->set_editable(false);
        text_view->set_cursor_visible(true);
        buffer->set_text("Pl");

        list_item->set_child(*text_view);
    }

    void on_bind_column_item(std::size_t column, const Glib::RefPtr<Gtk::ListItem> &list_item) {
        auto text_view = dynamic_cast<Gtk::TextView*>(list_item->get_child());
        if (!text_view) {
            return;
        }
        auto item = list_item->get_item();
        auto mitem = std::dynamic_pointer_cast<RowObject<BlueprintFieldType>>(item);
        if (!mitem) {
            return;
        }
        auto buffer = text_view->get_buffer();
        if (column == 0) {
            buffer->set_text(mitem->to_string_decimal(column));
            return;
        }
        buffer->set_text(mitem->to_string(column));
        // We use this instead of signal_mark_set, as it seems to be called less times.
        // This gets called on click.
        buffer->signal_mark_deleted().connect(
            sigc::bind<0>(sigc::bind<0>(sigc::bind<0>(
                sigc::mem_fun(*this, &ExcaliburWindow::on_mark_deleted),
                                                      mitem->get_row_index()),
                                        column),
                          list_item));
    }

    void on_mark_deleted(std::size_t row, std::size_t column,
                         const Glib::RefPtr<Gtk::ListItem> &list_item,
                         const Glib::RefPtr<Gtk::TextBuffer::Mark> &mark) {
        if (cur_row == row && cur_column == column) {
            return;
        }
        auto text_view = dynamic_cast<Gtk::TextView*>(list_item->get_child());
        if (text_view) {
            auto item = list_item->get_item();
            if (auto mo = std::dynamic_pointer_cast<RowObject<BlueprintFieldType>>(item)) {
                element_entry.set_text(mo->to_string(column));
                cur_row = row;
                cur_column = column;

                text_view->add_css_class("selected");
            }
        }
    }

    void on_table_file_dialog_response(Glib::RefPtr<Gtk::FileDialog> file_dialog,
                                       std::shared_ptr<Gio::AsyncResult> &res) {
        auto result = file_dialog->open_finish(res);
        auto stream = result->read();
        auto file_info = result->query_info();
        auto file_size = file_info->get_size();

        using boost::spirit::qi::phrase_parse;
        using boost::spirit::qi::lit;
        using boost::phoenix::ref;
        // 200 should be enough for the first row
        const std::size_t first_line_size = file_size < 200 ? file_size : 200;
        char* buffer = new char[first_line_size + 1];

        std::string first_line = read_line_from_gstream(stream, first_line_size, file_size, buffer);
        if (first_line.empty()) {
            std::cerr << "Failed to read the header line." << std::endl;
            delete[] buffer;
            return;
        }

        TableSizes sizes;
        SizesParser<decltype(first_line.begin())> sizes_parser;
        auto first_line_begin = first_line.begin();

        bool r = phrase_parse(first_line_begin, first_line.end(), sizes_parser, boost::spirit::ascii::space, sizes);
        if (!r || first_line_begin != first_line.end()) {
            std::cerr << "Failed to parse the header line." << std::endl;
            delete[] buffer;
            return;
        }

        delete[] buffer;

        // All lines after the first should be the same length, if no one has messed with the output of table exporter
        auto line_size = (file_size - first_line.size()) / sizes.max_size;
        buffer = new char[line_size + 1];
        TableRowParser<decltype(first_line.begin())> row_parser(sizes);

        auto store = Gio::ListStore<RowObject<BlueprintFieldType>>::create();

        for (std::uint32_t i = 0; i < sizes.max_size; i++) {
            std::string line = read_line_from_gstream(stream, line_size, file_size, buffer);
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
            //row.insert(row.begin(), i);
            store->append(RowObject<BlueprintFieldType>::create(row, i));
        }
        std::cout << "Successfully parsed the file" << std::endl;
        delete[] buffer;
        stream->close();

        std::size_t column_num = 0,
                    column_size = sizes.witnesses_size + sizes.public_inputs_size +
                                  sizes.constants_size + sizes.selectors_size;

        auto get_column_name = [](const TableSizes &sizes, std::size_t i) {
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
            factory->signal_setup().connect(sigc::mem_fun(*this, &ExcaliburWindow::on_setup_column_item));
            factory->signal_bind().connect(
                sigc::bind<0>(sigc::mem_fun(*this, &ExcaliburWindow::on_bind_column_item), i));

            table.append_column(Gtk::ColumnViewColumn::create(get_column_name(sizes, i), factory));
        }

        auto model = Gtk::NoSelection::create(store);
        table.set_model(model);
    }

protected:
    Gtk::Entry element_entry;
    Gtk::Box vbox_prime, vbox_controls;
    Gtk::ScrolledWindow table_window;
    Gtk::ColumnView table;
    Gtk::Button open_button, save_button;
private:
    std::size_t cur_row, cur_column;
};

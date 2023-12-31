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

#include <gtkmm/application.h>

#include <giomm/menu.h>

#include <glibmm/optioncontext.h>
#include <glibmm/optiongroup.h>

#include <nil/crypto3/algebra/fields/vesta/base_field.hpp>
#include <nil/crypto3/algebra/fields/arithmetic_params/vesta.hpp>
#include <nil/crypto3/algebra/curves/vesta.hpp>
#include <nil/crypto3/algebra/fields/pallas/base_field.hpp>
#include <nil/crypto3/algebra/fields/arithmetic_params/pallas.hpp>
#include <nil/crypto3/algebra/curves/pallas.hpp>

#include "table.hpp"

int main(int argc, char* argv[]) {
    auto app = Gtk::Application::create("foundation.nil.excalibur");

    using vesta_curve_type = nil::crypto3::algebra::curves::vesta::base_field_type;
    using pallas_curve_type = nil::crypto3::algebra::curves::pallas::base_field_type;
    using bls12_fr_381_curve_type = nil::crypto3::algebra::fields::bls12_fr<381>;
    using bls12_fq_381_curve_type = nil::crypto3::algebra::fields::bls12_fq<381>;

    Glib::OptionGroup::vecustrings main_option_vector;
    Glib::OptionGroup main_group("curves", "Curves", "Curve used in the program");

    // boolean option
    bool vesta = false, pallas = false, bls12_fr_381 = false, bls12_fq_381 = false;
    Glib::OptionEntry vesta_entry, pallas_entry, bls12_fr_381_entry, bls12_fq_381_entry;

    vesta_entry.set_long_name("vesta");
    vesta_entry.set_short_name('v');
    vesta_entry.set_description("Use Vesta curve");
    main_group.add_entry(vesta_entry, vesta);

    pallas_entry.set_long_name("pallas");
    pallas_entry.set_short_name('p');
    pallas_entry.set_description("Use Pallas curve");
    main_group.add_entry(pallas_entry, pallas);

    bls12_fr_381_entry.set_long_name("bls12_fr_381");
    bls12_fr_381_entry.set_short_name('b');
    bls12_fr_381_entry.set_description("Use BLS12_fr_381 curve");
    main_group.add_entry(bls12_fr_381_entry, bls12_fr_381);

    bls12_fq_381_entry.set_long_name("bls12_fq_381");
    bls12_fq_381_entry.set_short_name('q');
    bls12_fq_381_entry.set_description("Use BLS12_fq_381 curve");
    main_group.add_entry(bls12_fq_381_entry, bls12_fq_381);

    // Add the main group to the context
    Glib::OptionContext context;
    context.set_main_group(main_group);
    context.set_help_enabled(true);
    context.set_ignore_unknown_options(true);
    context.parse(argc, argv);
    // check that only a single curve is selected
    if ((vesta && pallas) || (vesta && bls12_fr_381) || (vesta && bls12_fq_381) || (pallas && bls12_fr_381) ||
        (pallas && bls12_fq_381) || (bls12_fr_381 && bls12_fq_381)) {

        std::cerr << "Error: only one curve can be used at a time." << std::endl;
        return 1;
    }
    if (!vesta && !pallas && !bls12_fr_381 && !bls12_fq_381) {
        std::cerr << "Error: no curve selected. Use --vesta or --pallas or --bls12_fr_381, or --bls12_fq_381"
                  << std::endl;
        return 1;
    }

    if (vesta) {
        return app->make_window_and_run<ExcaliburWindow<vesta_curve_type>>(argc, argv);
    }
    if (pallas) {
        return app->make_window_and_run<ExcaliburWindow<pallas_curve_type>>(argc, argv);
    }
    if (bls12_fr_381) {
        return app->make_window_and_run<ExcaliburWindow<bls12_fr_381_curve_type>>(argc, argv);
    }
    if (bls12_fq_381) {
        return app->make_window_and_run<ExcaliburWindow<bls12_fq_381_curve_type>>(argc, argv);
    }
}
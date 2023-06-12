#include <gtkmm/application.h>
#include <giomm/menu.h>

#include <nil/crypto3/algebra/fields/vesta/base_field.hpp>
#include <nil/crypto3/algebra/fields/arithmetic_params/vesta.hpp>
#include <nil/crypto3/algebra/curves/vesta.hpp>
#include <nil/crypto3/algebra/fields/pallas/base_field.hpp>
#include <nil/crypto3/algebra/fields/arithmetic_params/pallas.hpp>
#include <nil/crypto3/algebra/curves/pallas.hpp>

#include "table.hpp"

int main(int argc, char* argv[]) {
    auto app = Gtk::Application::create("foundation.nil.excalibur");

    using curve_type = nil::crypto3::algebra::curves::vesta::base_field_type;

    return app->make_window_and_run<ExcaliburWindow<curve_type>>(argc, argv);
}
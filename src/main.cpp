#include <gtkmm/application.h>
#include <giomm/menu.h>

#include <nil/crypto3/algebra/fields/vesta/base_field.hpp>
#include <nil/crypto3/algebra/fields/arithmetic_params/vesta.hpp>

#include "table.hpp"

int main(int argc, char* argv[]) {
    auto app = Gtk::Application::create("foundation.nil.excalibur");

    return app->make_window_and_run<ExcaliburWindow>(argc, argv);
}
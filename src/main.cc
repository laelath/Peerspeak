#include <iostream>

#include <gtkmm.h>

#include "peerspeak_window.h"

int main(int argc, char *argv[])
{
    auto app = Gtk::Application::create(argc, argv, "us.laelath.peerspeak");
    try {
        auto builder = Gtk::Builder::create_from_resource("/us/laelath/peerspeak/peerspeak_ui.glade");
        peerspeak::PeerspeakWindow *window = nullptr;
        builder->get_widget_derived("peerspeak_window", window);
        auto style = Gtk::CssProvider::create();
        style->load_from_resource("/us/laelath/peerspeak/peerspeak_style.css");
        Gtk::StyleContext::add_provider_for_screen(Gdk::Screen::get_default(), style, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        int status = app->run(*window);
        delete window;
        return status;
    } catch (const Glib::FileError e) {
        std::cerr << e.what() << std::endl;
    } catch (const Gio::ResourceError& e) {
        std::cerr << e.what() << std::endl;
    } catch (const Gtk::BuilderError& e) {
        std::cerr << e.what() << std::endl;
    } catch (const Gtk::CssProviderError& e) {
        std::cerr << e.what() << std::endl;
    }
    return EXIT_FAILURE;
}

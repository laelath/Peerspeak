#include <iostream>
#include <mutex>
#include <thread>

#include <asio.hpp>
#include <gtkmm.h>

#include "connection_handler.h"
#include "peerspeak_window.h"

int main(int argc, char *argv[])
{
    auto app = Gtk::Application::create(argc, argv, "us.laelath.peerspeak");

    try {
        peerspeak::ConnectionHandler handler;

        auto builder = Gtk::Builder::create_from_resource(
            "/us/laelath/peerspeak/peerspeak_ui.glade");
        peerspeak::PeerspeakWindow *window_ptr = nullptr;
        builder->get_widget_derived("peerspeak_window", window_ptr, handler);
        auto window = std::unique_ptr<peerspeak::PeerspeakWindow>(window_ptr);
        auto style = Gtk::CssProvider::create();
        style->load_from_resource("/us/laelath/peerspeak/peerspeak_style.css");
        Gtk::StyleContext::add_provider_for_screen(Gdk::Screen::get_default(), style,
                                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

        // TODO Setting all this stuff from a settings file or from a dialog in the application
        asio::ip::address addr = asio::ip::address::from_string("127.0.0.1");
        asio::ip::tcp::endpoint discovery(addr, 2738);
        std::thread networker(std::bind(&peerspeak::ConnectionHandler::init, &handler,
                                        window.get(), discovery, 12345));

        // TODO Interface for making open requests

        int status = app->run(*window);
        handler.close();
        networker.join();
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

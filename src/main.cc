#include <iostream>
#include <thread>

#include <asio.hpp>
#include <gtkmm.h>

#include "connection_handler.h"
#include "peerspeak_window.h"

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define htonll(x) ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32)
#define ntohll(x) ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32)
#else
#define htonll(x) (x)
#define ntohll(x) (x)
#endif

void run_networking(uint64_t this_id, uint64_t other_id)
{
    asio::io_service io_service;
    asio::ip::address addr = asio::ip::address::from_string("127.0.0.1");
    asio::ip::tcp::endpoint discovery(addr, 2738);
    ConnectionHandler handler(io_service, discovery, this_id);

    uint64_t temp_id = htonll(other_id);
    handler.queue_write_message(OPEN, asio::buffer(&temp_id, sizeof(temp_id)));

    io_service.run();
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        std::cout << "Incorrect number of arguments" << std::endl;
        return EXIT_FAILURE;
    }
    run_networking(std::stoul(argv[1]), std::stoul(argv[2]));
    return EXIT_SUCCESS;
    //auto app = Gtk::Application::create(argc, argv, "us.laelath.peerspeak");
    //std::thread net_thread(run_networking);
    //try {
        //auto builder = Gtk::Builder::create_from_resource(
            //"/us/laelath/peerspeak/peerspeak_ui.glade");
        //peerspeak::PeerspeakWindow *window_ptr = nullptr;
        //builder->get_widget_derived("peerspeak_window", window_ptr);
        //auto window = std::unique_ptr<peerspeak::PeerspeakWindow>(window_ptr);
        //auto style = Gtk::CssProvider::create();
        //style->load_from_resource("/us/laelath/peerspeak/peerspeak_style.css");
        //Gtk::StyleContext::add_provider_for_screen(Gdk::Screen::get_default(), style,
                                                   //GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        //int status = app->run(*window);
        //return status;
    //} catch (const Glib::FileError e) {
        //std::cerr << e.what() << std::endl;
    //} catch (const Gio::ResourceError& e) {
        //std::cerr << e.what() << std::endl;
    //} catch (const Gtk::BuilderError& e) {
        //std::cerr << e.what() << std::endl;
    //} catch (const Gtk::CssProviderError& e) {
        //std::cerr << e.what() << std::endl;
    //}
    //return EXIT_FAILURE;
}

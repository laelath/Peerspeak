#ifndef peerspeak_window_h_INCLUDED
#define peerspeak_window_h_INCLUDED

#include <memory>
#include <queue>
#include <thread>
#include <vector>

#include <gtkmm.h>

#include "connection_handler.h"

namespace peerspeak {

static const uint16_t default_port = 2738;

class PeerspeakWindow : public Gtk::ApplicationWindow {
public:
    PeerspeakWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder>& builder);
    ~PeerspeakWindow();

    void recv_open(uint64_t id);
    void recv_chat(uint64_t id, std::string msg);
    void show_message(std::string msg);
    void show_error(std::string msg);

private:
    Glib::RefPtr<Gtk::Builder> builder;

    Gtk::TreeView *connections_view;
    Gtk::Box *chat_box;
    Gtk::Entry *chat_entry;

    Gtk::Dialog *accept_dialog;
    Gtk::Label *accept_id_label;
    Gtk::Button *accept_accept_button;
    Gtk::Button *accept_reject_button;

    Gtk::Dialog *init_dialog;
    Gtk::Entry *init_id_entry;
    Gtk::Entry *init_ip_entry;
    Gtk::Entry *init_port_entry;
    Gtk::Button *init_apply_button;
    Gtk::Button *init_cancel_button;

    Gtk::Dialog *open_dialog;
    Gtk::Entry *open_id_entry;
    Gtk::Button *open_connect_button;
    Gtk::Button *open_cancel_button;

    std::vector<std::unique_ptr<Gtk::Label>> chat_labels;

    std::thread network_thread;
    ConnectionHandler handler;

    Glib::Dispatcher open_dispatcher;
    Glib::Dispatcher chat_dispatcher;
    Glib::Dispatcher message_dispatcher;
    Glib::Dispatcher error_dispatcher;

    std::queue<uint64_t> open_queue;
    std::queue<std::pair<uint64_t, std::string>> chat_queue;
    std::queue<std::string> message_queue;
    std::queue<std::string> error_queue;

    std::mutex open_mutex;
    std::mutex chat_mutex;
    std::mutex message_mutex;
    std::mutex error_mutex;

    void init_widgets();
    void connect_signals();
    void init_actions();
    void init_networking();

    void open_connection();

    void open_callback();
    void chat_callback();
    void message_callback();
    void error_callback();

    void add_chat(std::string msg, std::string user);
    void add_chat(std::string msg);
    void add_entry_chat();
};

} // namespace peerspeak

#endif // peerspeak_window_h_INCLUDED


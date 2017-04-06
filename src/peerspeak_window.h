#ifndef peerspeak_window_h_INCLUDED
#define peerspeak_window_h_INCLUDED

#include <list>
#include <unordered_map>
#include <memory>
#include <queue>
#include <thread>
//#include <vector>

#include <gtkmm.h>

#include "connection_handler.h"
#include "connection_info.h"

namespace peerspeak {

static const uint16_t default_port = 2738;

class PeerspeakWindow : public Gtk::ApplicationWindow {
public:
    PeerspeakWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder>& builder);
    ~PeerspeakWindow();

    void recv_open(uint64_t id);
    void recv_connect(uint64_t id);
    void recv_disconnect(uint64_t id);
    void recv_add(uint64_t id);
    void recv_remove(uint64_t id);
    void recv_chat(uint64_t id, std::string msg);
    void show_error(std::string msg);

private:
    Glib::RefPtr<Gtk::Builder> builder;

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

    std::list<std::unique_ptr<Gtk::Label>> chat_labels;
    std::list<std::unique_ptr<Gtk::Label>>::iterator prev_user_label;
    uint64_t prev_user;

    std::thread network_thread;
    ConnectionHandler handler;

    std::unordered_map<uint64_t, std::pair<unsigned int, std::unique_ptr<ConnectionInfo>>>
        direct_connections;
    std::unordered_map<uint64_t, std::pair<unsigned int, std::unique_ptr<ConnectionInfo>>>
        secondary_connections;

    Glib::Dispatcher open_dispatcher;
    Glib::Dispatcher connect_dispatcher;
    Glib::Dispatcher disconnect_dispatcher;
    Glib::Dispatcher add_dispatcher;
    Glib::Dispatcher remove_dispatcher;
    Glib::Dispatcher chat_dispatcher;
    Glib::Dispatcher error_dispatcher;

    std::queue<uint64_t> open_queue;
    std::queue<uint64_t> connect_queue;
    std::queue<uint64_t> disconnect_queue;
    std::queue<uint64_t> add_queue;
    std::queue<uint64_t> remove_queue;
    std::queue<std::pair<uint64_t, std::string>> chat_queue;
    std::queue<std::string> error_queue;

    std::mutex open_mutex;
    std::mutex connect_mutex;
    std::mutex disconnect_mutex;
    std::mutex add_mutex;
    std::mutex remove_mutex;
    std::mutex chat_mutex;
    std::mutex error_mutex;

    void init_widgets();
    void connect_signals();
    void init_actions();
    void init_networking();

    void open_connection();

    void open_callback();
    void connect_callback();
    void disconnect_callback();
    void add_callback();
    void remove_callback();
    void chat_callback();
    void error_callback();

    void add_connect(uint64_t id);
    void add_disconnect(uint64_t id);

    void add_chat(std::string msg, uint64_t peer_id);
    void add_chat(std::string msg);
    void add_entry_chat();
};

} // namespace peerspeak

#endif // peerspeak_window_h_INCLUDED


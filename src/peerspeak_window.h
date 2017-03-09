#ifndef peerspeak_window_h_INCLUDED
#define peerspeak_window_h_INCLUDED

#include <memory>
#include <queue>
#include <thread>
#include <vector>

#include <gtkmm.h>

#include "connection_handler.h"

namespace peerspeak {

class ConnectionHandler;

class PeerspeakWindow : public Gtk::ApplicationWindow {
public:
    PeerspeakWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder>& builder);
    ~PeerspeakWindow();

    void recv_open(uint64_t id);
    void recv_chat(uint64_t id, std::string msg);

private:
    Glib::RefPtr<Gtk::Builder> builder;

    Gtk::TreeView *connectionsView;
    Gtk::Box *chatBox;
    Gtk::Entry *chatEntry;

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

    // TODO replace unique_ptr with emplace_back
    std::vector<std::unique_ptr<Gtk::Label>> chatLabels;

    std::thread network_thread;
    ConnectionHandler handler;

    Glib::Dispatcher open_dispatcher;
    Glib::Dispatcher chat_dispatcher;

    std::queue<uint64_t> open_queue;
    std::queue<std::pair<uint64_t, std::string>> chat_queue;

    std::mutex open_mutex;
    std::mutex chat_mutex;

    void initWidgets();
    void connectSignals();
    void initActions();
    void init_networking();

    void open_callback();
    void chat_callback();

    void addChat(std::string msg, std::string user);
    void addChat(std::string msg);
    void addEntryChat();
};

} // namespace peerspeak

#endif // peerspeak_window_h_INCLUDED


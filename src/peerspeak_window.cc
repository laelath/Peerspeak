#include "peerspeak_window.h"

#include <iostream>

namespace peerspeak {

PeerspeakWindow::PeerspeakWindow(BaseObjectType *cobject,
                                 const Glib::RefPtr<Gtk::Builder>& builder)
    : Gtk::ApplicationWindow(cobject),
      builder(builder),
      handler(this)
{
    init_widgets();
    connect_signals();
    init_actions();

    chat_entry->grab_focus();
}

PeerspeakWindow::~PeerspeakWindow()
{
    //handler.send_close();
    //if (network_thread.joinable())
        //network_thread.join();
    handler.stop();
    if (network_thread.joinable())
        network_thread.join();
}

void PeerspeakWindow::recv_open(uint64_t id)
{
    std::lock_guard<std::mutex> lock(open_mutex);
    open_queue.push(id);
    open_dispatcher();
}

void PeerspeakWindow::recv_connect(uint64_t id)
{
    std::lock_guard<std::mutex> lock(connect_mutex);
    connect_queue.push(id);
    connect_dispatcher();
}

void PeerspeakWindow::recv_disconnect(uint64_t id)
{
    std::lock_guard<std::mutex> lock(disconnect_mutex);
    disconnect_queue.push(id);
    disconnect_dispatcher();
}

void PeerspeakWindow::recv_chat(uint64_t id, std::string msg)
{
    std::lock_guard<std::mutex> lock(chat_mutex);
    chat_queue.emplace(id, msg);
    chat_dispatcher();
}

void PeerspeakWindow::show_error(std::string msg)
{
    std::lock_guard<std::mutex> lock(error_mutex);
    error_queue.push(msg);
    error_dispatcher();
}

void PeerspeakWindow::init_widgets()
{
    //builder->get_widget("connections_view", connections_view);
    builder->get_widget("chat_box", chat_box);
    builder->get_widget("chat_entry", chat_entry);

    builder->get_widget("accept_dialog", accept_dialog);
    builder->get_widget("accept_id_label", accept_id_label);
    builder->get_widget("accept_accept_button", accept_accept_button);
    builder->get_widget("accept_reject_button", accept_reject_button);

    builder->get_widget("init_dialog", init_dialog);
    builder->get_widget("init_id_entry", init_id_entry);
    builder->get_widget("init_ip_entry", init_ip_entry);
    builder->get_widget("init_port_entry", init_port_entry);
    builder->get_widget("init_apply_button", init_apply_button);
    builder->get_widget("init_cancel_button", init_cancel_button);

    builder->get_widget("open_dialog", open_dialog);
    builder->get_widget("open_id_entry", open_id_entry);
    builder->get_widget("open_connect_button", open_connect_button);
    builder->get_widget("open_cancel_button", open_cancel_button);
}

void PeerspeakWindow::connect_signals()
{
    signal_show().connect(std::bind(&PeerspeakWindow::init_networking, this));

    chat_entry->signal_activate().connect(std::bind(&PeerspeakWindow::add_entry_chat, this));

    accept_accept_button->signal_clicked().connect(
        std::bind(&Gtk::Dialog::response, accept_dialog, Gtk::RESPONSE_ACCEPT));
    accept_reject_button->signal_clicked().connect(
        std::bind(&Gtk::Dialog::response, accept_dialog, Gtk::RESPONSE_REJECT));

    init_apply_button->signal_clicked().connect(
        std::bind(&Gtk::Dialog::response, init_dialog, Gtk::RESPONSE_APPLY));
    init_cancel_button->signal_clicked().connect(
        std::bind(&Gtk::Dialog::response, init_dialog, Gtk::RESPONSE_CANCEL));

    open_connect_button->signal_clicked().connect(
        std::bind(&Gtk::Dialog::response, open_dialog, Gtk::RESPONSE_APPLY));
    open_cancel_button->signal_clicked().connect(
        std::bind(&Gtk::Dialog::response, open_dialog, Gtk::RESPONSE_CANCEL));

    open_dispatcher.connect(std::bind(&PeerspeakWindow::open_callback, this));
    connect_dispatcher.connect(std::bind(&PeerspeakWindow::connect_callback, this));
    disconnect_dispatcher.connect(std::bind(&PeerspeakWindow::disconnect_callback, this));
    chat_dispatcher.connect(std::bind(&PeerspeakWindow::chat_callback, this));
    error_dispatcher.connect(std::bind(&PeerspeakWindow::error_callback, this));
}

void PeerspeakWindow::init_actions()
{
    this->add_action("quit", std::bind(&Gtk::ApplicationWindow::close, this));
    this->add_action("open-connection", std::bind(&PeerspeakWindow::open_connection, this));
}

void PeerspeakWindow::init_networking()
{
    int status = init_dialog->run();
    if (status == Gtk::RESPONSE_APPLY) {
        // TODO parse error handling
        uint64_t id = std::stoull(init_id_entry->get_text());

        uint16_t port = default_port;
        if (init_port_entry->get_text().length() > 0)
            port = std::stoi(init_port_entry->get_text());

        handler.init(init_ip_entry->get_text(), port, id);

        network_thread = std::thread(std::bind(&ConnectionHandler::start, &handler));
    } else
        std::cout << "Critical: Discovery server information not given" << std::endl;
    init_dialog->close();
}

void PeerspeakWindow::open_connection()
{
    int status = open_dialog->run();
    if (status == Gtk::RESPONSE_APPLY) {
        // TODO parse error handling
        uint64_t id = std::stoull(open_id_entry->get_text());
        handler.send_open(id);
    }
    open_dialog->close();
}

void PeerspeakWindow::open_callback()
{
    std::unique_lock<std::mutex> lock(open_mutex);
    if (not open_queue.empty()) {
        uint64_t new_id = open_queue.front();
        open_queue.pop();
        lock.unlock();

        accept_id_label->set_text("Open request from ID: " + std::to_string(new_id));
        int response = accept_dialog->run();
        if (response == Gtk::RESPONSE_ACCEPT)
            handler.send_accept(true);
        else
            handler.send_accept(false);
        accept_dialog->close();
    }
}

void PeerspeakWindow::connect_callback()
{
    std::unique_lock<std::mutex> lock(connect_mutex);
    if (not connect_queue.empty()) {
        uint64_t id = connect_queue.front();
        add_connect(id);
        connect_queue.pop();
    }
}

void PeerspeakWindow::disconnect_callback()
{
    std::unique_lock<std::mutex> lock(disconnect_mutex);
    if (not disconnect_queue.empty()) {
        uint64_t id = disconnect_queue.front();
        add_disconnect(id);
        disconnect_queue.pop();
    }
}

void PeerspeakWindow::chat_callback()
{
    std::unique_lock<std::mutex> lock(chat_mutex);
    if (not chat_queue.empty()) {
        auto chat = chat_queue.front();
        add_chat(chat.second, chat.first);
        chat_queue.pop();
    }
}

void PeerspeakWindow::error_callback()
{
    std::unique_lock<std::mutex> lock(error_mutex);
    if (not error_queue.empty()) {
        std::string err = error_queue.front();
        error_queue.pop();
        lock.unlock();

        Gtk::MessageDialog dialog(err, false, Gtk::MessageType::MESSAGE_ERROR,
                                  Gtk::ButtonsType::BUTTONS_CLOSE, true);
        dialog.set_transient_for(*this);
        dialog.run();
        dialog.close();
    }
}

void PeerspeakWindow::add_connect(uint64_t id)
{
    auto label = std::make_unique<Gtk::Label>();
    label->set_markup(std::to_string(id) + " connected");
    label->set_halign(Gtk::Align::ALIGN_START);
    //label->set_xalign(0);
    label->show();
    chat_box->pack_start(*label, false, true);
    chat_labels.push_back(std::move(label));
}

void PeerspeakWindow::add_disconnect(uint64_t id)
{
    auto label = std::make_unique<Gtk::Label>();
    label->set_markup(std::to_string(id) + " disconnected");
    label->set_halign(Gtk::Align::ALIGN_START);
    //label->set_xalign(0);
    label->show();
    chat_box->pack_start(*label, false, true);
    chat_labels.push_back(std::move(label));
}

void PeerspeakWindow::add_chat(std::string msg, uint64_t peer_id)
{
    auto label = std::make_unique<Gtk::Label>();
    label->set_markup(msg);
    label->set_halign(Gtk::Align::ALIGN_START);
    //label->set_xalign(0);
    label->set_line_wrap(true);
    label->set_selectable(true);
    label->get_style_context()->add_class("chat");
    label->show();
    chat_box->pack_start(*label, false, true);
    chat_labels.push_back(std::move(label));

    if (prev_user == peer_id) {
        chat_box->remove(**prev_user_label);
        chat_box->pack_start(**prev_user_label, false, true);
    } else {
        auto user_label = std::make_unique<Gtk::Label>();
        user_label->set_markup(std::to_string(peer_id));
        user_label->set_halign(Gtk::Align::ALIGN_START);
        //user_label->set_xalign(0);
        user_label->show();
        chat_box->pack_start(*user_label, false, true);
        chat_labels.push_back(std::move(user_label));
        prev_user = peer_id;
        prev_user_label = --chat_labels.end();
    }
}

void PeerspeakWindow::add_chat(std::string msg)
{
    auto label = std::make_unique<Gtk::Label>();
    label->set_markup(msg);
    label->set_halign(Gtk::Align::ALIGN_END);
    //label->set_xalign(1);
    label->set_line_wrap(true);
    label->set_selectable(true);
    label->get_style_context()->add_class("chat");
    label->show();
    chat_box->pack_start(*label, false, true);
    chat_labels.push_back(std::move(label));
    prev_user = 0;
}

void PeerspeakWindow::add_entry_chat()
{
    std::string msg = chat_entry->get_text();
    chat_entry->set_text("");
    add_chat(msg);

    handler.send_chat(msg);
}

} // namespace peerspeak

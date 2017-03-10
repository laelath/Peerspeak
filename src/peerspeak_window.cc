#include "peerspeak_window.h"

#include <iostream>

namespace peerspeak {

PeerspeakWindow::PeerspeakWindow(BaseObjectType *cobject,
                                 const Glib::RefPtr<Gtk::Builder>& builder)
    : Gtk::ApplicationWindow(cobject),
      builder(builder)
{
    initWidgets();
    connectSignals();
    initActions();

    connectionsView->grab_focus();
}

PeerspeakWindow::~PeerspeakWindow()
{
    handler.close();
    if (network_thread.joinable())
        network_thread.join();
}

void PeerspeakWindow::recv_open(uint64_t id)
{
    std::lock_guard<std::mutex> lock(open_mutex);
    open_queue.push(id);
    open_dispatcher();
}

void PeerspeakWindow::recv_chat(uint64_t id, std::string msg)
{
    std::lock_guard<std::mutex> lock(chat_mutex);
    chat_queue.emplace(id, msg);
    chat_dispatcher();
}

void PeerspeakWindow::initWidgets()
{
    builder->get_widget("connections_view", connectionsView);
    builder->get_widget("chat_box", chatBox);
    builder->get_widget("chat_entry", chatEntry);

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

void PeerspeakWindow::connectSignals()
{
    signal_show().connect(std::bind(&PeerspeakWindow::init_networking, this));

    chatEntry->signal_activate().connect(std::bind(&PeerspeakWindow::addEntryChat, this));

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
    chat_dispatcher.connect(std::bind(&PeerspeakWindow::chat_callback, this));
}

void PeerspeakWindow::initActions()
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
        asio::ip::address addr = asio::ip::address::from_string(init_ip_entry->get_text());

        uint16_t port = default_port;
        if (init_port_entry->get_text().length() > 0)
            port = std::stoi(init_port_entry->get_text());

        asio::ip::tcp::endpoint discovery(addr, port);
        network_thread = std::thread(std::bind(&ConnectionHandler::init, &handler,
                                               this, discovery, id));
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

void PeerspeakWindow::chat_callback()
{
    std::lock_guard<std::mutex> lock(chat_mutex);
    if (not chat_queue.empty()) {
        auto chat = chat_queue.front();
        addChat(chat.second, std::to_string(chat.first));
        chat_queue.pop();
    }
}

void PeerspeakWindow::addChat(std::string msg, std::string user)
{
    Gtk::Label *label = new Gtk::Label();
    label->set_markup("<b>" + user + "</b>: " + msg);
    label->set_halign(Gtk::Align::ALIGN_START);
    label->set_xalign(0);
    label->set_line_wrap(true);
    label->set_selectable(true);
    label->show();
    chatBox->pack_start(*label, false, true);
    chatLabels.push_back(std::move(label));
}

void PeerspeakWindow::addChat(std::string msg)
{
    Gtk::Label *label = new Gtk::Label();
    label->set_halign(Gtk::Align::ALIGN_END);
    label->set_xalign(1);
    label->set_line_wrap(true);
    label->set_selectable(true);
    label->show();
    chatBox->pack_start(*label, false, true);
    chatLabels.push_back(std::move(label));
}

void PeerspeakWindow::addEntryChat()
{
    std::string msg = chatEntry->get_text();
    chatEntry->set_text("");
    addChat(msg);

    handler.send_chat(msg);
}

} // namespace peerspeak

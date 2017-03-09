#include "peerspeak_window.h"

#include "connection_handler.h"

namespace peerspeak {

PeerspeakWindow::PeerspeakWindow(BaseObjectType *cobject,
                                 const Glib::RefPtr<Gtk::Builder>& builder,
                                 ConnectionHandler &handler)
    : Gtk::ApplicationWindow(cobject),
      builder(builder)
{
    initWidgets();
    connectSignals();
    initActions();

    this->handler = &handler;

    connectionsView->grab_focus();

    addChat("This is a test message", "Test");
    addChat("This is a message sent by the user");
    addChat("Another <s>test message</s> sent by someone else", "Tast");
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
}

void PeerspeakWindow::connectSignals()
{
    chatEntry->signal_activate().connect(std::bind(&PeerspeakWindow::addEntryChat, this));

    open_dispatcher.connect(std::bind(&PeerspeakWindow::open_callback, this));
    chat_dispatcher.connect(std::bind(&PeerspeakWindow::chat_callback, this));
}

void PeerspeakWindow::initActions()
{
}

void PeerspeakWindow::open_callback()
{
    std::lock_guard<std::mutex> lock(open_mutex);
    if (not open_queue.empty()) {
        uint64_t new_id = open_queue.front();
        // TODO handle open request
        open_queue.pop();
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
    auto label = std::make_unique<Gtk::Label>();
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
    auto label = std::make_unique<Gtk::Label>(msg);
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

    handler->send_chat(msg);
}

} // namespace peerspeak

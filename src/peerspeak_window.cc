#include "peerspeak_window.h"

namespace peerspeak {

PeerspeakWindow::PeerspeakWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder>& builder)
    : Gtk::ApplicationWindow(cobject),
      builder(builder)
{
    initWidgets();
    connectSignals();
    initActions();

    connectionsView->grab_focus();

    addChat("This is a test message", "Test");
    addChat("This is a message sent by the user");
}

void PeerspeakWindow::initWidgets()
{
    builder->get_widget("connections_view", connectionsView);
    builder->get_widget("chat_box", chatBox);
    builder->get_widget("chat_entry", chatEntry);
}

void PeerspeakWindow::connectSignals()
{
    chatEntry->signal_activate().connect(sigc::mem_fun(this, &PeerspeakWindow::addEntryChat));
}

void PeerspeakWindow::initActions()
{
}

void PeerspeakWindow::addChat(std::string msg, std::string user)
{
    auto label = std::make_unique<Gtk::Label>();
    label->set_markup("<b>" + user + "</b>: " + msg);
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
}

} // namespace peerspeak

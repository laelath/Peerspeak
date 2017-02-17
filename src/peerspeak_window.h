#ifndef peerspeak_window_h_INCLUDED
#define peerspeak_window_h_INCLUDED

#include <memory>
#include <vector>

#include <gtkmm.h>

namespace peerspeak {

class PeerspeakWindow : public Gtk::ApplicationWindow {
public:
    PeerspeakWindow(BaseObjectType *cobject,
                    const Glib::RefPtr<Gtk::Builder>& builder);

private:
    Glib::RefPtr<Gtk::Builder> builder;

    Gtk::TreeView *connectionsView;
    Gtk::Box *chatBox;
    Gtk::Entry *chatEntry;

    std::vector<std::unique_ptr<Gtk::Label>> chatLabels;

    void initWidgets();
    void connectSignals();
    void initActions();

    void addChat(std::string msg, std::string user);
    void addChat(std::string msg);
    void addEntryChat();
};

} // namespace peerspeak

#endif // peerspeak_window_h_INCLUDED


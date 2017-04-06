#ifndef connection_info_h_INCLUDED
#define connection_info_h_INCLUDED

#include <string>

#include <gtkmm.h>

namespace peerspeak {

class ConnectionInfo {
public:
    ConnectionInfo(std::string name);

    void set_active(bool active);

private:
    std::unique_ptr<Gtk::Box> info_box;
    std::unique_ptr<Gtk::Image> audio_image;
    std::unique_ptr<Gtk::Label> connection_label;
};

} // namespace peerspeak

#endif // connection_info_h_INCLUDED

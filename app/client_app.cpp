#include <gtkmm.h>
#include <giomm.h>
#include <iostream>
#include <vector>
#include <thread>

class ChatClient : public Gtk::Window {
public:
    ChatClient();
    ~ChatClient() override;

private:
    // UI элементы
    Gtk::Box m_MainBox; // Используем конструктор по умолчанию
    Gtk::Entry m_UsernameEntry;
    Gtk::Entry m_MessageEntry;
    Gtk::TextView m_ChatView;
    Gtk::ScrolledWindow m_ScrolledWindow;
    Gtk::Button m_ConnectButton{"Connect"};
    Gtk::Button m_SendButton{"Send"};

    // Сетевые объекты
    Glib::RefPtr<Gio::SocketClient> m_SocketClient;
    Glib::RefPtr<Gio::SocketConnection> m_Connection;
    Glib::RefPtr<Gio::OutputStream> m_OutStream;
    Glib::RefPtr<Gio::InputStream> m_InStream;

    // Логика
    void on_connect_button_clicked();
    void on_send_button_clicked();
    void read_messages();
    void append_message(const Glib::ustring& message);
};

ChatClient::ChatClient() : Gtk::Window(Gtk::WINDOW_TOPLEVEL), m_MainBox(Gtk::ORIENTATION_VERTICAL, 5) {
    set_title("Chat Client");
    set_default_size(600, 400);
    set_border_width(10);

    // Добавляем элементы
    add(m_MainBox);

    // Поле ввода имени
    m_UsernameEntry.set_placeholder_text("Enter your name");
    m_MainBox.pack_start(m_UsernameEntry, Gtk::PACK_SHRINK);

    // Кнопка подключения
    m_MainBox.pack_start(m_ConnectButton, Gtk::PACK_SHRINK);
    m_ConnectButton.signal_clicked().connect(sigc::mem_fun(*this, &ChatClient::on_connect_button_clicked));

    // Область чата
    m_ChatView.set_editable(false);
    m_ScrolledWindow.add(m_ChatView);
    m_MainBox.pack_start(m_ScrolledWindow, Gtk::PACK_EXPAND_WIDGET);

    // Поле ввода сообщений
    m_MessageEntry.set_placeholder_text("Type a message...");
    m_MainBox.pack_start(m_MessageEntry, Gtk::PACK_SHRINK);

    // Кнопка отправки
    m_MainBox.pack_start(m_SendButton, Gtk::PACK_SHRINK);
    m_SendButton.signal_clicked().connect(sigc::mem_fun(*this, &ChatClient::on_send_button_clicked));

    show_all_children();
}

ChatClient::~ChatClient() {}

void ChatClient::on_connect_button_clicked() {
    Glib::ustring username = m_UsernameEntry.get_text();
    if (username.empty()) {
        Gtk::MessageDialog dialog(*this, "Please enter a username!");
        dialog.run();
        return;
    }

    try {
        m_SocketClient = Gio::SocketClient::create();
        m_Connection = m_SocketClient->connect_to_host("192.168.2.112", 12345); // IP сервера
        m_OutStream = m_Connection->get_output_stream();
        m_InStream = m_Connection->get_input_stream();

        // Отправляем имя
        std::string usernameMsg = "USERNAME:" + username.raw();
        guint32 length = static_cast<guint32>(usernameMsg.size());
        m_OutStream->write(&length, sizeof(length));
        m_OutStream->write(usernameMsg.data(), usernameMsg.size());
        m_OutStream->flush();

        // Запускаем чтение сообщений в отдельном потоке
        std::thread([this]() {
            this->read_messages();
        }).detach();
    }
    catch (const Glib::Error& ex) {
        Glib::signal_idle().connect([this, ex]() {
            Gtk::MessageDialog dialog(*this, "Connection failed: " + ex.what());
            dialog.run();
            return false;
        });
    }
}

void ChatClient::on_send_button_clicked() {
    Glib::ustring message = m_MessageEntry.get_text();
    if (!m_Connection || message.empty()) return;

    try {
        guint32 length = static_cast<guint32>(message.bytes());
        m_OutStream->write(&length, sizeof(length));
        m_OutStream->write(message.data(), length);
        m_OutStream->flush();

        // Получаем имя пользователя
        Glib::ustring username = m_UsernameEntry.get_text();
        // Формируем сообщение в формате "Имя: Текст"
        Glib::ustring localMessage = username + ": " + message;

        // Добавляем сообщение в чат через Glib::signal_idle (потокобезопасно)
        Glib::signal_idle().connect([this, localMessage]() {
            append_message(localMessage);
            return false;
        });

        m_MessageEntry.set_text("");
    }
    catch (const Glib::Error& ex) {
        Glib::signal_idle().connect([this, ex]() {
            Gtk::MessageDialog dialog(*this, "Failed to send message: " + ex.what());
            dialog.run();
            return false;
        });
    }
}

void ChatClient::read_messages() {
    try {
        while (true) {
            guint32 length = 0;
            gsize bytesRead = m_InStream->read(&length, sizeof(length));
            if (bytesRead != sizeof(length)) break;

            std::vector<char> buffer(length + 1);
            bytesRead = m_InStream->read(buffer.data(), length);
            if (bytesRead != length) break;

            buffer[length] = '\0';
            Glib::ustring msg(buffer.data());

            // Обновляем интерфейс в основном потоке
            Glib::signal_idle().connect([this, msg]() {
                append_message(msg);
                return false;
            });
        }
    }
    catch (const Glib::Error& ex) {
        Glib::signal_idle().connect([this, ex]() {
            Gtk::MessageDialog dialog(*this, "Connection lost: " + ex.what());
            dialog.run();
            return false;
        });
    }
}

void ChatClient::append_message(const Glib::ustring& message) {
    Gtk::TextBuffer::iterator iter = m_ChatView.get_buffer()->end();
    m_ChatView.get_buffer()->insert(iter, message + "\n");
}

int main(int argc, char* argv[]) {
    auto app = Gtk::Application::create(argc, argv, "org.gtkmm.chatclient");
    ChatClient window;
    return app->run(window);
}
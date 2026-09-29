#pragma once

#include "search_engine.h"
#include "search_types.h"
#include <dbus/dbus.h>
#include <string>

namespace AsterSearch {

class DBusService {
public:
    explicit DBusService(SearchEngine& engine);
    ~DBusService();

    bool initialize();
    bool register_service();
    void run();
    void stop();

private:
    static DBusHandlerResult message_handler(DBusConnection* connection,
                                            DBusMessage* message,
                                            void* user_data);

    void handle_search(DBusMessage* message);
    void handle_status(DBusMessage* message);
    void handle_index_stats(DBusMessage* message);

    void send_reply(DBusMessage* request, DBusMessage* reply);
    void send_error(DBusMessage* request, const std::string& error_name, const std::string& message);

    static SearchRequest parse_search_request(DBusMessage* message);
    static std::string serialize_search_response(const SearchResponse& response);
    static std::string escape_json(const std::string& input);

    DBusConnection* connection_ = nullptr;
    SearchEngine& engine_;
    const std::string service_name_ = "com.astersearch.Service";
    const std::string object_path_ = "/com/astersearch/Service";
    const std::string interface_name_ = "com.astersearch.Service.Search";
};

}  // namespace AsterSearch

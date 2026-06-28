#ifndef WSPROTOCOL_HPP
#define WSPROTOCOL_HPP

namespace WsProtocol {

constexpr const char *VERSION = "1.0.0";

enum class MessageType { Command, Response, Event, Heartbeat };

namespace Action {
constexpr const char *OPEN_APP = "open_app";
constexpr const char *CLOSE_APP = "close_app";
constexpr const char *CAPTURE_SCREEN = "capture_screen";
constexpr const char *GET_DEVICE_INFO = "get_device_info";
} // namespace Action

namespace Event {
constexpr const char *DEVICE_READY = "device_ready";
constexpr const char *DEVICE_DISCONNECTED = "device_disconnected";
} // namespace Event

namespace Code {
constexpr int SUCCESS = 0;
constexpr int UNKNOWN_ERROR = 1001;
constexpr int INVALID_PARAMS = 1002;
constexpr int DEVICE_NOT_READY = 1003;
constexpr int PERMISSION_DENIED = 1004;
constexpr int TIMEOUT = 1005;
constexpr int NOT_SUPPORTED = 1006;
constexpr int APP_NOT_FOUND = 1007;
constexpr int TASK_ALREADY_RUN = 1008;
} // namespace Code

} // namespace WsProtocol

#endif // WSPROTOCOL_HPP

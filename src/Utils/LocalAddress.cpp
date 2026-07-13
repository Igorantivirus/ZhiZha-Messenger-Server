#include <Utils/LocalAddress.hpp>

#include <boost/asio.hpp>

namespace utils
{

std::string detectLocalAddress()
{
    try
    {
        namespace asio = boost::asio;
        asio::io_context io;
        asio::ip::udp::socket socket(io);
        // Адрес назначения произвольный публичный: соединение UDP не шлёт
        // пакетов, нужен только выбор маршрута операционной системой.
        socket.connect(asio::ip::udp::endpoint(
            asio::ip::make_address("8.8.8.8"), 53));
        return socket.local_endpoint().address().to_string();
    }
    catch (const std::exception &)
    {
        return "127.0.0.1";
    }
}

} // namespace utils

#pragma once

#include <optional>

#include <Protocol/Common/Http.hpp>
#include <Protocol/Common/Types.hpp>
#include <Protocol/Dto/Messages.hpp>

namespace protocol::messages
{

// Операция получения сообщения из комнаты
struct GetMessagesOperation
{
    static constexpr HttpMethod method = HttpMethod::get;
    static constexpr Path path = API "rooms/<uint>/messages";

    struct PathParams
    {
        RoomId roomId;
    };
    struct QueryParams
    {
        std::optional<MessageId> beforeId; // id до которого нужны сообщения
        std::optional<MessageId> afterId;  // id после которого нужны сообщения
        unsigned limit;                    // максимальнео число сообщений
        bool withSenders;                  // Отправлять ли информацию об отправителдях
    };

    using Request = void;
    using Response = dto::GetMessagesResponseDto;
};

} // namespace protocol::messages

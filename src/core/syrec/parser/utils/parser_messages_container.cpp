#include "core/syrec/parser/utils/parser_messages_container.hpp"

using namespace syrecParser;

void ParserMessagesContainer::recordMessage(Message&& message) {
    if (message.mType != Message::MessageType::Error)
        return;

    if (!errorMessages.empty() && recordAtMostOneError)
        return;

    errorMessages.push_back(std::move(message));
}

std::vector<Message> ParserMessagesContainer::getMessagesOfType(Message::MessageType messageType) const {
    switch (messageType) {
        case Message::MessageType::Error:
            return errorMessages;
        default:
            return {};
    }
}
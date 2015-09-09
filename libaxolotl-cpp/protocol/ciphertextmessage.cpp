
#include "ciphertextmessage.h"

const int CiphertextMessage::UNSUPPORTED_VERSION         = 1;
const int CiphertextMessage::CURRENT_VERSION             = 3;

const int CiphertextMessage::WHISPER_TYPE                = 2;
const int CiphertextMessage::PREKEY_TYPE                 = 3;
const int CiphertextMessage::SENDERKEY_TYPE              = 4;
const int CiphertextMessage::SENDERKEY_DISTRIBUTION_TYPE = 5;

// This should be the worst case (worse than V2).  So not always accurate, but good enough for padding.
const int CiphertextMessage::ENCRYPTED_MESSAGE_OVERHEAD = 53;



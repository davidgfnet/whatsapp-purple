#include "inmemorysessionstore.h"
#include "serializer.h"
#include "state/LocalStorageProtocol.pb.h"

#include <iostream>
#include <vector>

InMemorySessionStore::InMemorySessionStore(Unserializer uns)
{
	unsigned int n = uns.readInt32();
	while (n--) {
		uint64_t recipientId = uns.readInt64();
		int deviceId = uns.readInt32();

		SessionsKeyPair key(recipientId, deviceId);
		sessions[key] = uns.readString();
	}
}

SessionRecord *InMemorySessionStore::loadSession(uint64_t recipientId, int deviceId)
{
	SessionsKeyPair key(recipientId, deviceId);
	if (sessions.find(key) != sessions.end()) {
		return new SessionRecord(sessions.at(key));
	}
	else {
		return new SessionRecord();
	}
}

std::vector<int> InMemorySessionStore::getSubDeviceSessions(uint64_t recipientId)
{
	std::vector<int> deviceIds;

	for (auto it: sessions) {
		if (it.first.first == recipientId) {
			deviceIds.push_back(it.first.second);
		}
	}

	return deviceIds;
}

void InMemorySessionStore::storeSession(uint64_t recipientId, int deviceId, SessionRecord *record)
{
	SessionsKeyPair key(recipientId, deviceId);
	ByteArray serialized = record->serialize();
	sessions.emplace(key, serialized);
}

bool InMemorySessionStore::containsSession(uint64_t recipientId, int deviceId)
{
	SessionsKeyPair key(recipientId, deviceId);
	return sessions.find(key) != sessions.end();
}

void InMemorySessionStore::deleteSession(uint64_t recipientId, int deviceId)
{
	SessionsKeyPair key(recipientId, deviceId);
	sessions.erase(key);
}

void InMemorySessionStore::deleteAllSessions(uint64_t recipientId)
{
	bool modified;
	do {
		modified = false;
		for (auto & key: sessions) {
			if (key.first.first == recipientId) {
				sessions.erase(key.first);
				modified = true;
				break;
			}
		}
	} while (modified);
}

std::string InMemorySessionStore::serialize() const {
	Serializer ser;
	ser.putInt32(sessions.size());

	for (auto & key: sessions) {
		ser.putInt64(key.first.first);
		ser.putInt32(key.first.second);
		ser.putString(key.second);
	}

	return ser.getBuffer();
}




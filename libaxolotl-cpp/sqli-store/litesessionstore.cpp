#include "litesessionstore.h"
#include "sqliutil.h"

LiteSessionStore::LiteSessionStore(sqlite::connection &db)
 : _db(db)
{
	sqlite::execute(_db, "CREATE TABLE IF NOT EXISTS sessions (_id INTEGER PRIMARY KEY AUTOINCREMENT, recipient_id INTEGER UNIQUE, device_id INTEGER, record BLOB, timestamp INTEGER);", true);
}

void LiteSessionStore::clear()
{
	sqlite::execute(_db, "DELETE FROM sessions;", true);
}

SessionRecord *LiteSessionStore::loadSession(uint64_t recipientId, int deviceId)
{
	sqlite::query q(_db, "SELECT record FROM sessions WHERE recipient_id=" + 
		std::to_string(recipientId) + " AND device_id=" + std::to_string(deviceId) + ";");

	boost::shared_ptr<sqlite::result> result = q.get_result();

	if (result->next_row()) {
		std::cerr << "Loaded session" << recipientId << deviceId << std::endl;
		std::vector<unsigned char> res;
		result->get_binary(0, res);
		std::string serialized = barray_to_string(res);
		return new SessionRecord(serialized);
	}
	else {
		std::cerr << "New session " << recipientId << " " << deviceId << std::endl;
		return new SessionRecord();
	}
}

std::vector<int> LiteSessionStore::getSubDeviceSessions(uint64_t recipientId)
{
	std::vector<int> deviceIds;
	sqlite::query q(_db, "SELECT device_id from sessions WHERE recipient_id=?;");
	q % (int64_t)recipientId;
	q();

	boost::shared_ptr<sqlite::result> result = q.get_result();

	while (result->next_row()) {
		int deviceId = result->get_int(0);
		deviceIds.push_back(deviceId);
	}
	return deviceIds;
}

void LiteSessionStore::storeSession(uint64_t recipientId, int deviceId, SessionRecord *record)
{
	sqlite::query q(_db, "INSERT OR REPLACE INTO sessions (`recipient_id`, `device_id`, `record`, `timestamp`) VALUES (?, ?, ?, ?);");
	q % (int64_t)recipientId % deviceId
		% string_to_barray(record->serialize()) % 0;
	q();
}

bool LiteSessionStore::containsSession(uint64_t recipientId, int deviceId)
{
	sqlite::query q(_db, "SELECT record FROM sessions WHERE recipient_id=" + 
		std::to_string(recipientId) + " AND device_id=" + std::to_string(deviceId) + ";");

	return q.get_result()->next_row();
}

void LiteSessionStore::deleteSession(uint64_t recipientId, int deviceId)
{
	sqlite::query q(_db, "DELETE FROM sessions WHERE recipient_id=? AND device_id=?;");
	q % (int64_t)recipientId % deviceId;
	q();
}

void LiteSessionStore::deleteAllSessions(uint64_t recipientId)
{
	sqlite::query q(_db, "DELETE FROM sessions WHERE recipient_id=?;");
	q % (int64_t)recipientId;
	q();
}



#include "liteprekeystore.h"
#include "whisperexception.h"
#include "sqliutil.h"
#include <iostream>

LitePreKeyStore::LitePreKeyStore(sqlite::connection &db)
 : _db(db)
{
	sqlite::execute(_db, "CREATE TABLE IF NOT EXISTS prekeys (_id INTEGER PRIMARY KEY AUTOINCREMENT, prekey_id INTEGER UNIQUE, sent_to_server BOOLEAN, record BLOB);", true);
}

void LitePreKeyStore::clear()
{
	sqlite::execute(_db, "DELETE FROM prekeys;", true);
}

PreKeyRecord LitePreKeyStore::loadPreKey(uint64_t preKeyId)
{
	sqlite::query q(_db, "SELECT record FROM prekeys WHERE prekey_id = " + std::to_string(preKeyId) + ";");
	std::cerr << "Querying pre key id " << preKeyId << std::endl;

	boost::shared_ptr<sqlite::result> result = q.get_result();

	if (result->next_row()) {
		std::vector<unsigned char> res;
		result->get_binary(0, res);
		std::string serialized = barray_to_string(res);
		PreKeyRecord record(serialized);
		return record;
	}
	else {
		throw WhisperException("No such prekeyRecord! " + std::to_string(preKeyId));
	}
}

void LitePreKeyStore::storePreKey(uint64_t preKeyId, const PreKeyRecord &record)
{
	sqlite::query q(_db, "INSERT INTO prekeys (prekey_id, sent_to_server, record) VALUES (?, ?, ?);");
	q % (int64_t)preKeyId % 0
		% string_to_barray(record.serialize());
	q();
}

bool LitePreKeyStore::containsPreKey(uint64_t preKeyId)
{
	sqlite::query q(_db, "SELECT record FROM prekeys WHERE prekey_id= "+ std::to_string(preKeyId) +";");
	return q.get_result()->next_row();
}

void LitePreKeyStore::removePreKey(uint64_t preKeyId)
{
	std::cerr << "DELETE pre key id " << preKeyId << std::endl;
	sqlite::query q(_db, "DELETE FROM prekeys WHERE prekey_id = "+ std::to_string(preKeyId) +";");
	q();
}

int LitePreKeyStore::countPreKeys()
{
	sqlite::query q(_db, "SELECT COUNT(*) FROM prekeys;");
	boost::shared_ptr<sqlite::result> result = q.get_result();

	if (result->next_row())
		return result->get_int(0);
	return 0;
}


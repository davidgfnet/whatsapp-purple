#include "litesignedprekeystore.h"
#include "whisperexception.h"
#include "sqliutil.h"

LiteSignedPreKeyStore::LiteSignedPreKeyStore(sqlite::connection &db)
 : _db(db)
{
	sqlite::execute(_db, "CREATE TABLE IF NOT EXISTS signed_prekeys (_id INTEGER PRIMARY KEY AUTOINCREMENT, prekey_id INTEGER UNIQUE, timestamp INTEGER, record BLOB);", true);
}

void LiteSignedPreKeyStore::clear()
{
	sqlite::execute(_db, "DELETE FROM signed_prekeys;", true);
}

SignedPreKeyRecord LiteSignedPreKeyStore::loadSignedPreKey(uint64_t signedPreKeyId)
{
	sqlite::query q(_db, "SELECT record FROM signed_prekeys WHERE prekey_id = " + std::to_string(signedPreKeyId) + " ;");
	std::cerr << "Querying signed pre key id " << signedPreKeyId << std::endl;

	boost::shared_ptr<sqlite::result> result = q.get_result();

	if (result->next_row()) {
		std::vector<unsigned char> res;
		result->get_binary(0, res);
		std::string serialized = barray_to_string(res);

		SignedPreKeyRecord record(serialized);
		return record;
	}
	else {
		throw WhisperException("No such signedprekeyrecord! " + std::to_string(signedPreKeyId));
	}
}

std::vector<SignedPreKeyRecord> LiteSignedPreKeyStore::loadSignedPreKeys()
{
	std::vector<SignedPreKeyRecord> recordsList;

	sqlite::query q(_db, "SELECT record FROM signed_prekeys;");
	boost::shared_ptr<sqlite::result> result = q.get_result();

	while (result->next_row()) {
		std::vector<unsigned char> res;
		result->get_binary(0, res);
		std::string serialized = barray_to_string(res);
		SignedPreKeyRecord record(serialized);
		recordsList.push_back(record);
	}
	return recordsList;
}

void LiteSignedPreKeyStore::storeSignedPreKey(uint64_t signedPreKeyId, const SignedPreKeyRecord &record)
{
	sqlite::query q(_db, "INSERT INTO signed_prekeys VALUES (NULL, ?, ?, ?);");
	q % (int64_t)signedPreKeyId % 0
		% string_to_barray(record.serialize());
	q();
}

bool LiteSignedPreKeyStore::containsSignedPreKey(uint64_t signedPreKeyId)
{
	sqlite::query q(_db, "SELECT record FROM signed_prekeys WHERE prekey_id=?;");
	q % (int64_t)signedPreKeyId;
	q();

	return q.get_result()->next_row();
}

void LiteSignedPreKeyStore::removeSignedPreKey(uint64_t signedPreKeyId)
{
	std::cerr << "Deleting signed pre key id " << signedPreKeyId << std::endl;
	sqlite::query q(_db, "DELETE FROM signed_prekeys WHERE prekey_id=?;");
	q % (int64_t)signedPreKeyId;
	q();
}


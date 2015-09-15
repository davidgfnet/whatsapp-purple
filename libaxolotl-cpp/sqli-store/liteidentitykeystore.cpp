
#include "liteidentitykeystore.h"
#include "whisperexception.h"
#include "sqliutil.h"

LiteIdentityKeyStore::LiteIdentityKeyStore(sqlite::connection &db)
 : _db(db)
{
    sqlite::execute(_db, "CREATE TABLE IF NOT EXISTS identities (_id INTEGER PRIMARY KEY AUTOINCREMENT, recipient_id INTEGER UNIQUE, registration_id INTEGER, public_key BLOB, private_key BLOB, next_prekey_id INTEGER, timestamp INTEGER);", true);
}

void LiteIdentityKeyStore::clear()
{
    sqlite::execute(_db, "DELETE FROM identities;", true);
}

IdentityKeyPair LiteIdentityKeyStore::getIdentityKeyPair()
{
    sqlite::query q(_db, "SELECT public_key, private_key FROM identities WHERE recipient_id = -1;");

    boost::shared_ptr<sqlite::result> result = q.get_result();

    if (result->next_row()) {
        std::vector<unsigned char> res, res2;
        result->get_binary(0, res);
        std::string publicBytes = barray_to_string(res).substr(1);
        DjbECPublicKey publicKey(publicBytes);
        IdentityKey publicIdentity(publicKey);
        result->get_binary(1, res2);
        std::string privateBytes = barray_to_string(res2);
        DjbECPrivateKey privateKey(privateBytes);
        IdentityKeyPair keypair(publicIdentity, privateKey);
        return keypair;
    }
    else {
        throw WhisperException("Can't get IdentityKeyPair!");
    }
}

unsigned int LiteIdentityKeyStore::getLocalRegistrationId()
{
    sqlite::query q(_db, "SELECT registration_id FROM identities WHERE recipient_id = -1;");

    boost::shared_ptr<sqlite::result> result = q.get_result();

    if (result->next_row()) {
        return result->get_int(0);
    }
    else {
        throw WhisperException("Can't get LocalRegistrationId!");
    }
}

void LiteIdentityKeyStore::removeIdentity(uint64_t recipientId)
{
    sqlite::query q(_db, "DELETE FROM identities WHERE recipient_id=?;");
    q % (int64_t)recipientId;
    q();
}

void LiteIdentityKeyStore::storeLocalData(uint64_t registrationId, const IdentityKeyPair identityKeyPair)
{
    sqlite::query q(_db, "INSERT INTO identities (recipient_id, registration_id, public_key, private_key) VALUES(-1, ?, ?, ?);");
    q % (int64_t)registrationId
        % string_to_barray(identityKeyPair.getPublicKey().getPublicKey().serialize())
        % string_to_barray(identityKeyPair.getPrivateKey().serialize());
    q();
}

void LiteIdentityKeyStore::saveIdentity(uint64_t recipientId, const IdentityKey &identityKey)
{
    sqlite::query q(_db, "DELETE FROM identities WHERE recipient_id=?;");
    q % (int64_t)recipientId;
    q();

    sqlite::query q2(_db, "INSERT INTO identities (recipient_id, public_key) VALUES(?, ?);");
    q2 % (int64_t)recipientId % string_to_barray(identityKey.getPublicKey().serialize());
    q2();
}

bool LiteIdentityKeyStore::isTrustedIdentity(uint64_t recipientId, const IdentityKey &identityKey)
{
    sqlite::query q(_db, "SELECT public_key from identities WHERE recipient_id=?;");
    q % (int64_t)recipientId;
    q();
    boost::shared_ptr<sqlite::result> result = q.get_result();

    if (result->next_row()) {
        std::vector<unsigned char> res;
        result->get_binary(0, res);
        std::string publicKey = barray_to_string(res);
        return publicKey == identityKey.getPublicKey().serialize();
    }
    else {
        return true;
    }
}


CFLAGS = -I./ecc -I./exception -I./util -I./state -I./groups/ratchet -I./groups/state -I./kdf -I./ratchet -I./ -std=c++11 -fPIC -O0 -ggdb

CPP_FILES = identitykey.cpp \
			identitykeypair.cpp \
			sessionbuilder.cpp \
			sessioncipher.cpp \
			ecc/curve.cpp \
			ecc/eckeypair.cpp \
			ecc/djbec.cpp \
			kdf/derivedmessagesecrets.cpp \
			kdf/derivedrootsecrets.cpp \
			kdf/hkdf.cpp \
			groups/state/senderkeystate.cpp \
			groups/state/senderkeyrecord.cpp \
			groups/ratchet/senderchainkey.cpp \
			groups/ratchet/sendermessagekey.cpp \
			ratchet/bobaxolotlparameters.cpp \
			ratchet/aliceaxolotlparameters.cpp \
			ratchet/messagekeys.cpp \
			ratchet/ratchetingsession.cpp \
			ratchet/chainkey.cpp \
			ratchet/rootkey.cpp \
			ratchet/symmetricaxolotlparameters.cpp \
			state/signedprekeyrecord.cpp \
			state/prekeybundle.cpp \
			state/prekeyrecord.cpp \
			state/sessionrecord.cpp \
			state/sessionstate.cpp \
			util/byteutil.cpp \
			util/keyhelper.cpp \
			protocol/keyexchangemessage.cpp \
			protocol/senderkeydistributionmessage.cpp \
			protocol/whispermessage.cpp \
			protocol/prekeywhispermessage.cpp \
			protocol/senderkeymessage.cpp \
			protocol/ciphertextmessage.cpp \
			store/liteaxolotlstore.cpp \
			store/liteidentitykeystore.cpp \
			store/liteprekeystore.cpp \
			store/litesignedprekeystore.cpp \
			store/litesessionstore.cpp \
			store/sqliutil.cpp \
			state/LocalStorageProtocol.pb.cc \
			protocol/WhisperTextProtocol.pb.cc

HEADERS = state/LocalStorageProtocol.pb.h protocol/WhisperTextProtocol.pb.h

OBJS1 = $(CPP_FILES:%.cpp=%.o)
OBJS =  $(OBJS1:%.cc=%.o)

all:	$(HEADERS) $(OBJS)
	g++ -shared -o libaxolotl.so $(OBJS) -lprotobuf -lcrypto -lvsqlitepp libcurve25519/libcurve25519.a

%.h:
	protoc --cpp_out=state protobuf/LocalStorageProtocol.proto
	protoc --cpp_out=protocol protobuf/WhisperTextProtocol.proto
	cp state/protobuf/LocalStorageProtocol.pb.cc state/LocalStorageProtocol.pb.cc
	cp protocol/protobuf/WhisperTextProtocol.pb.cc protocol/WhisperTextProtocol.pb.cc
	cp state/protobuf/LocalStorageProtocol.pb.h state/LocalStorageProtocol.pb.h
	cp protocol/protobuf/WhisperTextProtocol.pb.h protocol/WhisperTextProtocol.pb.h

%.o:	%.cpp
	$(CXX) -o $@ -c $< $(CFLAGS)

%.o:	%.cc
	$(CXX) -o $@ -c $< $(CFLAGS)

clean:
	rm -f $(OBJS) state/LocalStorageProtocol.pb.cc protocol/WhisperTextProtocol.pb.cc state/LocalStorageProtocol.pb.h protocol/WhisperTextProtocol.pb.h



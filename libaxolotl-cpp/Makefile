
CXX=g++
GCC=gcc

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
			sqli-store/liteaxolotlstore.cpp \
			sqli-store/liteidentitykeystore.cpp \
			sqli-store/liteprekeystore.cpp \
			sqli-store/litesignedprekeystore.cpp \
			sqli-store/litesessionstore.cpp \
			sqli-store/sqliutil.cpp \
			mem-store/inmemoryaxolotlstore.cpp \
			mem-store/inmemoryidentitykeystore.cpp \
			mem-store/inmemoryprekeystore.cpp \
			mem-store/inmemorysignedprekeystore.cpp \
			mem-store/inmemorysessionstore.cpp \
			mem-store/serializer.cpp \
			state/LocalStorageProtocol.pb.cc \
			protocol/WhisperTextProtocol.pb.cc

HEADERS = state/LocalStorageProtocol.pb.h protocol/WhisperTextProtocol.pb.h

OBJS1 = $(CPP_FILES:%.cpp=%.o)
OBJS =  $(OBJS1:%.cc=%.o)

all:	$(HEADERS) $(OBJS)
	$(CXX) -shared -o libaxolotl.so $(OBJS) -lprotobuf libcurve25519/libcurve25519.a
	$(AR) rcs libaxolotl.a $(OBJS)

protocol/WhisperTextProtocol.pb.cc:	protocol/WhisperTextProtocol.pb.h
	# Do nothing

state/LocalStorageProtocol.pb.cc:	state/LocalStorageProtocol.pb.h
	# Do nothing

protocol/WhisperTextProtocol.pb.h:
	protoc --cpp_out=protocol protobuf/WhisperTextProtocol.proto
	cp protocol/protobuf/WhisperTextProtocol.pb.cc protocol/WhisperTextProtocol.pb.cc
	cp protocol/protobuf/WhisperTextProtocol.pb.h protocol/WhisperTextProtocol.pb.h

state/LocalStorageProtocol.pb.h:
	protoc --cpp_out=state protobuf/LocalStorageProtocol.proto
	cp state/protobuf/LocalStorageProtocol.pb.cc state/LocalStorageProtocol.pb.cc
	cp state/protobuf/LocalStorageProtocol.pb.h state/LocalStorageProtocol.pb.h

%.o:	%.cpp
	$(CXX) -o $@ -c $< $(CFLAGS)

%.o:	%.cc
	$(CXX) -o $@ -c $< $(CFLAGS)

clean:
	rm -f $(OBJS) state/LocalStorageProtocol.pb.cc protocol/WhisperTextProtocol.pb.cc state/LocalStorageProtocol.pb.h protocol/WhisperTextProtocol.pb.h



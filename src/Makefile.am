DIST_SUBDIRS = secp256k1 univalue

AM_LDFLAGS = $(PTHREAD_CFLAGS) $(LIBTOOL_LDFLAGS) $(HARDENED_LDFLAGS)
AM_CXXFLAGS = $(HARDENED_CXXFLAGS)
AM_CPPFLAGS = $(HARDENED_CPPFLAGS)
EXTRA_LIBRARIES =

if EMBEDDED_UNIVALUE
LIBUNIVALUE = univalue/libunivalue.la

$(LIBUNIVALUE): $(wildcard univalue/lib/*) $(wildcard univalue/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)
else
LIBUNIVALUE = $(UNIVALUE_LIBS)
endif

MASSGRID_CONFIG_INCLUDES=-I$(builddir)/config
MASSGRID_INCLUDES=-I$(builddir) -I$(builddir)/obj $(BOOST_CPPFLAGS) $(LEVELDB_CPPFLAGS) $(CRYPTO_CFLAGS) $(SSL_CFLAGS)

MASSGRID_INCLUDES += -I$(srcdir)/secp256k1/include
MASSGRID_INCLUDES += $(UNIVALUE_CFLAGS)

LIBMASSGRID_SERVER=libmassgrid_server.a
LIBMASSGRID_WALLET=libmassgrid_wallet.a
LIBMASSGRID_COMMON=libmassgrid_common.a
LIBMASSGRID_CLI=libmassgrid_cli.a
LIBMASSGRID_UTIL=libmassgrid_util.a
LIBMASSGRID_CRYPTO=crypto/libmassgrid_crypto.a
LIBMASSGRIDQT=qt/libmassgridqt.a
LIBSECP256K1=secp256k1/libsecp256k1.la

$(LIBSECP256K1): $(wildcard secp256k1/src/*) $(wildcard secp256k1/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)

# Make is not made aware of per-object dependencies to avoid limiting building parallelization
# But to build the less dependent modules first, we manually select their order here:
EXTRA_LIBRARIES += \
  crypto/libmassgrid_crypto.a \
  libmassgrid_util.a \
  libmassgrid_common.a \
  libmassgrid_server.a \
  libmassgrid_cli.a
if ENABLE_WALLET
MASSGRID_INCLUDES += $(BDB_CPPFLAGS)
EXTRA_LIBRARIES += libmassgrid_wallet.a
endif
if ENABLE_ZMQ
EXTRA_LIBRARIES += libmassgrid_zmq.a
endif

if BUILD_MASSGRID_LIBS
lib_LTLIBRARIES = libmassgridconsensus.la
LIBMASSGRID_CONSENSUS=libmassgridconsensus.la
else
LIBMASSGRID_CONSENSUS=
endif

bin_PROGRAMS =
TESTS =
BENCHMARKS =

if BUILD_MASSGRIDD
  bin_PROGRAMS += massgridd
endif

if BUILD_MASSGRID_UTILS
  bin_PROGRAMS += massgrid-cli massgrid-tx
endif

.PHONY: FORCE check-symbols check-security
# massgrid core #
MASSGRID_CORE_H = \
  addrdb.h \
  activemasternode.h \
  addressindex.h \
  spentindex.h \
  addrman.h \
  alert.h \
  amount.h \
  arith_uint256.h \
  base58.h \
  bip39.h \
  bip39_english.h \
  bloom.h \
  cachemap.h \
  cachemultimap.h \
  chain.h \
  chainparams.h \
  chainparamsbase.h \
  chainparamsseeds.h \
  checkpoints.h \
  checkqueue.h \
  clientversion.h \
  coincontrol.h \
  coins.h \
  compat.h \
  compat/byteswap.h \
  compat/endian.h \
  compat/sanity.h \
  compressor.h \
  consensus/consensus.h \
  consensus/merkle.h \
  consensus/params.h \
  consensus/validation.h \
  core_io.h \
  core_memusage.h \
  dsnotificationinterface.h \
  governance.h \
  governance-classes.h \
  governance-exceptions.h \
  governance-object.h \
  governance-validators.h \
  governance-vote.h \
  governance-votedb.h \
  flat-database.h \
  hash.h \
  hdchain.h \
  httprpc.h \
  httpserver.h \
  init.h \
  instantx.h \
  key.h \
  keepass.h \
  keystore.h \
  dbwrapper.h \
  limitedmap.h \
  masternode.h \
  masternode-payments.h \
  masternode-sync.h \
  masternodeman.h \
  masternodeconfig.h \
  memusage.h \
  merkleblock.h \
  messagesigner.h \
  miner.h \
  net.h \
  net_processing.h \
  netaddress.h \
  netbase.h \
  netfulfilledman.h \
  noui.h \
  policy/fees.h \
  policy/policy.h \
  policy/rbf.h \
  pow.h \
  prevector.h \
  primitives/block.h \
  primitives/transaction.h \
  protocol.h \
  pubkey.h \
  random.h \
  reverselock.h \
  rpc/client.h \
  rpc/protocol.h \
  rpc/server.h \
  scheduler.h \
  script/interpreter.h \
  script/script.h \
  script/script_error.h \
  script/sigcache.h \
  script/sign.h \
  script/standard.h \
  serialize.h \
  spork.h \
  streams.h \
  support/allocators/secure.h \
  support/allocators/zeroafterfree.h \
  support/cleanse.h \
  support/pagelocker.h \
  sync.h \
  threadsafety.h \
  threadinterrupt.h \
  timedata.h \
  tinyformat.h \
  torcontrol.h \
  txdb.h \
  txmempool.h \
  ui_interface.h \
  uint256.h \
  undo.h \
  util.h \
  utilmoneystr.h \
  utilstrencodings.h \
  utiltime.h \
  validation.h \
  validationinterface.h \
  version.h \
  versionbits.h \
  wallet/crypter.h \
  wallet/db.h \
  wallet/wallet.h \
  wallet/wallet_ismine.h \
  wallet/walletdb.h \
  zmq/zmqabstractnotifier.h \
  zmq/zmqconfig.h\
  zmq/zmqnotificationinterface.h \
  zmq/zmqpublishnotifier.h


obj/build.h: FORCE
	@$(MKDIR_P) $(builddir)/obj
	@$(top_srcdir)/share/genbuild.sh $(abs_top_builddir)/src/obj/build.h \
	  $(abs_top_srcdir)
libmassgrid_util_a-clientversion.$(OBJEXT): obj/build.h

# server: shared between massgridd and massgrid-qt
libmassgrid_server_a_CPPFLAGS = $(AM_CPPFLAGS) $(MASSGRID_INCLUDES) $(MINIUPNPC_CPPFLAGS) $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)
libmassgrid_server_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libmassgrid_server_a_SOURCES = \
  activemasternode.cpp \
  addrman.cpp \
  addrdb.cpp \
  alert.cpp \
  bloom.cpp \
  chain.cpp \
  checkpoints.cpp \
  dsnotificationinterface.cpp \
  httprpc.cpp \
  httpserver.cpp \
  init.cpp \
  instantx.cpp \
  dbwrapper.cpp \
  governance.cpp \
  governance-classes.cpp \
  governance-object.cpp \
  governance-validators.cpp \
  governance-vote.cpp \
  governance-votedb.cpp \
  masternode.cpp \
  masternode-payments.cpp \
  masternode-sync.cpp \
  masternodeconfig.cpp \
  masternodeman.cpp \
  merkleblock.cpp \
  messagesigner.cpp \
  miner.cpp \
  net.cpp \
  netfulfilledman.cpp \
  net_processing.cpp \
  noui.cpp \
  policy/fees.cpp \
  policy/policy.cpp \
  pow.cpp \
  rest.cpp \
  rpc/blockchain.cpp \
  rpc/masternode.cpp \
  rpc/governance.cpp \
  rpc/mining.cpp \
  rpc/misc.cpp \
  rpc/net.cpp \
  rpc/rawtransaction.cpp \
  rpc/server.cpp \
  script/sigcache.cpp \
  sendalert.cpp \
  spork.cpp \
  timedata.cpp \
  torcontrol.cpp \
  txdb.cpp \
  txmempool.cpp \
  validation.cpp \
  validationinterface.cpp \
  $(MASSGRID_CORE_H)

if ENABLE_ZMQ
LIBMASSGRID_ZMQ=libmassgrid_zmq.a

libmassgrid_zmq_a_CPPFLAGS = $(MASSGRID_INCLUDES) $(ZMQ_CFLAGS)
libmassgrid_zmq_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libmassgrid_zmq_a_SOURCES = \
  zmq/zmqabstractnotifier.cpp \
  zmq/zmqnotificationinterface.cpp \
  zmq/zmqpublishnotifier.cpp
endif


# wallet: shared between massgridd and massgrid-qt, but only linked
# when wallet enabled
libmassgrid_wallet_a_CPPFLAGS = $(AM_CPPFLAGS) $(MASSGRID_INCLUDES)
libmassgrid_wallet_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libmassgrid_wallet_a_SOURCES = \
  keepass.cpp \
  wallet/crypter.cpp \
  wallet/db.cpp \
  wallet/rpcdump.cpp \
  wallet/rpcwallet.cpp \
  wallet/wallet.cpp \
  wallet/wallet_ismine.cpp \
  wallet/walletdb.cpp \
  policy/rbf.cpp \
  $(MASSGRID_CORE_H)

# crypto primitives library
crypto_libmassgrid_crypto_a_CPPFLAGS = $(AM_CPPFLAGS) $(MASSGRID_CONFIG_INCLUDES) $(PIC_FLAGS)
crypto_libmassgrid_crypto_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) $(PIC_FLAGS)
crypto_libmassgrid_crypto_a_SOURCES = \
  crypto/common.h \
  crypto/hmac_sha256.cpp \
  crypto/hmac_sha256.h \
  crypto/hmac_sha512.cpp \
  crypto/hmac_sha512.h \
  crypto/ripemd160.cpp \
  crypto/aes_helper.c \
  crypto/ripemd160.h \
  crypto/sha1.cpp \
  crypto/sha1.h \
  crypto/sha256.cpp \
  crypto/sha256.h \
  crypto/sha512.cpp \
  crypto/sha512.h

# x11
crypto_libmassgrid_crypto_a_SOURCES += \
  crypto/blake.c \
  crypto/bmw.c \
  crypto/cubehash.c \
  crypto/echo.c \
  crypto/groestl.c \
  crypto/jh.c \
  crypto/keccak.c \
  crypto/luffa.c \
  crypto/shavite.c \
  crypto/simd.c \
  crypto/skein.c \
  crypto/sph_blake.h \
  crypto/sph_bmw.h \
  crypto/sph_cubehash.h \
  crypto/sph_echo.h \
  crypto/sph_groestl.h \
  crypto/sph_jh.h \
  crypto/sph_keccak.h \
  crypto/sph_luffa.h \
  crypto/sph_shavite.h \
  crypto/sph_simd.h \
  crypto/sph_skein.h \
  crypto/sph_types.h \
  crypto/sha1.cpp \
  crypto/sha256.cpp \
  crypto/keccak256.cpp \
  crypto/scrypt.cpp  \
  crypto/blake.c \
  crypto/bmw.c \
  crypto/cubehash.c \
  crypto/echo.c \
  crypto/fugue.c \
  crypto/groestl.c \
  crypto/hamsi.c \
  crypto/hamsi_helper.c \
  crypto/jh.c \
  crypto/keccak.c \
  crypto/luffa.c \
  crypto/shavite.c \
  crypto/simd.c \
  crypto/skein.c \
  crypto/w_blake.c \
  crypto/w_bmw.c \
  crypto/w_cubehash.c \
  crypto/w_echo.c \
  crypto/w_fugue.c \
  crypto/w_groestl.c \
  crypto/w_hamsi.c \
  crypto/w_aes_helper.c \
  crypto/w_jh.c \
  crypto/w_keccak.c \
  crypto/w_luffa.c \
  crypto/w_shavite.c \
  crypto/w_simd.c \
  crypto/w_sha2big.c \
  crypto/w_haval.c \
  crypto/w_panama.c \
  crypto/w_blake256.c \
  crypto/w_skein256.c \
  crypto/w_shabal.c \
  crypto/w_skein.c \
  crypto/hashpow.cpp \
  crypto/sha512.cpp \
  crypto/hmac_sha256.cpp \
  crypto/rfc6979_hmac_sha256.cpp \
  crypto/hmac_sha512.cpp \
  crypto/ripemd160.cpp \
  crypto/common.h \
  crypto/sha256.h \
  crypto/keccak256.h \
  crypto/scrypt.h  \
  crypto/blake.h \
  crypto/bmw.h \
  crypto/cubehash.h \
  crypto/echo.h \
  crypto/fugue.h \
  crypto/groestl.h \
  crypto/hamsi.h \
  crypto/hamsi_helper.h \
  crypto/jh.h \
  crypto/keccak.h \
  crypto/luffa.h \
  crypto/shavite.h \
  crypto/simd.h \
  crypto/skein.h \
  crypto/w_blake.h \
  crypto/w_bmw.h \
  crypto/w_cubehash.h \
  crypto/w_echo.h \
  crypto/w_fugue.h \
  crypto/w_groestl.h \
  crypto/w_hamsi.h \
  crypto/w_hamsi_helper.h \
  crypto/w_jh.h \
  crypto/w_keccak.h \
  crypto/w_luffa.h \
  crypto/w_shavite.h \
  crypto/w_simd.h \
  crypto/w_sha2big.h \
  crypto/w_haval.h \
  crypto/w_panama.h \
  crypto/w_blake256.h \
  crypto/w_skein256.h \
  crypto/w_shabal.h \
  crypto/w_skein.h \
  crypto/hashpow.h \
  crypto/jumphash.h \
  crypto/wutil.h \
  crypto/sha512.h \
  crypto/hmac_sha256.h \
  crypto/rfc6979_hmac_sha256.h \
  crypto/hmac_sha512.h \
  crypto/sha1.h \
  crypto/ripemd160.h

# common: shared between massgridd, and massgrid-qt and non-server tools
libmassgrid_common_a_CPPFLAGS = $(AM_CPPFLAGS) $(MASSGRID_INCLUDES)
libmassgrid_common_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libmassgrid_common_a_SOURCES = \
  amount.cpp \
  arith_uint256.cpp \
  base58.cpp \
  bip39.cpp \
  chainparams.cpp \
  coins.cpp \
  compressor.cpp \
  consensus/merkle.cpp \
  core_read.cpp \
  core_write.cpp \
  hash.cpp \
  hdchain.cpp \
  key.cpp \
  keystore.cpp \
  netaddress.cpp \
  netbase.cpp \
  primitives/block.cpp \
  primitives/transaction.cpp \
  protocol.cpp \
  pubkey.cpp \
  scheduler.cpp \
  script/interpreter.cpp \
  script/script.cpp \
  script/script_error.cpp \
  script/sign.cpp \
  script/standard.cpp \
  $(MASSGRID_CORE_H)

# util: shared between all executables.
# This library *must* be included to make sure that the glibc
# backward-compatibility objects and their sanity checks are linked.
libmassgrid_util_a_CPPFLAGS = $(AM_CPPFLAGS) $(MASSGRID_INCLUDES)
libmassgrid_util_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libmassgrid_util_a_SOURCES = \
  support/pagelocker.cpp \
  chainparamsbase.cpp \
  clientversion.cpp \
  compat/glibc_sanity.cpp \
  compat/glibcxx_sanity.cpp \
  compat/strnlen.cpp \
  random.cpp \
  rpc/protocol.cpp \
  support/cleanse.cpp \
  sync.cpp \
  uint256.cpp \
  threadinterrupt.cpp \
  util.cpp \
  utilmoneystr.cpp \
  utilstrencodings.cpp \
  utiltime.cpp \
  $(MASSGRID_CORE_H)

if GLIBC_BACK_COMPAT
libmassgrid_util_a_SOURCES += compat/glibc_compat.cpp
endif

# cli: shared between massgrid-cli and massgrid-qt
libmassgrid_cli_a_CPPFLAGS = $(AM_CPPFLAGS) $(MASSGRID_INCLUDES)
libmassgrid_cli_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libmassgrid_cli_a_SOURCES = \
  rpc/client.cpp \
  $(MASSGRID_CORE_H)

nodist_libmassgrid_util_a_SOURCES = $(srcdir)/obj/build.h
#

# massgridd binary #
massgridd_SOURCES = massgridd.cpp
massgridd_CPPFLAGS = $(AM_CPPFLAGS) $(MASSGRID_INCLUDES)
massgridd_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
massgridd_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
massgridd_SOURCES += massgridd-res.rc
endif

massgridd_LDADD = \
  $(LIBMASSGRID_SERVER) \
  $(LIBMASSGRID_COMMON) \
  $(LIBUNIVALUE) \
  $(LIBMASSGRID_UTIL) \
  $(LIBMASSGRID_CRYPTO) \
  $(LIBLEVELDB) \
  $(LIBMEMENV) \
  $(LIBSECP256K1)

if ENABLE_ZMQ
massgridd_LDADD += $(LIBMASSGRID_ZMQ) $(ZMQ_LIBS)
endif

if ENABLE_WALLET
massgridd_LDADD += libmassgrid_wallet.a
endif

massgridd_LDADD += $(BOOST_LIBS) $(BDB_LIBS) $(SSL_LIBS) $(CRYPTO_LIBS) $(MINIUPNPC_LIBS) $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS)

# massgrid-cli binary #
massgrid_cli_SOURCES = massgrid-cli.cpp
massgrid_cli_CPPFLAGS = $(AM_CPPFLAGS) $(MASSGRID_INCLUDES) $(EVENT_CFLAGS)
massgrid_cli_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
massgrid_cli_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
massgrid_cli_SOURCES += massgrid-cli-res.rc
endif

massgrid_cli_LDADD = \
  $(LIBMASSGRID_CLI) \
  $(LIBUNIVALUE) \
  $(LIBMASSGRID_UTIL)
massgrid_cli_LDADD += $(BOOST_LIBS) $(SSL_LIBS) $(CRYPTO_LIBS) $(EVENT_LIBS)
#

# massgrid-tx binary #
massgrid_tx_SOURCES = massgrid-tx.cpp
massgrid_tx_CPPFLAGS = $(AM_CPPFLAGS) $(MASSGRID_INCLUDES)
massgrid_tx_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
massgrid_tx_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
massgrid_tx_SOURCES += massgrid-tx-res.rc
endif

massgrid_tx_LDADD = \
  $(LIBUNIVALUE) \
  $(LIBMASSGRID_COMMON) \
  $(LIBMASSGRID_UTIL) \
  $(LIBMASSGRID_CRYPTO) \
  $(LIBSECP256K1)

massgrid_tx_LDADD += $(BOOST_LIBS) $(CRYPTO_LIBS)
#

# massgridconsensus library #
if BUILD_MASSGRID_LIBS
include_HEADERS = script/massgridconsensus.h
libmassgridconsensus_la_SOURCES = \
  crypto/hmac_sha512.cpp \
  crypto/ripemd160.cpp \
  crypto/sha1.cpp \
  crypto/sha256.cpp \
  crypto/sha512.cpp \
  hash.cpp \
  primitives/transaction.cpp \
  pubkey.cpp \
  script/massgridconsensus.cpp \
  script/interpreter.cpp \
  script/script.cpp \
  uint256.cpp \
  utilstrencodings.cpp

if GLIBC_BACK_COMPAT
  libmassgridconsensus_la_SOURCES += compat/glibc_compat.cpp
endif

libmassgridconsensus_la_LDFLAGS = $(AM_LDFLAGS) -no-undefined $(RELDFLAGS)
libmassgridconsensus_la_LIBADD = $(LIBSECP256K1)
libmassgridconsensus_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(builddir)/obj -I$(srcdir)/secp256k1/include -DBUILD_MASSGRID_INTERNAL
libmassgridconsensus_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)

endif
#

CLEANFILES = leveldb/libleveldb.a leveldb/libmemenv.a
CLEANFILES += *.gcda *.gcno
CLEANFILES += compat/*.gcda compat/*.gcno
CLEANFILES += consensus/*.gcda consensus/*.gcno
CLEANFILES += crypto/*.gcda crypto/*.gcno
CLEANFILES += policy/*.gcda policy/*.gcno
CLEANFILES += primitives/*.gcda primitives/*.gcno
CLEANFILES += script/*.gcda script/*.gcno
CLEANFILES += support/*.gcda support/*.gcno
CLEANFILES += univalue/*.gcda univalue/*.gcno
CLEANFILES += wallet/*.gcda wallet/*.gcno
CLEANFILES += wallet/test/*.gcda wallet/test/*.gcno
CLEANFILES += zmq/*.gcda zmq/*.gcno

DISTCLEANFILES = obj/build.h

EXTRA_DIST = leveldb

clean-local:
	-$(MAKE) -C leveldb clean
	-$(MAKE) -C secp256k1 clean
	-$(MAKE) -C univalue clean
	-rm -f leveldb/*/*.gcda leveldb/*/*.gcno leveldb/helpers/memenv/*.gcda leveldb/helpers/memenv/*.gcno
	-rm -f config.h

.rc.o:
	@test -f $(WINDRES)
	$(AM_V_GEN) $(WINDRES) -DWINDRES_PREPROC -i $< -o $@

.mm.o:
	$(AM_V_CXX) $(OBJCXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	  $(CPPFLAGS) $(AM_CXXFLAGS) $(QT_INCLUDES) $(AM_CXXFLAGS) $(PIE_FLAGS) $(CXXFLAGS) -c -o $@ $<

check-symbols: $(bin_PROGRAMS)
if GLIBC_BACK_COMPAT
	@echo "Checking glibc back compat..."
	$(AM_V_at) READELF=$(READELF) CPPFILT=$(CPPFILT) $(top_srcdir)/contrib/devtools/symbol-check.py < $(bin_PROGRAMS)
endif

check-security: $(bin_PROGRAMS)
if HARDEN
	@echo "Checking binary security..."
	$(AM_V_at) READELF=$(READELF) OBJDUMP=$(OBJDUMP) $(top_srcdir)/contrib/devtools/security-check.py < $(bin_PROGRAMS)
endif

%.pb.cc %.pb.h: %.proto
	@test -f $(PROTOC)
	$(AM_V_GEN) $(PROTOC) --cpp_out=$(@D) --proto_path=$(abspath $(<D) $<)

if EMBEDDED_LEVELDB
include Makefile.leveldb.include
endif

if ENABLE_TESTS
include Makefile.test.include
endif

if ENABLE_BENCH
include Makefile.bench.include
endif

if ENABLE_QT
include Makefile.qt.include
endif

if ENABLE_QT_TESTS
include Makefile.qttest.include
endif

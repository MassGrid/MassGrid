DIST_SUBDIRS = secp256k1
AM_LDFLAGS = $(PTHREAD_CFLAGS) $(LIBTOOL_LDFLAGS)


if EMBEDDED_LEVELDB
LEVELDB_CPPFLAGS += -I$(srcdir)/leveldb/include
LEVELDB_CPPFLAGS += -I$(srcdir)/leveldb/helpers/memenv
LIBLEVELDB += $(builddir)/leveldb/libleveldb.a
LIBMEMENV  += $(builddir)/leveldb/libmemenv.a

# NOTE: This dependency is not strictly necessary, but without it make may try to build both in parallel, which breaks the LevelDB build system in a race
$(LIBLEVELDB): $(LIBMEMENV)

$(LIBLEVELDB) $(LIBMEMENV):
	@echo "Building LevelDB ..." && $(MAKE) -C $(@D) $(@F) CXX="$(CXX)" \
	  CC="$(CC)" PLATFORM=$(TARGET_OS) AR="$(AR)" $(LEVELDB_TARGET_FLAGS) \
          OPT="$(CXXFLAGS) $(CPPFLAGS)"
endif

MLGBCOIN_CONFIG_INCLUDES=-I$(builddir)/config
MLGBCOIN_INCLUDES=-I$(builddir) -I$(builddir)/obj $(BOOST_CPPFLAGS) $(LEVELDB_CPPFLAGS) $(CRYPTO_CFLAGS) $(SSL_CFLAGS)

MLGBCOIN_INCLUDES += -I$(srcdir)/secp256k1/include

LIBMLGBCOIN_SERVER=libmlgbcoin_server.a
LIBMLGBCOIN_WALLET=libmlgbcoin_wallet.a
LIBMLGBCOIN_COMMON=libmlgbcoin_common.a
LIBMLGBCOIN_CLI=libmlgbcoin_cli.a
LIBMLGBCOIN_UTIL=libmlgbcoin_util.a
LIBMLGBCOIN_CRYPTO=crypto/libmlgbcoin_crypto.a
LIBMLGBCOIN_UNIVALUE=univalue/libmlgbcoin_univalue.a
LIBMLGBCOINQT=qt/libmlgbcoinqt.a
LIBSECP256K1=secp256k1/libsecp256k1.la

$(LIBSECP256K1): $(wildcard secp256k1/src/*) $(wildcard secp256k1/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)

# Make is not made aware of per-object dependencies to avoid limiting building parallelization
# But to build the less dependent modules first, we manually select their order here:
EXTRA_LIBRARIES = \
  crypto/libmlgbcoin_crypto.a \
  libmlgbcoin_util.a \
  libmlgbcoin_common.a \
  univalue/libmlgbcoin_univalue.a \
  libmlgbcoin_server.a \
  libmlgbcoin_cli.a
if ENABLE_WALLET
MLGBCOIN_INCLUDES += $(BDB_CPPFLAGS)
EXTRA_LIBRARIES += libmlgbcoin_wallet.a
endif

if BUILD_MLGBCOIN_LIBS
lib_LTLIBRARIES = libmlgbcoinconsensus.la
LIBMLGBCOIN_CONSENSUS=libmlgbcoinconsensus.la
else
LIBMLGBCOIN_CONSENSUS=
endif

bin_PROGRAMS =
TESTS =

if BUILD_MLGBCOIND
  bin_PROGRAMS += mlgbcoind
endif

if BUILD_MLGBCOIN_UTILS
  bin_PROGRAMS += mlgbcoin-cli mlgbcoin-tx
endif

.PHONY: FORCE
# mlgbcoin core #
MLGBCOIN_CORE_H = \
  addrman.h \
  alert.h \
  allocators.h \
  amount.h \
  base58.h \
  bloom.h \
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
  compressor.h \
  primitives/block.h \
  primitives/transaction.h \
  core_io.h \
  crypter.h \
  db.h \
  eccryptoverify.h \
  ecwrapper.h \
  hash.h \
  init.h \
  key.h \
  keystore.h \
  leveldbwrapper.h \
  limitedmap.h \
  main.h \
  merkleblock.h \
  miner.h \
  mruset.h \
  netbase.h \
  net.h \
  noui.h \
  pow.h \
  protocol.h \
  pubkey.h \
  random.h \
  rpcclient.h \
  rpcprotocol.h \
  rpcserver.h \
  script/interpreter.h \
  script/script.h \
  script/sigcache.h \
  script/sign.h \
  script/standard.h \
  script/script_error.h \
  serialize.h \
  streams.h \
  sync.h \
  threadsafety.h \
  timedata.h \
  tinyformat.h \
  txdb.h \
  txmempool.h \
  ui_interface.h \
  uint256.h \
  undo.h \
  util.h \
  utilstrencodings.h \
  utilmoneystr.h \
  utiltime.h \
  version.h \
  wallet.h \
  wallet_ismine.h \
  walletdb.h \
  compat/sanity.h

JSON_H = \
  json/json_spirit.h \
  json/json_spirit_error_position.h \
  json/json_spirit_reader.h \
  json/json_spirit_reader_template.h \
  json/json_spirit_stream_reader.h \
  json/json_spirit_utils.h \
  json/json_spirit_value.h \
  json/json_spirit_writer.h \
  json/json_spirit_writer_template.h

obj/build.h: FORCE
	@$(MKDIR_P) $(builddir)/obj
	@$(top_srcdir)/share/genbuild.sh $(abs_top_builddir)/src/obj/build.h \
	  $(abs_top_srcdir)
libmlgbcoin_util_a-clientversion.$(OBJEXT): obj/build.h

# server: shared between mlgbcoind and mlgbcoin-qt
libmlgbcoin_server_a_CPPFLAGS = $(MLGBCOIN_INCLUDES) $(MINIUPNPC_CPPFLAGS)
libmlgbcoin_server_a_SOURCES = \
  addrman.cpp \
  alert.cpp \
  bloom.cpp \
  chain.cpp \
  checkpoints.cpp \
  init.cpp \
  leveldbwrapper.cpp \
  main.cpp \
  merkleblock.cpp \
  miner.cpp \
  net.cpp \
  noui.cpp \
  pow.cpp \
  rest.cpp \
  rpcblockchain.cpp \
  rpcmining.cpp \
  rpcmisc.cpp \
  rpcnet.cpp \
  rpcrawtransaction.cpp \
  rpcserver.cpp \
  script/sigcache.cpp \
  timedata.cpp \
  txdb.cpp \
  txmempool.cpp \
  $(JSON_H) \
  $(MLGBCOIN_CORE_H)

# wallet: shared between mlgbcoind and mlgbcoin-qt, but only linked
# when wallet enabled
libmlgbcoin_wallet_a_CPPFLAGS = $(MLGBCOIN_INCLUDES)
libmlgbcoin_wallet_a_SOURCES = \
  db.cpp \
  crypter.cpp \
  rpcdump.cpp \
  rpcwallet.cpp \
  wallet.cpp \
  wallet_ismine.cpp \
  walletdb.cpp \
  $(MLGBCOIN_CORE_H)

# crypto primitives library
crypto_libmlgbcoin_crypto_a_CPPFLAGS = $(MLGBCOIN_CONFIG_INCLUDES)
crypto_libmlgbcoin_crypto_a_SOURCES = \
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
  crypto/hashpow.h \
  crypto/sha512.h \
  crypto/hmac_sha256.h \
  crypto/rfc6979_hmac_sha256.h \
  crypto/hmac_sha512.h \
  crypto/sha1.h \
  crypto/ripemd160.h

# univalue JSON library
univalue_libmlgbcoin_univalue_a_SOURCES = \
  univalue/univalue.cpp \
  univalue/univalue_read.cpp \
  univalue/univalue_write.cpp \
  univalue/univalue_escapes.h \
  univalue/univalue.h

# common: shared between mlgbcoind, and mlgbcoin-qt and non-server tools
libmlgbcoin_common_a_CPPFLAGS = $(MLGBCOIN_INCLUDES)
libmlgbcoin_common_a_SOURCES = \
  allocators.cpp \
  amount.cpp \
  base58.cpp \
  chainparams.cpp \
  coins.cpp \
  compressor.cpp \
  primitives/block.cpp \
  primitives/transaction.cpp \
  core_read.cpp \
  core_write.cpp \
  eccryptoverify.cpp \
  ecwrapper.cpp \
  hash.cpp \
  key.cpp \
  keystore.cpp \
  netbase.cpp \
  protocol.cpp \
  pubkey.cpp \
  script/interpreter.cpp \
  script/script.cpp \
  script/sign.cpp \
  script/standard.cpp \
  script/script_error.cpp \
  $(MLGBCOIN_CORE_H)

# util: shared between all executables.
# This library *must* be included to make sure that the glibc
# backward-compatibility objects and their sanity checks are linked.
libmlgbcoin_util_a_CPPFLAGS = $(MLGBCOIN_INCLUDES)
libmlgbcoin_util_a_SOURCES = \
  compat/strnlen.cpp \
  compat/glibc_sanity.cpp \
  compat/glibcxx_sanity.cpp \
  chainparamsbase.cpp \
  clientversion.cpp \
  random.cpp \
  rpcprotocol.cpp \
  sync.cpp \
  uint256.cpp \
  util.cpp \
  utilstrencodings.cpp \
  utilmoneystr.cpp \
  utiltime.cpp \
  $(MLGBCOIN_CORE_H)

if GLIBC_BACK_COMPAT
libmlgbcoin_util_a_SOURCES += compat/glibc_compat.cpp
libmlgbcoin_util_a_SOURCES += compat/glibcxx_compat.cpp
endif

# cli: shared between mlgbcoin-cli and mlgbcoin-qt
libmlgbcoin_cli_a_CPPFLAGS = $(MLGBCOIN_INCLUDES)
libmlgbcoin_cli_a_SOURCES = \
  rpcclient.cpp \
  $(MLGBCOIN_CORE_H)

nodist_libmlgbcoin_util_a_SOURCES = $(srcdir)/obj/build.h
#

# mlgbcoind binary #
mlgbcoind_LDADD = \
  $(LIBMLGBCOIN_SERVER) \
  $(LIBMLGBCOIN_COMMON) \
  $(LIBMLGBCOIN_UNIVALUE) \
  $(LIBMLGBCOIN_UTIL) \
  $(LIBMLGBCOIN_CRYPTO) \
  $(LIBLEVELDB) \
  $(LIBMEMENV) \
  $(LIBSECP256K1)

if ENABLE_WALLET
mlgbcoind_LDADD += libmlgbcoin_wallet.a
endif
mlgbcoind_SOURCES = mlgbcoind.cpp
#

if TARGET_WINDOWS
mlgbcoind_SOURCES += mlgbcoind-res.rc
endif

mlgbcoind_LDADD += $(BOOST_LIBS) $(BDB_LIBS) $(SSL_LIBS) $(CRYPTO_LIBS) $(MINIUPNPC_LIBS)
mlgbcoind_CPPFLAGS = $(MLGBCOIN_INCLUDES)
mlgbcoind_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

# mlgbcoin-cli binary #
mlgbcoin_cli_LDADD = \
  $(LIBMLGBCOIN_CLI) \
  $(LIBMLGBCOIN_UTIL) \
  $(BOOST_LIBS) \
  $(SSL_LIBS) \
  $(CRYPTO_LIBS)

mlgbcoin_cli_SOURCES = \
  mlgbcoin-cli.cpp

mlgbcoin_cli_CPPFLAGS = $(MLGBCOIN_INCLUDES)
#

# mlgbcoin-tx binary #
mlgbcoin_tx_LDADD = \
  $(LIBMLGBCOIN_UNIVALUE) \
  $(LIBMLGBCOIN_COMMON) \
  $(LIBMLGBCOIN_UTIL) \
  $(LIBMLGBCOIN_CRYPTO) \
  $(LIBSECP256K1) \
  $(BOOST_LIBS) \
  $(CRYPTO_LIBS)

mlgbcoin_tx_SOURCES = mlgbcoin-tx.cpp
mlgbcoin_tx_CPPFLAGS = $(MLGBCOIN_INCLUDES)
#
mlgbcoin_tx_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
mlgbcoin_cli_SOURCES += mlgbcoin-cli-res.rc
endif
mlgbcoin_cli_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if BUILD_MLGBCOIN_LIBS
include_HEADERS = script/mlgbcoinconsensus.h
libmlgbcoinconsensus_la_SOURCES = \
  primitives/transaction.cpp \
  crypto/hmac_sha512.cpp \
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
  crypto/hashpow.cpp \
  crypto/sha512.cpp \
  crypto/ripemd160.cpp \
  eccryptoverify.cpp \
  ecwrapper.cpp \
  hash.cpp \
  pubkey.cpp \
  script/script.cpp \
  script/interpreter.cpp \
  script/mlgbcoinconsensus.cpp \
  uint256.cpp \
  utilstrencodings.cpp

if GLIBC_BACK_COMPAT
  libmlgbcoinconsensus_la_SOURCES += compat/glibc_compat.cpp
  libmlgbcoinconsensus_la_SOURCES += compat/glibcxx_compat.cpp
endif

libmlgbcoinconsensus_la_LDFLAGS = -no-undefined $(RELDFLAGS)
libmlgbcoinconsensus_la_LIBADD = $(CRYPTO_LIBS)
libmlgbcoinconsensus_la_CPPFLAGS = $(CRYPTO_CFLAGS) -I$(builddir)/obj -DBUILD_MLGBCOIN_INTERNAL
if USE_LIBSECP256K1
libmlgbcoinconsensus_la_LIBADD += secp256k1/libsecp256k1.la
endif
endif

CLEANFILES = leveldb/libleveldb.a leveldb/libmemenv.a *.gcda *.gcno

DISTCLEANFILES = obj/build.h

EXTRA_DIST = leveldb

clean-local:
	-$(MAKE) -C leveldb clean
	-$(MAKE) -C secp256k1 clean
	rm -f leveldb/*/*.gcno leveldb/helpers/memenv/*.gcno
	-rm -f config.h

.rc.o:
	@test -f $(WINDRES)
	$(AM_V_GEN) $(WINDRES) -DWINDRES_PREPROC -i $< -o $@

.mm.o:
	$(AM_V_CXX) $(OBJCXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	  $(CPPFLAGS) $(AM_CXXFLAGS) $(QT_INCLUDES) $(CXXFLAGS)  -c -o $@ $<

%.pb.cc %.pb.h: %.proto
	@test -f $(PROTOC)
	$(AM_V_GEN) $(PROTOC) --cpp_out=$(@D) --proto_path=$(abspath $(<D) $<)

if ENABLE_TESTS
include Makefile.test.include
endif

if ENABLE_QT
include Makefile.qt.include
endif

if ENABLE_QT_TESTS
include Makefile.qttest.include
endif

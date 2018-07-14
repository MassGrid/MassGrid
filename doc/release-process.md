Release Process
====================

* Update translations, see [translation_process.md](https://github.com/massgrid/massgrid/blob/master/doc/translation_process.md#syncing-with-transifex)
* Update hardcoded [seeds](/contrib/seeds)

* * *

### First time / New builders
Check out the source code in the following directory hierarchy.

	cd /path/to/your/toplevel/build
	git clone https://github.com/massgrid/gitian.sigs.git
	git clone https://github.com/massgrid/massgrid-detached-sigs.git
	git clone https://github.com/devrandom/gitian-builder.git
	git clone https://github.com/massgrid/massgrid.git

### MassGrid maintainers/release engineers, update (commit) version in sources

	pushd ./massgrid
	contrib/verifysfbinaries/verify.sh
	configure.ac
	doc/README*
	doc/Doxyfile
	contrib/gitian-descriptors/*.yml
	src/clientversion.h (change CLIENT_VERSION_IS_RELEASE to true)

	# tag version in git

	git tag -s v(new version, e.g. 0.8.0)

	# write release notes. git shortlog helps a lot, for example:

	git shortlog --no-merges v(current version, e.g. 0.7.2)..v(new version, e.g. 0.8.0)
	popd

* * *

### Setup and perform Gitian builds

 Setup Gitian descriptors:

	pushd ./massgrid
	export SIGNER=(your Gitian key, ie bluematt, sipa, etc)
	export VERSION=(new version, e.g. 0.8.0)
	git fetch
	git checkout v${VERSION}
	popd

  Ensure your gitian.sigs are up-to-date if you wish to gverify your builds against other Gitian signatures.

	pushd ./gitian.sigs
	git pull
	popd

  Ensure gitian-builder is up-to-date to take advantage of new caching features (`e9741525c` or later is recommended).

	pushd ./gitian-builder
	git pull

### Fetch and create inputs: (first time, or when dependency versions change)

	mkdir -p inputs
	wget -P inputs https://massgrid.org/cfields/osslsigncode-Backports-to-1.7.1.patch
	wget -P inputs http://downloads.sourceforge.net/project/osslsigncode/osslsigncode/osslsigncode-1.7.1.tar.gz

 Register and download the Apple SDK: see [OS X readme](README_osx.txt) for details.

 https://developer.apple.com/devcenter/download.action?path=/Developer_Tools/xcode_6.1.1/xcode_6.1.1.dmg

 Using a Mac, create a tarball for the 10.9 SDK and copy it to the inputs directory:

	tar -C /Volumes/Xcode/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/ -czf MacOSX10.9.sdk.tar.gz MacOSX10.9.sdk

### Optional: Seed the Gitian sources cache and offline git repositories

By default, Gitian will fetch source files as needed. To cache them ahead of time:

	make -C ../massgrid/depends download SOURCES_PATH=`pwd`/cache/common

Only missing files will be fetched, so this is safe to re-run for each build.

NOTE: Offline builds must use the --url flag to ensure Gitian fetches only from local URLs. For example:
```
./bin/gbuild --url massgrid=/path/to/massgrid,signature=/path/to/sigs {rest of arguments}
```
The gbuild invocations below <b>DO NOT DO THIS</b> by default.

### Build and sign MassGrid for Linux, Windows, and OS X:

	./bin/gbuild --commit massgrid=v${VERSION} ../massgrid/contrib/gitian-descriptors/gitian-linux.yml
	./bin/gsign --signer $SIGNER --release ${VERSION}-linux --destination ../gitian.sigs/ ../massgrid/contrib/gitian-descriptors/gitian-linux.yml
	mv build/out/massgrid-*.tar.gz build/out/src/massgrid-*.tar.gz ../

	./bin/gbuild --commit massgrid=v${VERSION} ../massgrid/contrib/gitian-descriptors/gitian-win.yml
	./bin/gsign --signer $SIGNER --release ${VERSION}-win-unsigned --destination ../gitian.sigs/ ../massgrid/contrib/gitian-descriptors/gitian-win.yml
	mv build/out/massgrid-*-win-unsigned.tar.gz inputs/massgrid-win-unsigned.tar.gz
	mv build/out/massgrid-*.zip build/out/massgrid-*.exe ../

	./bin/gbuild --commit massgrid=v${VERSION} ../massgrid/contrib/gitian-descriptors/gitian-osx.yml
	./bin/gsign --signer $SIGNER --release ${VERSION}-osx-unsigned --destination ../gitian.sigs/ ../massgrid/contrib/gitian-descriptors/gitian-osx.yml
	mv build/out/massgrid-*-osx-unsigned.tar.gz inputs/massgrid-osx-unsigned.tar.gz
	mv build/out/massgrid-*.tar.gz build/out/massgrid-*.dmg ../
	popd

  Build output expected:

  1. source tarball (massgrid-${VERSION}.tar.gz)
  2. linux 32-bit and 64-bit dist tarballs (massgrid-${VERSION}-linux[32|64].tar.gz)
  3. windows 32-bit and 64-bit unsigned installers and dist zips (massgrid-${VERSION}-win[32|64]-setup-unsigned.exe, massgrid-${VERSION}-win[32|64].zip)
  4. OS X unsigned installer and dist tarball (massgrid-${VERSION}-osx-unsigned.dmg, massgrid-${VERSION}-osx64.tar.gz)
  5. Gitian signatures (in gitian.sigs/${VERSION}-<linux|{win,osx}-unsigned>/(your Gitian key)/

### Verify other gitian builders signatures to your own. (Optional)

  Add other gitian builders keys to your gpg keyring

	gpg --import ../massgrid/contrib/gitian-downloader/*.pgp

  Verify the signatures

	./bin/gverify -v -d ../gitian.sigs/ -r ${VERSION}-linux ../massgrid/contrib/gitian-descriptors/gitian-linux.yml
	./bin/gverify -v -d ../gitian.sigs/ -r ${VERSION}-win-unsigned ../massgrid/contrib/gitian-descriptors/gitian-win.yml
	./bin/gverify -v -d ../gitian.sigs/ -r ${VERSION}-osx-unsigned ../massgrid/contrib/gitian-descriptors/gitian-osx.yml

	popd

### Next steps:

Commit your signature to gitian.sigs:

	pushd gitian.sigs
	git add ${VERSION}-linux/${SIGNER}
	git add ${VERSION}-win-unsigned/${SIGNER}
	git add ${VERSION}-osx-unsigned/${SIGNER}
	git commit -a
	git push  # Assuming you can push to the gitian.sigs tree
	popd

  Wait for Windows/OS X detached signatures:
	Once the Windows/OS X builds each have 3 matching signatures, they will be signed with their respective release keys.
	Detached signatures will then be committed to the [massgrid-detached-sigs](https://github.com/massgrid/massgrid-detached-sigs) repository, which can be combined with the unsigned apps to create signed binaries.

  Create (and optionally verify) the signed OS X binary:

	pushd ./gitian-builder
	./bin/gbuild -i --commit signature=v${VERSION} ../massgrid/contrib/gitian-descriptors/gitian-osx-signer.yml
	./bin/gsign --signer $SIGNER --release ${VERSION}-osx-signed --destination ../gitian.sigs/ ../massgrid/contrib/gitian-descriptors/gitian-osx-signer.yml
	./bin/gverify -v -d ../gitian.sigs/ -r ${VERSION}-osx-signed ../massgrid/contrib/gitian-descriptors/gitian-osx-signer.yml
	mv build/out/massgrid-osx-signed.dmg ../massgrid-${VERSION}-osx.dmg
	popd

  Create (and optionally verify) the signed Windows binaries:

	pushd ./gitian-builder
	./bin/gbuild -i --commit signature=v${VERSION} ../massgrid/contrib/gitian-descriptors/gitian-win-signer.yml
	./bin/gsign --signer $SIGNER --release ${VERSION}-win-signed --destination ../gitian.sigs/ ../massgrid/contrib/gitian-descriptors/gitian-win-signer.yml
	./bin/gverify -v -d ../gitian.sigs/ -r ${VERSION}-win-signed ../massgrid/contrib/gitian-descriptors/gitian-win-signer.yml
	mv build/out/massgrid-*win64-setup.exe ../massgrid-${VERSION}-win64-setup.exe
	mv build/out/massgrid-*win32-setup.exe ../massgrid-${VERSION}-win32-setup.exe
	popd

Commit your signature for the signed OS X/Windows binaries:

	pushd gitian.sigs
	git add ${VERSION}-osx-signed/${SIGNER}
	git add ${VERSION}-win-signed/${SIGNER}
	git commit -a
	git push  # Assuming you can push to the gitian.sigs tree
	popd

-------------------------------------------------------------------------

### After 3 or more people have gitian-built and their results match:

- Create `SHA256SUMS.asc` for the builds, and GPG-sign it:
```bash
sha256sum * > SHA256SUMS
gpg --digest-algo sha256 --clearsign SHA256SUMS # outputs SHA256SUMS.asc
rm SHA256SUMS
```
(the digest algorithm is forced to sha256 to avoid confusion of the `Hash:` header that GPG adds with the SHA256 used for the files)
Note: check that SHA256SUMS itself doesn't end up in SHA256SUMS, which is a spurious/nonsensical entry.

- Upload zips and installers, as well as `SHA256SUMS.asc` from last step, to the massgrid.org server

- Update massgrid.org

- Announce the release:

  - Release on MassGrid forum: https://www.massgrid.org/forum/topic/official-announcements.54/

  - MassGrid-development mailing list

  - Update title of #massgrid on Freenode IRC

  - Optionally reddit /r/MassGridpay, ... but this will usually sort out itself

- Notify flare so that he can start building [the PPAs](https://launchpad.net/~massgrid.org/+archive/ubuntu/massgrid)

- Add release notes for the new version to the directory `doc/release-notes` in git master

- Celebrate

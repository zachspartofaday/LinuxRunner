# SteamOS ARM64 userspace experiment

The supplied SteamFrame repair image runs a terminal and an X11 KDE Plasma
desktop in LinuxRunner on a physical iPad running iPadOS 27.2, after initial
validation in the macOS ARM64 host runtime. The app offers an experimental
installer for the specifically prepared image, with a bundled archive checksum,
storage checks, cancellation and regular Linux account setup. The user confirmed
KDE's wallpaper and panel, screen taps, Magic Keyboard trackpad clicks and
hardware typing into Konsole, where `uname -m` printed `aarch64`.

## Observed results

Tested on 2026-11-02 against LinuxRunner source `d141978`, using the interpreter
with JIT disabled and a real ARM64 guest vDSO:

- Bash 5.3.3, `uname -m` (`aarch64`), user identity, file write/read and
  fork/wait passed.
- Pacman 6.1.0 ran and queried the image's existing package database:
  `bash 5.3.3-3`. Package downloads, installation and upgrades were not tested.
- Xvfb at 960 × 540, KWin X11, Plasma and Konsole rendered under a dedicated
  regular guest account. The desktop probe checked managed window classes for
  both `plasmashell` and `konsole` and captured the framebuffer.
- Graphics used Qt software rendering with KWin compositing disabled. This
  does not establish Metal, Vulkan, gamescope, Steam client or game support.
- The managed app desktop profile passed its offline preparation check, started
  Plasma as a regular user through the normal session launcher, published
  `ready=1` and answered the RFB 3.8 handshake through x11vnc on the host.
- Native importer tests passed checksum rejection, source-pin rejection,
  symlink rejection, cancellation during copying and extraction, and source
  preservation. Swift manifest tests exercise size bounds and overflow.
- The first iPad import exposed PAX UTF-8 filename conversion under iOS's C
  locale. The importer now preserves the original name bytes supplied by
  libarchive's PAX fallback for that specific conversion warning, after checking
  that the fields contain valid UTF-8. It requires no installed UTF-8 locale and
  changes no thread/process locale. Other parse/codec warnings remain fatal.
  A regression linked against the bundled libarchive 3.4.3 reproduced the old
  failure and tests non-ASCII filenames, symlink targets, malformed UTF-8 and
  truncated gzip rejection under the C locale. Archive integrity is checked
  separately against the pinned SHA-256 before extraction. Failed/cancelled
  imports also release their archive, database and file handles.
- The first installed iPad session reached X11 and KWin but its Plasma panel
  and desktop surface did not become ready within four minutes. Discover's
  Flatpak notifier crashed, and iPadOS rejected several host memory mappings.
  The SteamOS session now defaults Discover, Baloo indexing, the Python firewall
  applet and IBus out of autostart, preserving explicit per-account overrides.
  Baloo attempted a 256 GiB shared mapping; these services are outside the
  desktop/terminal milestone. The updated test app was signed with expanded
  virtual addressing through `SteamOSExperiment.entitlements`, then installed
  and launched on the iPad. This does not enable JIT or change the anonymous
  memory budget. The updated host session passed panel/surface readiness and
  ran an ARM64 shell through Konsole with those applets absent. The iPad then
  reported `ready=1` with panel and surface present after 15 seconds, and a user
  screenshot confirmed KDE's wallpaper and panel. A `tags:/` error prompted
  explicitly disabling indexing in KDE's configuration as well as autostart.
- Physical mouse testing exposed a generic input-source transition bug: the
  first GCMouse press could be accepted before UIKit identified an indirect
  pointer, after which its release was ignored and the button remained held.
  GCMouse now consistently owns physical buttons while UIKit supplies absolute
  pointer positions. Duplicate UIKit button events are suppressed; direct
  finger/Pencil clicks and UIKit-only pointers remain supported. Policy tests
  cover the transition, and input diagnostics record forwarded button edges.
  The updated app was installed and launched on the iPad. Direct screen taps
  worked before this fix, and the user confirmed Magic Keyboard trackpad clicks
  afterward. The native RFB transport probe preserved all button,
  key and wheel transitions during 9,051 queued input events, while coalescing
  redundant pointer motion.
- Subsequent iPad interaction exposed a Plasma crash in Qt's raster painter
  called by `breeze6.so`. The SteamOS session selects Qt's Fusion widget style,
  including applications activated through its session bus. Hardware typing
  also requires the native framebuffer view to become first responder: it now
  requests that focus on a desktop click when a hardware keyboard is present.
  Focus and key-event counters omit typed content. A host probe sent a command
  through the native RFB client into Konsole and verified the file written by
  that shell; this establishes transport/terminal input separately from UIKit
  hardware-keyboard focus.
- The final updated app was built, installed and launched on the same iPad.
  Its trace recorded panel/surface readiness after 18 seconds, successful native
  keyboard focus and hardware key-down/key-up delivery. The user confirmed that
  typing into Konsole worked and `uname -m` printed `aarch64`. This completes the
  initial physical desktop/terminal milestone. Longer interactive sessions,
  restarts across OS updates and Steam/game compatibility are not qualified.

The image's default Wayland session invokes
`start-gamescope-session plasma-session.target`. The experiment instead uses
`dbus-run-session -- startplasma-x11` and sets `systemdBoot=false` in the test
user's `startkderc`. Steam autostart is disabled for that account. Auxiliary
services still report missing system bus, FUSE, PipeWire and polkit integration;
the desktop result does not qualify those services or long-running interactive
use.

## Scope and dependencies

This installer PR is based on the verified-import PR (#5); it does not include
the separate keyboard, socket, virtual DRM, capture or Metal sampler changes.
The software Plasma profile is the baseline. Graphics and display enhancements
are reviewed separately. Prior physical observations above refer to the local
integration, not an independent installation of this extracted branch.

## Networking

LinuxRunner translates guest Linux sockets to iPadOS sockets. Wi-Fi and
Ethernet hardware remain managed by iPadOS; Linux Wi-Fi kernel modules and
firmware from Arch or SteamOS are not used in this userspace environment.
The app's existing DNS setup supplies the iPadOS resolver to every distro,
including SteamOS.

The maintained read-only check is `tests/linuxrunner/steamos/network.sh`. It
checks IPv4 name resolution and an HTTPS request with certificate verification
and HTTPS-only redirects. Both checks passed in the disposable host root.
On 2026-11-02 the operator also ran the equivalent `getent` and `curl` commands
in Konsole on the connected iPad and reported `DNS_OK` and `HTTPS_OK`. This
confirms DNS and outbound HTTPS on that iPad's current connection; Steam login,
downloads and other network services have not been qualified.

## Source image

Local filename: `SteamFrame OOBE Repair 2026-09-22.img.bz2`.

Local compressed-image SHA-256:

```text
3a4a077f1b1f40688ab3279affcb56776bd97c54db1573e7c65fc52a97106676
```

This records the supplied bytes; it is not an independently authenticated Valve
checksum. The initial image probes were offline. Device-image preparation adds
two pinned Arch Linux ARM display packages; it does not use the SteamOS image's
package repositories.

The image is a 7 GiB GPT disk containing a 5 GiB Btrfs `rootfs-A` partition.
Its `/etc/os-release` identifies SteamOS `0.3.0`, variant `vr`, build
`20260922.5152327`. Bash, Pacman, Xvfb, KWin and Plasma are ARM64 ELF binaries.
The root partition's type GUID advertises x86-64 despite those ARM64 contents;
the exporter checks the binaries themselves.

## Inspect and export locally

Use Python 3.10–3.14; the pinned Btrfs parser is incompatible with Python 3.15.
The tool reads a local raw `.img` or `.img.bz2`, validates GPT checksums and
SteamOS/ARM64 identity, and optionally writes an uncompressed userspace tar.
It never runs image scripts or extracts guest paths onto the host filesystem.
Existing output files are refused.

```sh
python3.14 -m venv build/steamos-tools
build/steamos-tools/bin/pip install -r tools/steamos/requirements.txt
mkdir -p build/steamos-probe

# Inspection only. Replace the image path with your local copy.
build/steamos-tools/bin/python tools/steamos/export-steamframe.py \
  '/path/to/SteamFrame OOBE Repair 2026-09-22.img.bz2' \
  --report build/steamos-probe/image-report.json

# Export separately, or combine --report and --output in one invocation.
build/steamos-tools/bin/python tools/steamos/export-steamframe.py \
  '/path/to/SteamFrame OOBE Repair 2026-09-22.img.bz2' \
  --output build/steamos-probe/steamos-rootfs.tar
```

Btrfs requires random access. A compressed input is decompressed into a
temporary file, with a default 16 GiB limit. Passing an already decompressed
image avoids repeating that work. The tested export contains 226,048 entries
and is approximately 8.4 GiB; allow space for both the archive and an imported
filesystem, plus 7 GiB of temporary space when using compressed input.

The export preserves Linux ownership, modes, symlinks and hardlinks. It omits
boot/EFI/persistent partitions, kernel modules, firmware and debug files;
`dev`, `proc`, `sys`, `run` and `tmp` are exported as empty directories. Separate
`var-A` and `home` partitions are not merged into the export. This is a
development userspace export, not a recovery or bootable image.

## Reproduce the host checks

Follow [Building LinuxRunner](BUILDING.md) for dependencies, including Meson,
Ninja, libarchive development files and `clang` with `ld.lld` on PATH. Require
the real vDSO: a stub-only host build failed Bash signal return during the
initial experiment.

```sh
meson setup build/steamos-host -Dengine=asbestos -Dguest_arch=arm64 \
  -Djit=false -Djit_emit=false -Dcli_aot= -Dvdso_required=true
ninja -C build/steamos-host ish tools/fakefsify
```

Import onto a **case-sensitive** volume. The complete image contains both
`usr/lib/Xorg` (a file) and `usr/lib/xorg` (a directory); the import fails on a
case-insensitive macOS volume. For example, create an empty test disk image:

```sh
hdiutil create -size 16g -type SPARSE -fs 'Case-sensitive APFS' \
  -volname LinuxRunnerSteamOSProbe build/steamos-probe/steamos-test.sparseimage
mkdir -p build/steamos-probe/case-sensitive
hdiutil attach -nobrowse -mountpoint "$PWD/build/steamos-probe/case-sensitive" \
  build/steamos-probe/steamos-test.sparseimage
build/steamos-host/tools/fakefsify build/steamos-probe/steamos-rootfs.tar \
  build/steamos-probe/case-sensitive/rootfs
export CPU_ROOTFS="$PWD/build/steamos-probe/case-sensitive/rootfs"
```

Run these checks only in that disposable filesystem. The desktop check writes
configuration for a dedicated test account, disables its Steam autostart, runs
for approximately 50 seconds, captures window identities and a framebuffer,
then stops the display.

```sh
build/steamos-host/ish -f "$CPU_ROOTFS" /bin/bash --noprofile --norc \
  -ec "$(cat tests/linuxrunner/steamos/terminal.sh)"

build/steamos-host/ish -f "$CPU_ROOTFS" /bin/bash --noprofile --norc -ec \
  'id -u lrprobe >/dev/null 2>&1 || useradd -m -U -s /bin/bash lrprobe; exec runuser -u lrprobe -- /bin/bash --noprofile --norc -ec "$1"' \
  bash "$(cat tests/linuxrunner/steamos/desktop.sh)"
```

Expected markers are `STEAMOS_TERMINAL=PASS` and `STEAMOS_DESKTOP=PASS`.
Desktop logs and the XWD framebuffer are under the guest path
`/home/lrprobe/steamframe-probe/`. Save desired evidence before detaching the
test volume:

```sh
hdiutil detach "$PWD/build/steamos-probe/case-sensitive"
```

The exporter contract tests run without third-party dependencies:

```sh
python3.14 -m unittest discover -s tests/linuxrunner/steamos -v
```

## Prepared device image

The app installs a prepared `rootfs.tar.gz`, rather than the raw disk image.
`SteamOSSupport.bundle/manifest.json` pins its exact size, SHA-256, original
image identity and expected installed size. Other distributions retain their
512 MiB import limit; only a matching SteamOS pin admits the larger archive.
Copying, verification and extraction run off the UI thread with cooperative
cancellation. A completed filesystem is published under a new environment ID;
the original archive and existing environments are kept.

The device preparation tool requires the disposable case-sensitive fakefs root
from the previous steps. It adds checksum-pinned x11vnc 0.9.17 and
libvncserver 0.9.15 packages, records them in the SteamOS package database, and
builds the four LinuxRunner desktop helpers using that image's GCC. Their
checksums and source identities are checked by the app's offline desktop
profile. The root must be imported from the supplied image; use it only for
preparation, before creating test users or running desktop probes.

```sh
ninja -C build/steamos-host tools/unfakefsify
python3.14 tools/steamos/prepare-device-image.py \
  --source-image '/path/to/SteamFrame OOBE Repair 2026-09-22.img.bz2' \
  --root "$CPU_ROOTFS" --cache build/steamos-arch-cache \
  --output build/steamos-package \
  --app-manifest app/LinuxRunner/SteamOSSupport.bundle/manifest.json
```

The prepared archive for this candidate is 3,883,082,382 bytes. Installation
needs approximately 13.3 GB of additional free space for its verified snapshot,
expanded filesystem and working reserve. Rebuilding the prepared image may
change its checksum: rebuild the app with the corresponding generated manifest.
This is a local-image workflow; the app provides no SteamOS download endpoint.

Build and install the normal LinuxRunner scheme using your Apple development
team. For a separate test installation, override `ROOT_BUNDLE_IDENTIFIER` and
`LINUXRUNNER_DISPLAY_NAME` and use `app/LinuxRunner/SteamOSExperiment.entitlements`
for private-container storage with expanded virtual addressing on supported
devices. The signing profile must allow that entitlement.
Do not override `PRODUCT_NAME` globally: that also renames dependent libraries.

Copy the prepared archive to the test app's `Documents/SteamOS/rootfs.tar.gz`,
or select it through **Choose Prepared SteamOS Archive…**. Open
**Distributions → Set Up Linux → Continue → SteamOS ARM64 → Continue →
Install Prepared SteamOS**. Keep the app in the foreground until completion.
When requested, force-close and reopen the app, create or select a regular
Linux account, then choose **Check SteamOS Desktop → Continue → Open Desktop**.

The initial desktop uses Plasma X11 with software rendering and Steam autostart
disabled for newly configured accounts. Installed-package inspection is
available; managed repository updates and package changes are disabled for
this hardware-specific image experiment. Audio and system services remain
outside this milestone. Gamescope, Vulkan and local games are separate work.

The isolated storage-lifecycle change and allowlisted before/after evidence were
submitted upstream as [PR #18](https://github.com/mxhm11cr/LinuxRunner/pull/18).

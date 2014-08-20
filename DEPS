use_relative_paths = True

vars = {
  "libyuv_trunk" : "https://libyuv.googlecode.com/svn/trunk",

  # Override root_dir in your .gclient's custom_vars to specify a custom root
  # folder name.
  "root_dir": "trunk",
  "extra_gyp_flag": "-Dextra_gyp_flag=0",

  # Use this googlecode_url variable only if there is an internal mirror for it.
  # If you do not know, use the full path while defining your new deps entry.
  "googlecode_url": "http://%s.googlecode.com/svn",
  "chromium_trunk" : "http://src.chromium.org/svn/trunk",
  # chrome://version/ for revision of canary Chrome.
  # http://chromium-status.appspot.com/lkgr is a last known good revision.
  "chromium_revision": "280149",
}

# NOTE: Prefer revision numbers to tags for svn deps. Use http rather than
# https; the latter can cause problems for users behind proxies.
deps = {
  "../chromium_deps":
    File(Var("chromium_trunk") + "/src/DEPS@" + Var("chromium_revision")),

  "../chromium_gn":
    File(Var("chromium_trunk") + "/src/.gn@" + Var("chromium_revision")),

  "build":
    Var("chromium_trunk") + "/src/build@" + Var("chromium_revision"),

  "buildtools":
    From("chromium_deps", "src/buildtools"),

  # Needed by common.gypi.
  "google_apis/build":
    Var("chromium_trunk") + "/src/google_apis/build@" + Var("chromium_revision"),

  "testing":
    Var("chromium_trunk") + "/src/testing@" + Var("chromium_revision"),

  "testing/gtest":
    From("chromium_deps", "src/testing/gtest"),

  "tools/clang":
    Var("chromium_trunk") + "/src/tools/clang@" + Var("chromium_revision"),

  "tools/gn":
    Var("chromium_trunk") + "/src/tools/gn@" + Var("chromium_revision"),

  "tools/gyp":
    From("chromium_deps", "src/tools/gyp"),

  "tools/memory":
    Var("chromium_trunk") + "/src/tools/memory@" + Var("chromium_revision"),

  "tools/python":
    Var("chromium_trunk") + "/src/tools/python@" + Var("chromium_revision"),

  "tools/sanitizer_options":
    File(Var("chromium_trunk") + "/src/base/debug/sanitizer_options.cc@" + Var("chromium_revision")),

  "tools/tsan_suppressions":
    File(Var("chromium_trunk") + "/src/base/debug/tsan_suppressions.cc@" + Var("chromium_revision")),

  "tools/valgrind":
    Var("chromium_trunk") + "/src/tools/valgrind@" + Var("chromium_revision"),

  # Needed by build/common.gypi.
  "tools/win/supalink":
    Var("chromium_trunk") + "/src/tools/win/supalink@" + Var("chromium_revision"),

  "third_party/binutils":
    Var("chromium_trunk") + "/src/third_party/binutils@" + Var("chromium_revision"),

  "third_party/libc++":
    Var("chromium_trunk") + "/src/third_party/libc++@" + Var("chromium_revision"),

  "third_party/libc++/trunk":
    From("chromium_deps", "src/third_party/libc++/trunk"),

  "third_party/libc++abi":
    Var("chromium_trunk") + "/src/third_party/libc++abi@" + Var("chromium_revision"),

  "third_party/libc++abi/trunk":
    From("chromium_deps", "src/third_party/libc++abi/trunk"),

  "third_party/libjpeg_turbo":
    From("chromium_deps", "src/third_party/libjpeg_turbo"),

  # Yasm assember required for libjpeg_turbo
  "third_party/yasm":
    Var("chromium_trunk") + "/src/third_party/yasm@" + Var("chromium_revision"),

  "third_party/yasm/source/patched-yasm":
    Var("chromium_trunk") + "/deps/third_party/yasm/patched-yasm@" + Var("chromium_revision"),
}

deps_os = {
  "win": {
    # Use WebRTC's, stripped down, version of Cygwin (required by GYP).
    "third_party/cygwin":
      (Var("googlecode_url") % "webrtc") + "/deps/third_party/cygwin@2672",

    # Used by libjpeg-turbo.
    # TODO(fbarchard): Remove binaries and run yasm from build folder.
    "third_party/yasm/binaries":
      Var("chromium_trunk") + "/deps/third_party/yasm/binaries@" + Var("chromium_revision"),
    "third_party/yasm": None,

    "tools/find_depot_tools":
      File(Var("chromium_trunk") + "/src/tools/find_depot_tools.py@" + Var("chromium_revision")),
  },
  "android": {
    "third_party/android_tools":
      From("chromium_deps", "src/third_party/android_tools"),

    "third_party/libjpeg":
      Var("chromium_trunk") + "/src/third_party/libjpeg@" + Var("chromium_revision"),
  },
  "ios": {
    # NSS, for SSLClientSocketNSS.
    "third_party/nss":
      From("chromium_deps", "src/third_party/nss"),

    "net/third_party/nss":
      Var("chromium_trunk") + "/src/net/third_party/nss@" + Var("chromium_revision"),

    # class-dump utility to generate header files for undocumented SDKs.
    "testing/iossim/third_party/class-dump":
      From("chromium_deps", "src/testing/iossim/third_party/class-dump"),

    # Helper for running under the simulator.
    "testing/iossim":
      Var("chromium_trunk") + "/src/testing/iossim@" + Var("chromium_revision"),
  },
}

hooks = [
  {
    # Copy .gn from temporary place (../chromium_gn) to root_dir.
    "name": "copy .gn",
    "pattern": ".",
    "action": ["python", Var("root_dir") + "/build/cp.py",
               Var("root_dir") + "/../chromium_gn/.gn",
               Var("root_dir")],
  },
  # Pull GN binaries. This needs to be before running GYP below.
  {
    "name": "gn_win",
    "pattern": ".",
    "action": [ "download_from_google_storage",
                "--no_resume",
                "--platform=win32",
                "--no_auth",
                "--bucket", "chromium-gn",
                "-s", Var("root_dir") + "/buildtools/win/gn.exe.sha1",
    ],
  },
  {
    "name": "gn_mac",
    "pattern": ".",
    "action": [ "download_from_google_storage",
                "--no_resume",
                "--platform=darwin",
                "--no_auth",
                "--bucket", "chromium-gn",
                "-s", Var("root_dir") + "/buildtools/mac/gn.sha1",
    ],
  },
  {
    "name": "gn_linux",
    "pattern": ".",
    "action": [ "download_from_google_storage",
                "--no_resume",
                "--platform=linux*",
                "--no_auth",
                "--bucket", "chromium-gn",
                "-s", Var("root_dir") + "/buildtools/linux64/gn.sha1",
    ],
  },
  {
    "name": "gn_linux32",
    "pattern": ".",
    "action": [ "download_from_google_storage",
                "--no_resume",
                "--platform=linux*",
                "--no_auth",
                "--bucket", "chromium-gn",
                "-s", Var("root_dir") + "/buildtools/linux32/gn.sha1",
    ],
  },
  {
    # Remove GN binaries from tools/gn/bin that aren't used anymore.
    # TODO(kjellander) remove after the end of July, 2014.
    "name": "remove_old_gn_binaries",
    "pattern": ".",
    "action": ["python", Var("root_dir") + "/tools/gn/bin/rm_binaries.py"],
  },
  {
    # Pull clang on mac. If nothing changed, or on non-mac platforms, this takes
    # zero seconds to run. If something changed, it downloads a prebuilt clang.
    "pattern": ".",
    "action": ["python", Var("root_dir") + "/tools/clang/scripts/update.py",
               "--if-needed"],
  },
  {
  # Update the Windows toolchain if necessary.
    "name": "win_toolchain",
    "pattern": ".",
    "action": ["python", Var("root_dir") + "/download_vs_toolchain.py",
               "update"],
  },
  {
    # Pull binutils for gold.
    "name": "binutils",
    "pattern": ".",
    "action": ["python", Var("root_dir") + "/third_party/binutils/download.py"],
  },
  {
    # A change to a .gyp, .gypi, or to GYP itself should run the generator.
    "pattern": ".",
    "action": ["python", Var("root_dir") + "/gyp_libyuv"],
  },
]

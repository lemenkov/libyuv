# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Autocompletion config for YouCompleteMe in Chromium.
#
# USAGE:
#
#   1. Install YCM [https://github.com/Valloric/YouCompleteMe]
#          (Googlers should check out [go/ycm])
#
#   2. Create a symbolic link to this file called .ycm_extra_conf.py in the
#      directory above your Chromium checkout (i.e. next to your .gclient file).
#
#          cd src
#          ln -rs tools/vim/chromium.ycm_extra_conf.py ../.ycm_extra_conf.py
#
#   3. (optional) Whitelist the .ycm_extra_conf.py from step #2 by adding the
#      following to your .vimrc:
#
#          let g:ycm_extra_conf_globlist=['<path to .ycm_extra_conf.py>']
#
#      You can also add other .ycm_extra_conf.py files you want to use to this
#      list to prevent excessive prompting each time you visit a directory
#      covered by a config file.
#
#   4. Profit
#
#
# Usage notes:
#
#   * You must use ninja & clang to build Chromium.
#
#   * You must have run gyp_chromium and built Chromium recently.
#
#
# Hacking notes:
#
#   * The purpose of this script is to construct an accurate enough command line
#     for YCM to pass to clang so it can build and extract the symbols.
#
#   * Right now, we only pull the -I and -D flags. That seems to be sufficient
#     for everything I've used it for.
#
#   * That whole ninja & clang thing? We could support other configs if someone
#     were willing to write the correct commands and a parser.
#
#   * This has only been tested on gPrecise.


import os
import os.path
import re
import shlex
import subprocess
import sys

# A dictionary mapping Clang binary path to a list of Clang command line
# arguments that specify the system include paths. It is used as a cache of the
# system include options since these options aren't expected to change per
# source file for the same clang binary. SystemIncludeDirectoryFlags() updates
# this map each time it runs a Clang binary to determine system include paths.
#
# Entries look like:
#   '/home/username/my-llvm/bin/clang++': ['-isystem',
#        '/home/username/my-llvm/include', '-isystem', '/usr/include']
_clang_system_include_map = {}


# Flags from YCM's default config.
_default_flags = [
  '-DUSE_CLANG_COMPLETER',
  '-std=c++11',
  '-x',
  'c++',
]


def FallbackSystemIncludeDirectoryFlags():
  """Returns a best guess list of system include directory flags for Clang.

  If Ninja doesn't give us a build step that specifies a Clang invocation or if
  something goes wrong while determining the system include paths, then this
  function can be used to determine some set of values that's better than
  nothing.

  Returns:
    (List of Strings) Compiler flags that specify the system include paths.
  """
  if _clang_system_include_map:
    return _clang_system_include_map.itervalues().next()
  return []


def SystemIncludeDirectoryFlags(clang_binary, clang_flags):
  """Determines compile flags for specifying system include directories.

  Use as a workaround for https://github.com/Valloric/YouCompleteMe/issues/303

  Caches the results of determining the system include directories in
  _clang_system_include_map.  Subsequent calls to SystemIncludeDirectoryFlags()
  uses the cached results for the same binary even if |clang_flags| differ.

  Args:
    clang_binary: (String) Path to clang binary.
    clang_flags: (List of Strings) List of additional flags to clang. It may
      affect the choice of system include directories if -stdlib= is specified.
      _default_flags are always included in the list of flags passed to clang.

  Returns:
    (List of Strings) Compile flags to append.
  """

  if clang_binary in _clang_system_include_map:
    return _clang_system_include_map[clang_binary]

  all_clang_flags = [] + _default_flags
  all_clang_flags += [flag for flag in clang_flags
                if flag.startswith('-std=') or flag.startswith('-stdlib=')]
  all_clang_flags += ['-v', '-E', '-']
  try:
    with open(os.devnull, 'rb') as DEVNULL:
      output = subprocess.check_output([clang_binary] + all_clang_flags,
                                       stdin=DEVNULL, stderr=subprocess.STDOUT)
  except:
    # Even though we couldn't figure out the flags for the given binary, if we
    # have results from another one, we'll use that. This logic assumes that the
    # list of default system directories for one binary can be used with
    # another.
    return FallbackSystemIncludeDirectoryFlags()
  includes_regex = r'#include <\.\.\.> search starts here:\s*' \
                   r'(.*?)End of search list\.'
  includes = re.search(includes_regex, output.decode(), re.DOTALL).group(1)
  system_include_flags = []
  for path in includes.splitlines():
    path = path.strip()
    if os.path.isdir(path):
      system_include_flags.append('-isystem')
      system_include_flags.append(path)
  if system_include_flags:
    _clang_system_include_map[clang_binary] = system_include_flags
  return system_include_flags


def PathExists(*args):
  return os.path.exists(os.path.join(*args))


def FindChromeSrcFromFilename(filename):
  """Searches for the root of the Chromium checkout.

  Simply checks parent directories until it finds .gclient and src/.

  Args:
    filename: (String) Path to source file being edited.

  Returns:
    (String) Path of 'src/', or None if unable to find.
  """
  curdir = os.path.normpath(os.path.dirname(filename))
  while not (os.path.basename(os.path.realpath(curdir)) == 'src'
             and PathExists(curdir, 'DEPS')
             and (PathExists(curdir, '..', '.gclient')
                  or PathExists(curdir, '.git'))):
    nextdir = os.path.normpath(os.path.join(curdir, '..'))
    if nextdir == curdir:
      return None
    curdir = nextdir
  return curdir


def GetDefaultCppFile(chrome_root, filename):
  """Returns the default target file to use for |filename|.

  The default target is some source file that is known to exist and loosely
  related to |filename|. Compile flags used to build the default target is
  assumed to be a close-enough approximation for building |filename|.

  Args:
    chrome_root: (String) Absolute path to the root of Chromium checkout.
    filename: (String) Absolute path to the target source file.

  Returns:
    (String) Absolute path to substitute target file.
  """
  blink_root = os.path.join(chrome_root, 'third_party', 'WebKit')
  if filename.startswith(blink_root):
    return os.path.join(blink_root, 'Source', 'core', 'Init.cpp')
  else:
    return os.path.join(chrome_root, 'base', 'logging.cc')


def GetBuildTargetForSourceFile(chrome_root, filename):
  """Returns a build target corresponding to |filename|.

  Args:
    chrome_root: (String) Absolute path to the root of Chromium checkout.
    filename: (String) Absolute path to the target source file.

  Returns:
    (String) Absolute path to build target.
  """
  if filename.endswith('.h'):
    # Header files can't be built. Instead, try to match a header file to its
    # corresponding source file.
    alternates = ['.cc', '.cpp', '.c']
    for alt_extension in alternates:
      alt_name = filename[:-2] + alt_extension
      if os.path.exists(alt_name):
        return alt_name

    # Failing that, build a default file instead and assume that the resulting
    # commandline options are valid for the .h file.
    return GetDefaultCppFile(chrome_root, filename)

  return filename


def GetClangCommandLineFromNinjaForFilename(out_dir, filename):
  """Returns the Clang command line for building |filename|

  Asks ninja for the list of commands used to build |filename| and returns the
  final Clang invocation.

  Args:
    out_dir: (String) Absolute path to ninja build output directory.
    filename: (String) Absolute path to source file.

  Returns:
    (String) Clang command line or None if command line couldn't be determined.
  """
  # Ninja needs the path to the source file relative to the output build
  # directory.
  rel_filename = os.path.relpath(os.path.realpath(filename), out_dir)

  # Ask ninja how it would build our source file.
  p = subprocess.Popen(['ninja', '-v', '-C', out_dir, '-t',
                        'commands', rel_filename + '^'],
                       stdout=subprocess.PIPE)
  stdout, stderr = p.communicate()
  if p.returncode:
    return None

  # Ninja might execute several commands to build something. We want the last
  # clang command.
  for line in reversed(stdout.split('\n')):
    if 'clang' in line:
      return line
  return None


def GetNormalizedClangCommand(command, out_dir):
  """Gets the normalized Clang binary path if |command| is a Clang command.

  Args:
    command: (String) Clang command.
    out_dir: (String) Absolute path the ninja build directory.

  Returns:
    (String or None)
      None : if command is not a clang command.
      Absolute path to clang binary : if |command| is an absolute or relative
          path to clang. If relative, it is assumed to be relative to |out_dir|.
      |command|: if command is a name of a binary.
  """
  if command.endswith('clang++') or command.endswith('clang'):
    if os.path.basename(command) == command:
      return command
    return os.path.normpath(os.path.join(out_dir, command))
  return None


def GetClangOptionsFromCommandLine(clang_commandline, out_dir,
                                   additional_flags):
  """Extracts relevant command line options from |clang_commandline|

  Args:
    clang_commandline: (String) Full Clang invocation.
    out_dir: (String) Absolute path to ninja build directory. Relative paths in
        the command line are relative to |out_dir|.
    additional_flags: (List of String) Additional flags to return.

  Returns:
    ((List of Strings), (List of Strings)) The first item in the tuple is a list
    of command line flags for this source file. The second item in the tuple is
    a list of command line flags that define the system include paths. Either or
    both can be empty.
  """
  chrome_flags = [] + additional_flags
  system_include_flags = []

  # Parse flags that are important for YCM's purposes.
  clang_tokens = shlex.split(clang_commandline)
  for flag in clang_tokens:
    if flag.startswith('-I'):
      # Relative paths need to be resolved, because they're relative to the
      # output dir, not the source.
      if flag[2] == '/':
        chrome_flags.append(flag)
      else:
        abs_path = os.path.normpath(os.path.join(out_dir, flag[2:]))
        chrome_flags.append('-I' + abs_path)
    elif flag.startswith('-std'):
      chrome_flags.append(flag)
    elif flag.startswith('-') and flag[1] in 'DWFfmO':
      if flag == '-Wno-deprecated-register' or flag == '-Wno-header-guard':
        # These flags causes libclang (3.3) to crash. Remove it until things
        # are fixed.
        continue
      chrome_flags.append(flag)

  # Assume that the command for invoking clang++ looks like one of the
  # following:
  #   1) /path/to/clang/clang++ arguments
  #   2) /some/wrapper /path/to/clang++ arguments
  #
  # We'll look at the first two tokens on the command line to see if they look
  # like Clang commands, and if so use it to determine the system include
  # directory flags.
  for command in clang_tokens[0:2]:
    normalized_command = GetNormalizedClangCommand(command, out_dir)
    if normalized_command:
      system_include_flags += SystemIncludeDirectoryFlags(normalized_command,
                                                          chrome_flags)
      break

  return (chrome_flags, system_include_flags)


def GetClangOptionsFromNinjaForFilename(chrome_root, filename):
  """Returns the Clang command line options needed for building |filename|.

  Command line options are based on the command used by ninja for building
  |filename|. If |filename| is a .h file, uses its companion .cc or .cpp file.
  If a suitable companion file can't be located or if ninja doesn't know about
  |filename|, then uses default source files in Blink and Chromium for
  determining the commandline.

  Args:
    chrome_root: (String) Path to src/.
    filename: (String) Absolute path to source file being edited.

  Returns:
    ((List of Strings), (List of Strings)) The first item in the tuple is a list
    of command line flags for this source file. The second item in the tuple is
    a list of command line flags that define the system include paths. Either or
    both can be empty.
  """
  if not chrome_root:
    return ([],[])

  # Generally, everyone benefits from including Chromium's src/, because all of
  # Chromium's includes are relative to that.
  additional_flags = ['-I' + os.path.join(chrome_root)]

  # Version of Clang used to compile Chromium can be newer then version of
  # libclang that YCM uses for completion. So it's possible that YCM's libclang
  # doesn't know about some used warning options, which causes compilation
  # warnings (and errors, because of '-Werror');
  additional_flags.append('-Wno-unknown-warning-option')

  sys.path.append(os.path.join(chrome_root, 'tools', 'vim'))
  from ninja_output import GetNinjaOutputDirectory
  out_dir = os.path.realpath(GetNinjaOutputDirectory(chrome_root))

  clang_line = GetClangCommandLineFromNinjaForFilename(
      out_dir, GetBuildTargetForSourceFile(chrome_root, filename))
  if not clang_line:
    # If ninja didn't know about filename or it's companion files, then try a
    # default build target. It is possible that the file is new, or build.ninja
    # is stale.
    clang_line = GetClangCommandLineFromNinjaForFilename(
        out_dir, GetDefaultCppFile(chrome_root, filename))

  return GetClangOptionsFromCommandLine(clang_line, out_dir, additional_flags)


def FlagsForFile(filename):
  """This is the main entry point for YCM. Its interface is fixed.

  Args:
    filename: (String) Path to source file being edited.

  Returns:
    (Dictionary)
      'flags': (List of Strings) Command line flags.
      'do_cache': (Boolean) True if the result should be cached.
  """
  abs_filename = os.path.abspath(filename)
  chrome_root = FindChromeSrcFromFilename(abs_filename)
  (chrome_flags, system_include_flags) = GetClangOptionsFromNinjaForFilename(
      chrome_root, abs_filename)

  # If either chrome_flags or system_include_flags could not be determined, then
  # assume that was due to a transient failure. Preventing YCM from caching the
  # flags allows us to try to determine the flags again.
  should_cache_flags_for_file = \
      bool(chrome_flags) and bool(system_include_flags)

  if not system_include_flags:
    system_include_flags = FallbackSystemIncludeDirectoryFlags()
  final_flags = _default_flags + chrome_flags + system_include_flags

  return {
    'flags': final_flags,
    'do_cache': should_cache_flags_for_file
  }

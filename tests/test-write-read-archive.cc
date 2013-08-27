// -*- Mode: C++ -*-
//
// Copyright (C) 2013 Red Hat, Inc.
//
// This file is part of the GNU Application Binary Interface Generic
// Analysis and Instrumentation Library (libabigail).  This library is
// free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the
// Free Software Foundation; either version 3, or (at your option) any
// later version.

// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Lesser Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this program; see the file COPYING-LGPLV3.  If
// not, see <http://www.gnu.org/licenses/>.

#include <cstdlib>
#include <iostream>
#include <fstream>
#include "test-utils.h"
#include "abg-ir.h"
#include "abg-corpus.h"

struct InOutSpec
{
  const char* in_path;
  const char* out_path;
};// end struct InOutSpec

/// This is an aggregate that specifies where the test gets the
/// elements that it reads to build an archive.  It also specifies
/// where to write the output result of the element that is written
/// back to disk, for diffing purposes.
InOutSpec archive_elements[] =
{
  {
    "data/test-write-read-archive/test0.xml",
    "output/test-write-read-archive/test0.xml",
  },
  {
    "data/test-write-read-archive/test1.xml",
    "output/test-write-read-archive/test2.xml",
  },
  {
    "data/test-write-read-archive/test2.xml",
    "output/test-write-read-archive/test2.xml",
  },
  {
    "data/test-write-read-archive/test3.xml",
    "output/test-write-read-archive/test3.xml",
  },
  {
    "data/test-write-read-archive/test4.xml",
    "output/test-write-read-archive/test4.xml",
  },
  // This should be the last entry.
  {NULL, NULL}
};

#define NUM_ARCHIVES_ELEMENTS \
  ((sizeof(archive_elements) / sizeof(InOutSpec)) -1)

/// Where to write the archive, and where to read it from to get the
/// base for the diffing.
const InOutSpec archive_spec =
{
  "data/test-write-read-archive/archive.abi",
  "output/test-write-read-archive/archive.abi"
};

using std::string;
using std::cerr;
using std::ofstream;
using std::tr1::shared_ptr;
using abigail::corpus;
using abigail::translation_unit;

int
main()
{
  // Read the elements into abigail::translation_unit and stick them
  // into an abigail::corpus.
  string in_path, out_path;
  bool is_ok = true;

  out_path =
    abigail::tests::get_build_dir() + "/tests/" + archive_spec.out_path;

  if (!abigail::tests::ensure_parent_dir_created(out_path))
    {
      cerr << "Could not create parent director for " << out_path;
      return 1;
    }
  corpus abi_corpus(out_path);

  for (InOutSpec *s = archive_elements; s->in_path; ++s)
    {
      in_path = abigail::tests::get_src_dir() + "/tests/" + s->in_path;
      shared_ptr<abigail::translation_unit> tu(new translation_unit(in_path));
      if (!tu->read())
	{
	  cerr << "failed to read " << in_path << "\n";
	  is_ok = false;
	  continue;
	}
      abi_corpus.add(tu);
    }

  if (!abi_corpus.write())
    {
      cerr  << "failed to write archive file: " << abi_corpus.get_file_path();
      return 1;
    }

  // Diff the archive members.
  //
  // Basically, re-read the corpus from disk, walk the loaded
  // translation units, write them back and diff them against their
  // reference.

  abi_corpus.drop_translation_units();
  if (abi_corpus.get_translation_units().size())
    {
      cerr << "In-memory object of abi corpus at '"
	   << abi_corpus.get_file_path()
	   << "' still has translation units after call to "
	      "corpus::drop_translation_units!";
      return false;
    }

  if (!abi_corpus.read())
    {
      cerr << "Failed to load the abi corpus from path '"
	   << abi_corpus.get_file_path()
	   << "'";
      return 1;
    }

  if (abi_corpus.get_translation_units().size() != NUM_ARCHIVES_ELEMENTS)
    {
      cerr << "Read " << abi_corpus.get_translation_units().size()
	   << " elements from the abi corpus at "
	   << abi_corpus.get_file_path()
	   << " instead of "
	   << NUM_ARCHIVES_ELEMENTS;
      return 1;
    }

  for (unsigned i = 0; i < NUM_ARCHIVES_ELEMENTS; ++i)
    {
      InOutSpec& spec = archive_elements[i];
      out_path =
	abigail::tests::get_build_dir() + "/tests/" + spec.out_path;

      if (!abi_corpus.get_translation_units()[i]->write(out_path))
	{
	  cerr << "Failed to serialize translation_unit to '"
	       << out_path
	       << "'\n";
	  is_ok = false;
	}

      string ref =
	abigail::tests::get_src_dir() + "/tests/" + spec.in_path;
      string cmd = "diff -u " + ref + " " + out_path;

      if (system(cmd.c_str()))
	is_ok = false;
    }

  return !is_ok;
}

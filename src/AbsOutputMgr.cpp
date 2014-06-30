// -*- mode: C++ -*-
//
// Copyright (c) 2007, 2008, 2009, 2010, 2011 The University of Utah
// All rights reserved.
//
// This file is part of `csmith', a random generator of C programs.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "AbsOutputMgr.h"

#include <cassert>
#include <sstream>
#include "Common.h"
#include "CGOptions.h"
#include "platform.h"
#include "Bookkeeper.h"
#include "FunctionInvocation.h"
#include "Function.h"
#include "VariableSelector.h"
#include "CGContext.h"
#include "Constant.h"
#include "ArrayVariable.h"
#include "Fact.h"
#include "random.h"  

static const char runtime_include[] = "\
#include \"csmith.h\"\n\
";

static const char volatile_include[] = "\
/* To use wrapper functions, compile this program with -DWRAP_VOLATILES=1. */\n\
#include \"volatile_runtime.h\"\n\
";

static const char access_once_macro[] = "\
#ifndef ACCESS_ONCE\n\
#define ACCESS_ONCE(v) (*(volatile typeof(v) *)&(v))\n\
#endif\n\
";

using namespace std;

// Initialize an output manager with:
// 1) create the output stream
// 2) set the tab indent to 0
void
AbsOutputMgr::Init()
{ 
	ofile_ = NULL;
	std::string ofile_str = CGOptions::output_file();

	if (!ofile_str.empty()) {
		ofile_ = new ofstream(ofile_str.c_str());  
	} 

	indent_ = 0; 
	newline_ = "\n"; 
} 

AbsOutputMgr::~AbsOutputMgr()
{ 
	if (ofile_)
		ofile_->close();
	delete ofile_;
}

void
AbsOutputMgr::OutputTail()
{
	ostream& out = Out();
	if (!CGOptions::concise()) {
		out << endl << "/************************ statistics *************************" << endl;
		Bookkeeper::output_statistics(out); 
		out << "********************* end of statistics **********************/" << endl;
		out << endl;
	}
}

void 
AbsOutputMgr::OutputProgramHeader(int argc, char *argv[], unsigned long seed)
{
	std::ostream &out = Out();
	if (CGOptions::concise()) {
		out << "// Options:  ";
		if (argc <= 1) {
			out << " (none)";
		} else {
			for (int i = 1; i < argc; ++i) {
				out << " " << argv[i];
			}
		}
		out << endl;
	}
	else {
		out << "/*" << endl;
		out << " * This is a RANDOMLY GENERATED PROGRAM." << endl;
		out << " *" << endl;
		out << " * Generator: " << PACKAGE_STRING << endl;
#ifdef GIT_VERSION
		out << " * Git version: " << GIT_VERSION << endl;
#endif
		out << " * Options:  ";
		if (argc <= 1) {
			out << " (none)";
		} else {
			for (int i = 1; i < argc; ++i) {
				out << " " << argv[i];
			}
		}
		out << endl;
		out << " * Seed:      " << seed << endl;
		out << " */" << endl;
		out << endl;
	}

	if (!CGOptions::longlong()) {
		out << endl;
		out << "#define NO_LONGLONG" << std::endl;
		out << endl;
	} 

	out << runtime_include << endl;

 	if (!CGOptions::compute_hash()) {
		if (CGOptions::allow_int64())
			out << "volatile uint64_t " << Variable::sink_var_name << " = 0;" << endl;
		else
			out << "volatile uint32_t " << Variable::sink_var_name << " = 0;" << endl;
	}
	out << endl;

	out << "static long __undefined;" << endl;
	out << endl;

	if (CGOptions::depth_protect()) {
		out << "#define MAX_DEPTH (5)" << endl;
		// Make depth signed, to cover our tails.
		out << "int32_t DEPTH = 0;" << endl;
		out << endl;
	}

	// out << platform_include << endl;
	if (CGOptions::wrap_volatiles()) {
		out << volatile_include << endl;
	}

	if (CGOptions::access_once()) {
		out << access_once_macro << endl;
	} 
} 
 
void
AbsOutputMgr::PrintFacts(const vector<const Fact*>& facts)
{
	for (size_t i=0; i<facts.size(); i++) {
		const Fact* f = facts[i];
		Out() << Comment2Str(f->ToString()) + newline_;
	}
}

void
AbsOutputMgr::PrintVarFacts(const vector<const Fact*>& facts, const char* vname)
{
	for (size_t i=0; i<facts.size(); i++) {
		const Fact* f = facts[i];
		if (f->get_var()->name == vname) {
			Out() << Comment2Str(f->ToString()) + newline_;
		}
	}
}
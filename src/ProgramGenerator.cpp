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

#include "ProgramGenerator.h"
#include <cassert>
#include <sstream>
#include "RandomNumber.h"
#include "AbsRndNumGenerator.h"
#include "DefaultCOutputMgr.h" 
#include "Finalization.h"
#include "Function.h"
#include "Type.h" 
#include "CGOptions.h"
#include "SafeOpFlags.h"

ProgramGenerator* current_generator_ = NULL;

ProgramGenerator::ProgramGenerator(int argc, char *argv[], unsigned long seed)
	: argc_(argc),
	  argv_(argv),
	  seed_(seed),
	  output_mgr_(NULL)
{

}

/* Factory method */
ProgramGenerator* ProgramGenerator::CreateInstance(int argc, char *argv[], unsigned long seed)
{
	if (current_generator_ == NULL)
		current_generator_ = new ProgramGenerator(argc, argv, seed);
	return current_generator_;
}

ProgramGenerator::~ProgramGenerator()
{
	Finalization::doFinalization();
	delete output_mgr_;
}

void
ProgramGenerator::Init()
{ 
	RandomNumber::CreateInstance(rDefaultRndNumGenerator, seed_);
	  
	// TODO: build different output managers based on user input 
	// (either in a descriptive language or as a template)
	output_mgr_ = DefaultCOutputMgr::CreateInstance();
	 
	assert(output_mgr_);
	output_mgr_->Init();
} 

void
ProgramGenerator::GoGenerator()
{
	output_mgr_->OutputProgramHeader(argc_, argv_, seed_);

	GenerateAllTypes();
	GenerateFunctions();
 
	output_mgr_->OutputProgram(); 
}

ProgramGenerator*
ProgramGenerator::CurrentGenerator()
{
	return current_generator_;
}

AbsOutputMgr*
ProgramGenerator::CurrentOutputMgr()
{
	return CurrentGenerator()->GetOutputMgr();
}


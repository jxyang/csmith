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

/// Class Description
///
/// This class defines the outputting functions specific to C. The implementations
/// are largely copied from vaiours "Output" functions that are used to
/// disbursed throughout the Csmith classes. By moving them inside a
/// single class, we can more easily customize the outputting behaviors,
/// ideally, by extending this class (or its parent class CFamilyOutputMgr)
/// and printing the AST tree with the child class.  
///
#ifndef DEFAULT_C_OUTPUT_MGR_H
#define DEFAULT_C_OUTPUT_MGR_H

#include <vector>
#include <string>
#include <ostream>
#include <fstream>
#include "CFamilyOutputMgr.h"
#include "Variable.h"

using namespace std;

class StatementAssign;
class Fact; 
class FactPointTo; 

class DefaultCOutputMgr : public CFamilyOutputMgr {
public:
	static DefaultCOutputMgr *CreateInstance();

	DefaultCOutputMgr() {}
	virtual ~DefaultCOutputMgr() {};  

	virtual std::string Expression2Str(const Expression& e);  
	  
	virtual void OutputStmtAssertions(const Statement* stm, bool forFactsOut);

	virtual std::string Type2Str(const Type& t); 
	virtual std::string QualifiedType2Str(const Type& t, const TypeQualifiers* qfer);  
	virtual std::string FirstQualifiers2Str(const TypeQualifiers& qfer); 

	virtual void OutputProgram(); 
	 
	virtual void OutputMain(); 

	virtual void OutputUserDefinedType(const Type& t);

	virtual std::string FactPointsTo2Str(const FactPointTo& fp);
	virtual std::string ArrayInits2Str(const ArrayVariable& av, const vector<const Expression*>& inits);  

	std::string FunctionInvocation2Str(const FunctionInvocation* fi);   

protected:  

private:    
	void OutputPtrResets(const vector<const Variable*>& ptrs);   

	std::string BuildArrayInitRecursive(const ArrayVariable& av, size_t dimen, const vector<string>& init_strings);

	std::string VarValueDump2Str(const Variable& v, string dumpPrefix);

	void OutputForwardDeclarations(const vector<const Function*>& funcList);

	static DefaultCOutputMgr *instance_; 
};

#endif // DEFAULT_C_OUTPUT_MGR_H

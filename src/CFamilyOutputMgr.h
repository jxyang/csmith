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
/// This file defines the class of a generic output manager for family
/// of programming languages derived from C. For example, C++, Java, C#.
/// The purpose the this class is to implement many of the outputtings
/// common to all of them, such as a comment. There is still pure virtual
/// functions need to implemented in the output manager of a specific
/// langauge. See DefaultCOutputMgr for an example.
///

#ifndef C_FAMILY_OUTPUT_MGR_H
#define C_FAMILY_OUTPUT_MGR_H

#include <vector>
#include <string>
#include <ostream>
#include <fstream>
#include "AbsOutputMgr.h"
#include "Variable.h"

using namespace std;

class StatementAssign;
class Fact; 
class FactPointTo; 

class CFamilyOutputMgr : public AbsOutputMgr {
public: 
	CFamilyOutputMgr() {}
	virtual ~CFamilyOutputMgr() {};

	///
	/// Supporting functions
	///
	virtual std::string Comment2Str(const std::string &comment); 
	virtual std::string ScopeOpener2Str(void); 
	virtual std::string ScopeCloser2Str(void); 

	/// 
	/// Construct outputting functions
	///
	virtual void OutputBlock(const Block& b);  
	virtual void OutputFunction(const Function& f);   
	virtual void OutputStatement(const Statement& s);	 
	virtual void OutputStatements(const vector<Statement*> &stms);
	virtual void OutputUserDefinedTypes();
	virtual std::string FuncTitle2Str(const Function& f); 

	// Variable output functions
	virtual std::string VarRef2Str(const Variable& v);  
	virtual std::string VarDecl2Str(const Variable& v);
	virtual std::string VarDecls2Str(const vector <const Variable*> &vars);
	virtual std::string VarDef2Str(const Variable& v);
	virtual std::string VarDefs2Str(const vector<Variable*> &vars); 
	virtual std::string VarHash2Str(const Variable& v);	 

	// Language builtin operators
	virtual string AssignOperator2Str(eAssignOps op);  
	virtual string BinaryOperator2Str(eBinaryOps op);
	virtual string UnaryOperator2Str(eUnaryOps op); 

	/// 
	/// Pure virtual functions that must be implemented in a child class
	/// See AbsOutputMgr.h for the purpose of these functions
	/// See DefaultCOutputMgr.cpp for examplar implementation of them
	///
	virtual std::string Expression2Str(const Expression& e) = 0; 
	virtual void OutputStmtAssertions(const Statement* stm, bool forFactsOut) = 0;
	virtual std::string QualifiedType2Str(const Type& t, const TypeQualifiers* qfer) = 0; 
	virtual std::string FirstQualifiers2Str(const TypeQualifiers& qfer) = 0; 
	virtual std::string Type2Str(const Type& t) = 0; 
	virtual void OutputUserDefinedType(const Type& t) = 0; 
	virtual std::string FactPointsTo2Str(const FactPointTo& fp) = 0; 
	virtual std::string ArrayInits2Str(const ArrayVariable& av, const vector<const Expression*>& inits) = 0;
	virtual std::string FunctionInvocation2Str(const FunctionInvocation* fi) = 0;   
	virtual void OutputProgram() = 0;  
	virtual void OutputMain() = 0; 

protected:  
	 
	/// 
	/// TODO: make some of them virtual so a child class has a chance
	/// To overide the behavior for a specific langauge.
	///
	void OutputBlockTmpVariables(const Block& b);  
	void OutputPtrResets(const vector<const Variable*>& ptrs);  
	std::string SimpleAssign2Str(const StatementAssign& assign);  
	std::string Assign2Str(const StatementAssign& assign);     
	std::string ArrayLoopHead2Str(const ArrayVariable& av, ArrayVariable*& itemizedAV); 
	std::string ArrayLoopTail2Str(const ArrayVariable& av); 
	std::string ArrayInit2Str(const ArrayVariable& av, const Expression& init);   
	std::string VarValueDump2Str(const Variable& v, string dumpPrefix); 
	void OutputForwardDeclarations(const vector<const Function*>& funcList); 

private:
};

#endif // C_FAMILY_OUTPUT_MGR_H

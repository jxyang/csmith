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
/// This file defines the class of base output manager.
/// For each programming language, there should be one or more output
/// managers (directly or indirectly) derived from this class. If the 
/// language is close to C enough, we suggest to derive the output 
/// manager(s) from either CFamilyOutputMgr or DefaultCOutputMgr.
///
#ifndef ABS_OUTPUT_MGR_H
#define ABS_OUTPUT_MGR_H

#include <ostream> 
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
using namespace std;

#include "StatementAssign.h"
#include "FunctionInvocationBinary.h"
#include "FunctionInvocationUnary.h"
#include "Variable.h"

class Variable;
class Type;
class TypeQualifiers;
class Block;
class Constant;
class Expression;
class Fact;
class Function;
class Lhs;
class Statement;
class FactPointTo;

#define TAB "    "    // to beautify output: 1 tab is 4 spaces

class AbsOutputMgr {
public:
	AbsOutputMgr() {}; 
	virtual ~AbsOutputMgr(); 
	
	// Initialize output manager, such is creating output stream 
	virtual void Init();

	// Get a handle to the output stream. Can be either a file or std::out
	std::ostream& Out() { if (ofile_) return *ofile_; else return (std::cout); }

	/// 
	/// All the pure virtual output functions must be defined in subclasses
	///
	// Return strings of language-builtin operators
	virtual string AssignOperator2Str(eAssignOps op) = 0;  
	virtual string BinaryOperator2Str(eBinaryOps op) = 0;
	virtual string UnaryOperator2Str(eUnaryOps op) = 0; 
	
	///
	/// Supportting functions
	///
	// Return a comment line
	virtual std::string Comment2Str(const std::string &comment) = 0; 
	
	// Return a scope opener string indicating starting of a new block
	virtual std::string ScopeOpener2Str(void) = 0; 
	// Return a scope closer string indicating the end of a block
	virtual std::string ScopeCloser2Str(void) = 0;

	// Return the specified number of tabs as string (to properly tab the output) 
	static std::string Tab2Str(int indent) { string str; while (indent--) str += TAB; return str;}
	// return the current number of tabs as string (tab number is updated when entering/exiting blocks
	std::string Tab2Str() { return Tab2Str(indent_);}

	///
	/// Outputting language contructs
	///
	// Output the whole program, including main function, header files, the tail, and the AST
	virtual void OutputProgram() = 0; 
	
	// Output the entry point to the program, in C it is main function
	virtual void OutputMain() = 0; 

	// Output the statisticals of the random generation
	void OutputTail(); 

	// Output the random generation info including seed, Csmith version, etc. The header is 
	// outputted even before the random generation, so we can reproduce a hang with those info.
	virtual void OutputProgramHeader(int argc, char *argv[], unsigned long seed);

	// Output a block which is a container for sequential statements and local variables
	virtual void OutputBlock(const Block& b) = 0;     

	// Output a function definition
	virtual void OutputFunction(const Function& f) = 0;  

	// Output a statement
	virtual void OutputStatement(const Statement& s) = 0;	 
	
	// Return string representation of an expression
	virtual std::string Expression2Str(const Expression& e) = 0;	
	
	// Output an user defined type, such as structures
	virtual void OutputUserDefinedType(const Type& t) = 0;
	// Output all user defined types
	virtual void OutputUserDefinedTypes() = 0; 

	/// 
	/// Type output functions
	///
	// Output a type. For user-defined types, this is the reference string 
	virtual std::string Type2Str(const Type& t)= 0;  
	// Output first level qualifiers. This is useful for non-pointer types as they only have one level of qualification.
	virtual std::string FirstQualifiers2Str(const TypeQualifiers& qfer) = 0;
	
	// Output a type coupled with its qualifiers. We maintain both type and qualifier info in 
	// Variable class. For languages don't support type qualification, pass 0 for qfer, and
	// this function should behave the same as Type2Str.
	virtual std::string QualifiedType2Str(const Type& t, const TypeQualifiers* qfer=0) = 0; 
	
	// Output a variable type, assuming the variable knows its qualifiers 
	virtual std::string VarType2Str(const Variable& v) { return QualifiedType2Str(*v.type, &v.qfer);}

	///
	/// Variable output functions
	///
	// Output a variable reference
	virtual std::string VarRef2Str(const Variable& v) = 0;  
	// Output a variable declaration
	virtual std::string VarDecl2Str(const Variable& v) = 0;
	// Output a set of variable declarations
	virtual std::string VarDecls2Str(const vector <const Variable*> &vars) = 0;
	// Output a variable definition
	virtual std::string VarDef2Str(const Variable& v) = 0;
	// Output a set of variable definitions
	virtual std::string VarDefs2Str(const vector<Variable*> &vars) = 0;   
	// Output a variable hashing
	virtual std::string VarHash2Str(const Variable& v) = 0;  

	///
	/// GTAV related output functions 
	///
	// Output analysis results for a statement (eith flow-in facts or flow-out facts)
	virtual void OutputStmtAssertions(const Statement* stm, bool forFactsOut) = 0; 
	// Output a set of facts as comments
	void PrintFacts(const vector<const Fact*>& facts);
	// Output a set of facts related to a variable as comments
	void PrintVarFacts(const vector<const Fact*>& facts, const char* vname);
	// Return string representation of a PointsTo fact
	virtual std::string FactPointsTo2Str(const FactPointTo& fp) = 0;

protected:  
	// The number of tabs that should be outputted before a statement or block opener/closer.
	// It is updated when entering/exiting blocks
	int indent_; 
	
	// Default is "\n". Some output managers may want to change to " " to generate compact outputs
	std::string newline_; 

private: 
	// The output stream. If NULL, use std::out.
	std::ofstream *ofile_; 
};  

#endif // ABS_OUTPUT_MGR_H

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

#include "CFamilyOutputMgr.h"

#include <cassert>
#include <sstream> 
#include "Common.h"
#include "CGOptions.h"
#include "platform.h"
#include "Bookkeeper.h"
#include "Function.h"
#include "Block.h"
#include "FunctionInvocation.h"
#include "CGContext.h"
#include "ArrayVariable.h"
#include "VariableSelector.h"
#include "Type.h"
#include "random.h"  
#include "StatementReturn.h"
#include "StatementIf.h"
#include "StatementGoto.h"
#include "StatementFor.h"
#include "StatementAssign.h"
#include "StatementArrayOp.h"
#include "StatementCall.h"
#include "StatementContinue.h"
#include "StatementBreak.h"  
#include "SafeOpFlags.h"
#include "FactMgr.h"
#include "Constant.h"
#include "ExpressionAssign.h"
#include "ExpressionComma.h"
#include "ExpressionFuncall.h"
#include "ExpressionVariable.h"
#include "FactUnion.h"
#include "FunctionInvocationUser.h"
#include "FunctionInvocationBinary.h"
#include "FunctionInvocationUnary.h"

using namespace std;     

// Output a line of comments
std::string
CFamilyOutputMgr::Comment2Str(const std::string &comment)
{
	if (CGOptions::quiet() || CGOptions::concise()) {
		return "";
	}
	else {
		return "/* " + comment + " */"; 
	}
}

// Output a scope opener
// In C, this could be "{\newline_". Be mindful of the tab, which should increase with a new scope.
std::string
CFamilyOutputMgr::ScopeOpener2Str()
{
	return Tab2Str(indent_++) + "{" + newline_; 
}

// Output a scope closer
// In C, this could be "\newline_}". Be mindful of the tab, which should decrease with a scope closing.
// If noNewline is true, there is no new line before "}"
std::string
CFamilyOutputMgr::ScopeCloser2Str()
{ 
	return newline_ + Tab2Str(--indent_) + "}";
}   
 
string
CFamilyOutputMgr::AssignOperator2Str(eAssignOps op)  
{
	switch (op) {
	case eSimpleAssign: return "=";
	case eMulAssign:	return "*=";
	case eDivAssign:	return "/=";
	case eRemAssign:	return "%=";
	case eAddAssign:	return "+=";
	case eSubAssign:	return "-=";
	case eLShiftAssign:	return "<<=";
	case eRShiftAssign:	return ">>=";
	case eBitAndAssign:	return "&=";
	case eBitXorAssign:	return "^=";
	case eBitOrAssign:	return "|="; 
	case ePreIncr:		return "++";
	case ePreDecr:		return "--";
	case ePostIncr:		return "++";
	case ePostDecr:		return "--";
	}
	assert(0);
	return "";
} 

string
CFamilyOutputMgr::BinaryOperator2Str(eBinaryOps op)
{
	switch (op) {
	// arithmatic Ops
	case eAdd:		return "+";  
	case eSub:		return "-";  
	case eMul:		return "*"; 
	case eDiv:		return "/"; 
	case eMod:		return "%";  
		
	// Logical Ops
	case eAnd:		return "&&"; 
	case eOr:		return "||"; 
	case eCmpEq:	return "=="; 
	case eCmpNe:	return "!="; 
	case eCmpGt:	return ">";	 
	case eCmpLt:	return "<";	 
	case eCmpLe:	return "<="; 
	case eCmpGe:	return ">="; 
		
	// Bitwise Ops
	case eBitAnd:	return "&";	 
	case eBitOr:	return "|";	 
	case eBitXor:	return "^";	 
	case eLShift:	return "<<"; 
	case eRShift:	return ">>"; 
	}
	assert(0);
	return "";
}

std::string
CFamilyOutputMgr::UnaryOperator2Str(eUnaryOps op)
{
	switch (op) { 
	// unary Ops 
	case ePlus:		return "+"; 
	case eMinus:	return "-"; 
	case eNot:		return "!";  
	case eBitNot:	return "~"; 
	}
	assert(0);
	return "";
} 

void
CFamilyOutputMgr::OutputUserDefinedTypes()
{ 
    Out() << Comment2Str("--- User Defined Type Declarations ---") + newline_;
    for (size_t i=0; i<Type::GetAllTypes().size(); i++) { 
        Type* t = Type::GetAllTypes()[i];
        if (t->used && t->IsUserDefined()) {
            OutputUserDefinedType(*t);
        }
    }
}

// Output a block
void
CFamilyOutputMgr::OutputBlock(const Block& b) 
{ 
	std::ostream& out = Out();
	out << ScopeOpener2Str();  
	out << Comment2Str("block id: " + StringUtils::int2str(b.stm_id)) +  newline_;  
 
	if (b.stm_id == 191)
		int i = 0;

	// JY: Do we really need this?
	if (CGOptions::math_notmp())
		OutputBlockTmpVariables(b);

	out << VarDefs2Str(b.local_vars);
	OutputStatements(b.stms);
	
	out << ScopeCloser2Str();
}

void
CFamilyOutputMgr::OutputBlockTmpVariables(const Block& b)
{
	std::map<string, enum eSimpleType>::const_iterator i;
	for (i = b.macro_tmp_vars.begin(); i != b.macro_tmp_vars.end(); ++i) {
		std::string name = (*i).first;
		enum eSimpleType type = (*i).second;
		Out() << Tab2Str();
		Out() << Type2Str(Type::get_simple_type(type));
		Out() << " " << name << " = 0;" << newline_;
	}
} 

// Generate the loop header for an array operation. 
// The control variable name is pass backed to caller via "cvname"
std::string
CFamilyOutputMgr::ArrayLoopHead2Str(const ArrayVariable& av, ArrayVariable*& itemizedAV)
{
#if 0
	static int loopID = 0;
	// increment the loopID to avoid control variable name conflict with other array loops 
	string cvname = "cv" + StringUtils::int2str(loopID++) + "_";
#endif
	string str;
	vector<const Variable*>& ctrlVars = Variable::get_new_ctrl_vars(av.get_dimension());
	str += VarDecls2Str(ctrlVars);
	for (size_t i=0; i<av.get_dimension(); i++) { 
#if 0
		string tmpname = cvname + StringUtils::int2str(i);
		ctrlVars.push_back(Variable::CreateTmpVariable(tmpname, *get_int_type()));
#endif
		string tmpname = VarRef2Str(*ctrlVars[i]);
		str += Tab2Str() + "for (" + tmpname + " = 0; ";
		str += tmpname + " < " + StringUtils::int2str(av.get_sizes()[i]) + "; "; 
		str += tmpname + " = " + tmpname + " + 1)" + newline_;   
		// the scope opening "{"s
		str += ScopeOpener2Str();
	}   

	itemizedAV = av.itemize(ctrlVars, NULL);
	return str;
}

std::string
CFamilyOutputMgr::ArrayLoopTail2Str(const ArrayVariable& av)
{
	string str;
	// output the closing bracelets
	for (size_t i=0; i<av.get_dimension(); i++) {
		--indent_; 
		str += newline_ + Tab2Str() + "}";
#if 0		 
		// delete the control variables we created in ArrayLoopHead2Str
		if (av.get_indices().size() > i && av.get_indices()[i]->term_type == eVariable) {
			const ExpressionVariable* ev = (const ExpressionVariable*)(av.get_indices()[i]);
			delete ev->get_var();
		} 
#endif
	}
	str += newline_;
	return str;
}

// Output a loop to initialize all array members to init 
std::string
CFamilyOutputMgr::ArrayInit2Str(const ArrayVariable& av, const Expression& init) 
{  
	if (av.collective != NULL) return "";
	ArrayVariable* arrayItem;
	// output loop headers
	string str = ArrayLoopHead2Str(av, arrayItem); 

	// assign array variable (indexed with control variables) to initial value
	// If initial value is a constant aggregate, need to create a tmp to circumstance a compiler error.
	if (init.term_type == eConstant && av.IsAggregate()) {
		str += Tab2Str() + Type2Str(*av.type) + " tmp = " + Expression2Str(init) + ";" + newline_; 
		str += Tab2Str() + VarRef2Str(*arrayItem) + " = tmp;" + newline_;
	}
	else {
		str += Tab2Str() + VarRef2Str(*arrayItem) + " = " + Expression2Str(init) + ";" + newline_; 
	}
	str += ArrayLoopTail2Str(*arrayItem); 

	return str;
} 

std::string
CFamilyOutputMgr::VarRef2Str(const Variable& v) 
{ 
	string str = v.get_actual_name();
	if (v.isArray) {
		const ArrayVariable& av = (const ArrayVariable&)v;
		// for itemized array variables, output the modularized index
		if (av.collective != 0) {  
			assert(!av.get_indices().empty()); 
			for (size_t i=0; i<av.get_indices().size(); i++) { 
				str += "[" + Expression2Str(*av.get_indices()[i]) + "]"; 
			}
		}
	}

	if (v.is_volatile() && CGOptions::wrap_volatiles()) {
		str = "VOL_RVAL(" + str +  ", " + Type2Str(*v.type) + ")"; 
	} 
	else if (CGOptions::access_once() && v.isAccessOnce && !v.isAddrTaken) {
		assert(CGOptions::access_once() && "access_once is disabled!");
		str = "ACCESS_ONCE(" + str + ")";
	}

	return str;
}

std::string
CFamilyOutputMgr::VarDecl2Str(const Variable& v) 
{  
	string str = QualifiedType2Str(*v.type, &v.qfer) + v.get_actual_name(); 
	if (v.isArray) {
		const ArrayVariable& av = (const ArrayVariable&)v;
		// for non-itemized array variables, output the modularized index
		if (av.collective == 0) {  
			assert(!av.get_sizes().empty()); 
			for (size_t i=0; i<av.get_sizes().size(); i++) { 
				str += "[" + StringUtils::int2str(av.get_sizes()[i]) + "]"; 
			}
		}
	}
	return str;
} 

// Output declarations for a list of variables
std::string
CFamilyOutputMgr::VarDecls2Str(const vector <const Variable*> &vars)
{   
	string str;
	if (vars.size() == 0) return str;

	// Check if all variables in the list are of the same type.
	bool sameType = true;
	for (size_t i=1; i<vars.size(); i++) { 
		if (vars[i]->type != vars[0]->type) {
			sameType = false;
			break;
		}
	}

	if (!sameType)
	{ 
		// If not of the same type, print a decl line by line.
		for (size_t i=0; i<vars.size(); i++) {
			str += Tab2Str() + VarDecl2Str(*vars[i]) + newline_; 
		}
	}
	else
	{
		// otherwise, output the type only once to save space
		str += Tab2Str() + Type2Str(*vars[0]->type); 
		for (size_t i=0; i<vars.size(); i++) {  
			str += " " + vars[i]->get_actual_name(); 
			str += (i<vars.size() - 1) ? "," : ";"; 
		}
		str += newline_;
	} 
	return str;
}

std::string
CFamilyOutputMgr::VarDef2Str(const Variable& v)
{    
	string str;
	// force global variables to be static if necessary
	if (CGOptions::force_globals_static() && v.is_global()) {
		str += "static ";
	}

	str += VarDecl2Str(v);   
	
	if (v.init != NULL)
	{
		str += " = ";  
		if (!v.isArray) {
			str += Expression2Str(*v.init);
		}
		else {	// for array variable, construct the initializer differently
			const ArrayVariable& av = (const ArrayVariable&)(v); 
			str += ArrayInits2Str(av, av.get_init_values());
		} 
	} 
	return str;
}
	
std::string
CFamilyOutputMgr::VarDefs2Str(const vector<Variable*> &vars)
{ 
	string str;
	for (size_t i=0; i<vars.size(); i++) { 
		const Variable* v = vars[i];
		// filter out itemized array variables
		if (vars[i]->isArray && ((const ArrayVariable*)(vars[i]))->collective != NULL)
			continue;
		if (v->name == "g_70")
			i = i;
		str += Tab2Str() + VarDef2Str(*v) + ";";
		if (v->is_volatile()) {
			string comment = "VOLATILE GLOBAL " + v->get_actual_name();
			str += Comment2Str(comment);
		}  
		str += newline_;
	} 
	return str;
}    

std::string
CFamilyOutputMgr::VarHash2Str(const Variable& v) 
{  
	string str;	

	// for item collection in an array, generate a loop and hash the itemized array member.
	if (v.isArray && ((const ArrayVariable&)v).collective == NULL) {
		const ArrayVariable& av = (const ArrayVariable&)v;
		ArrayVariable* arrayItem = NULL;
		str += ArrayLoopHead2Str(av, arrayItem);
		string itemHash = VarHash2Str(*arrayItem);

		// If the item has nothing to hash, skip hashing the whole array 
		if (itemHash.empty()) return "";

		str += VarHash2Str(*arrayItem);
		str += ArrayLoopTail2Str(*arrayItem); 
		return str;
	}

	// for non-array variables or single items in arrays, generate a hash for each eligible scalar field

	if (v.type->IsAggregate()) { 
		FactMgr* fm = get_fact_mgr_for_func(GetFirstFunction()); 
		for (size_t i=0; i<v.field_vars.size(); i++) {
			if (v.type->eType == eUnion && !FactUnion::is_field_readable(&v, i, fm->global_facts)) { 
				// don't read union fields that is not last written into or have possible padding bits 
				continue;
			}
			str += VarHash2Str(*v.field_vars[i]); 
		}
    } 
	else if (v.type->eType == eSimple) {
		str += ((CGOptions::compute_hash()) ?
			Tab2Str() + "transparent_crc(" + VarRef2Str(v) + ", \"" + v.name + "\", print_hash_value);" + newline_ :
			Tab2Str() + Variable::sink_var_name + " = " + VarRef2Str(v) + ";" + newline_);  
    }
	return str;
}  

void
CFamilyOutputMgr::OutputStatement(const Statement& s)
{
	std::ostream& out = Out();
	out << Tab2Str();

	switch (s.get_type())
	{
		case eAssign:  
			out << Assign2Str((const StatementAssign&)s) + ";";
			break;
		case eReturn: 	
			out << "return " + Expression2Str(*((const StatementReturn&)s).get_var()) + ";"; 
			break;
		case eIfElse: {
			const StatementIf& si = (const StatementIf&)s;
			out << "if (" + Expression2Str(*si.get_test()) + ")" + newline_;  
			OutputBlock(*si.get_true_branch());  
			out << Tab2Str() + "else" + newline_; 
			OutputBlock(*si.get_false_branch());
			break;
		}
		case eFor: {
			const StatementFor& sf = (const StatementFor&)s; 
			out << "for (" + Assign2Str(*sf.get_init()) + "; " + 
				Expression2Str(*sf.get_test()) + "; " + Assign2Str(*sf.get_incr()) + ")" + newline_; 
			OutputBlock(*sf.get_body()); 
			break;
		}
		case eArrayOp: {
			const StatementArrayOp& sa = (const StatementArrayOp&)s;
			// output loop header
			for (size_t i=0; i<sa.array_var->get_dimension(); i++) {
				if (i > 0) 
					out << ScopeOpener2Str();  
				out << Tab2Str();
				out << "for (" + VarRef2Str(*sa.ctrl_vars[i]);
				out << " = " << sa.inits[i] << "; ";
				out << VarRef2Str(*sa.ctrl_vars[i]);
				(sa.incrs[i] > 0) ? out << " < " << sa.array_var->get_sizes()[i] : out << " >= 0";
				out << "; "; 
				out << VarRef2Str(*sa.ctrl_vars[i]) + " += " + StringUtils::int2str(sa.incrs[i]) + ")" + newline_;  
			}
			ArrayVariable* arrayItem = sa.array_var->itemize(sa.ctrl_vars, 0);
			// output loop body	
			if (sa.body != NULL)
				OutputBlock(*sa.body);
			// Or output an random loop initializer if there is no generated body.
			else if (sa.init_value != NULL) { 
				out << ScopeOpener2Str();  
				// cannot assign array members to a struct/union constant directly, has to create a "fake" struct var first
				if (sa.init_value->term_type == eConstant && sa.array_var->IsAggregate()) {
					out << Tab2Str() + Type2Str(*sa.array_var->type) + " tmp = " + Expression2Str(*sa.init_value) + ";" + newline_; 
					out << Tab2Str() + VarRef2Str(*arrayItem) + " = tmp;" + newline_;
				} else {
				out << Tab2Str() + VarRef2Str(*arrayItem) + " = " + Expression2Str(*sa.init_value) + ";" + newline_; 
				}
				out << ScopeCloser2Str(); 
			}
	
			// output the closing bracelets
			for (size_t j=1; j<sa.array_var->get_dimension(); j++) {
				out << ScopeCloser2Str();
			} 
			break;
		} 
		case eInvoke:  
			out << Expression2Str(*((const StatementCall&)s).get_call()) + ";"; 
			break;
		case eGoto: {
			const StatementGoto& sg = (const StatementGoto&)s; 
			out << "if (" + Expression2Str(sg.test) + ")" + newline_ + Tab2Str(indent_+1); 
			out << "goto " << sg.label << ";";  
			break;
		}
		case eContinue: { 
			const StatementContinue& sc = (const StatementContinue&)s; 
			out << "if (" + Expression2Str(sc.test) + ")" + newline_ + Tab2Str(indent_+1); 
			out << "continue;";  
		}
		case eBreak: { 
			const StatementBreak& sb = (const StatementBreak&)s; 
			out << "if (" + Expression2Str(sb.test) + ")" + newline_ + Tab2Str(indent_+1); 
			out << "break;";  
		}
			 
	}
	out << newline_;		
}

void
CFamilyOutputMgr::OutputStatements(const vector<Statement*> &stms)
{ 
	std::ostream& out = Out();
	for (size_t i=0; i<stms.size(); i++) {
		const Statement* stm = stms[i];
			
		// output label if this is a goto target
		vector<const StatementGoto*> gotos;
		if (stm->find_jump_sources(gotos)) {
			assert(gotos.size() > 0);
			out << Tab2Str();
			out << gotos[0]->label << ":" << endl;   
		}
 
		OutputStatement(*stm);

		// output assertions for analysis results if needed
		if (CGOptions::paranoid() && !CGOptions::concise()) {
			FactMgr* fm = get_fact_mgr_for_func(stm->func);
			//fm->output_assertions(out, this, indent, true);
		} 
	}
}   

// Output an assignment (including effective assignment such as ++/-- etc)
// with NO consideration of of signed interger overflows 
std::string
CFamilyOutputMgr::SimpleAssign2Str(const StatementAssign& assign)  
{ 
	string str;
	eAssignOps op = assign.GetOp();
	switch (op) {
	default:
		str += Expression2Str(*assign.get_lhs()) + " " + AssignOperator2Str(op) + " " + Expression2Str(*assign.get_expr());  
		break; 
	case ePreIncr:
	case ePreDecr:
		str += AssignOperator2Str(op) + Expression2Str(*assign.get_lhs());  
		break; 
	case ePostIncr:
	case ePostDecr:
		str += Expression2Str(*assign.get_lhs()) + AssignOperator2Str(op);
		break;
	}
	return str;
} 

// Output an assignment (including effective assignment such as ++/-- etc)
// with avoidance consideration of of signed interger overflows 
std::string
CFamilyOutputMgr::Assign2Str(const StatementAssign& assign) 
{
	string str;
	eAssignOps op = assign.GetOp();
	// avoid signed int overflow for += and -=
	if (CGOptions::avoid_signed_overflow() && assign.op_flags && (op == eAddAssign || op == eSubAssign) ) { 
		enum eBinaryOps bop = StatementAssign::compound_to_binary_ops(op);  
		string fname = assign.op_flags->to_string(bop);
		int id = SafeOpFlags::to_id(fname); 
		
		// don't use safe math wrapper if this function is specified in "--safe-math-wrapper"
		if (!CGOptions::safe_math_wrapper(id)) {
			return SimpleAssign2Str(assign); 
		}

		str += Expression2Str(*assign.get_lhs()) + " = " + fname + "(";
		if (CGOptions::math_notmp()) {
			str += assign.tmp_var1 + ", ";
		}

		str += Expression2Str(*assign.get_lhs()) + ", "; 
		if (CGOptions::math_notmp()) {
			str += assign.tmp_var2 + ", ";
		} 
			
		str += Expression2Str(*assign.get_expr());
		if (CGOptions::identify_wrappers()) {
			str += ", " + id;
		}
		str += ")"; 
		return str;
	}
	// Simply output the assignment without safe math wrapper 
	else {
		return SimpleAssign2Str(assign);
	}
}  
 
// Output a function title
std::string
CFamilyOutputMgr::FuncTitle2Str(const Function& f)
{ 
	string str;
	if (f.is_inlined)
		str += "inline ";
	// force functions to be static if necessary
	if (CGOptions::force_globals_static()) {
		str += "static ";
	}

	str += VarType2Str(*f.rv) + " " + f.name + "(";

	// output parameters
	if (f.param.size() == 0) {
		str += Type2Str(*Type::void_type); 
	} else {
		for (size_t i=0; i<f.param.size(); i++) {  
			if (i > 0) str += ", "; 
			const Variable* var = f.param[i];
			str += VarType2Str(*var) + " " + var->name;
		}
	} 
	str += ")";
	return str;
}

void
CFamilyOutputMgr::OutputFunction(const Function& f)
{ 
	Out() << Comment2Str("------------------------------------------") + newline_;
	if (!CGOptions::concise()) {
		Out() << Comment2Str(f.feffect.ToString()) + newline_;
	} 
	Out() << FuncTitle2Str(f) + newline_;  

	OutputBlock(*f.body);  

	Out() << newline_ + newline_; 
}

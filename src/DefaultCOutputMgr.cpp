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

#include "DefaultCOutputMgr.h"

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
#include "FactPointTo.h"
#include "FunctionInvocationUser.h"
#include "FunctionInvocationBinary.h"
#include "FunctionInvocationUnary.h"

using namespace std;

DefaultCOutputMgr *DefaultCOutputMgr::instance_ = NULL;

// Make sure this is a singleton
DefaultCOutputMgr *
DefaultCOutputMgr::CreateInstance()
{
	if (DefaultCOutputMgr::instance_)
		return DefaultCOutputMgr::instance_; 

	DefaultCOutputMgr::instance_ = new DefaultCOutputMgr(); 

	assert(DefaultCOutputMgr::instance_);
	DefaultCOutputMgr::instance_->Init();
	return DefaultCOutputMgr::instance_;
}  

void 
DefaultCOutputMgr::OutputUserDefinedType(const Type& t)
{ 
	static vector<const Type*> printedTypes;

    // sanity check
    assert (t.IsUserDefined());

	std::ostream& out = Out();
	if (std::find(printedTypes.begin(), printedTypes.end(), &t) == printedTypes.end()) { 
        // output dependent structs, if any
        for (size_t i=0; i<t.fields.size(); i++) { 
			if (t.fields[i]->IsUserDefined()) {
                OutputUserDefinedType(*(t.fields[i]));
            }
        } 
        // output myself
        if (t.packed_) { 
            out << "#pragma pack(push)" + newline_; 
            out << "#pragma pack(1)" + newline_; 
        }
        
		out << Type2Str(t);
        out << " {" + newline_; 

		assert(t.fields.size() == t.qfers_.size());  
		size_t j = 0;
        for (size_t i=0; i<t.fields.size(); i++) {
            out << "   ";
			const Type *field = t.fields[i];
			bool is_bitfield = t.is_bitfield(i);

            if (is_bitfield) {
				assert(field->eType == eSimple);
				out << FirstQualifiers2Str(t.qfers_[i]);
				if (field->simple_type == eInt)
					out << "signed";
				else if (field->simple_type == eUInt)
					out << "unsigned";
				else
					assert(0);
				int length = t.bitfields_length_[i];
				assert(length >= 0);
				if (length == 0)
					out << " : ";
				else
					out << " f" << j++ << " : ";
				out << length << ";";
			}
			else {
				out << QualifiedType2Str(*field, &t.qfers_[i]);
				out << " f" << j++ << ";";
			}
			out << newline_;
        }
        out << "};" + newline_; 

        if (t.packed_) { 
			out << "#pragma pack(pop)" + newline_;  
        } 
		out << newline_;

		printedTypes.push_back(&t);
    }
}

void
DefaultCOutputMgr::OutputProgram()
{
	std::ostream &out = Out(); 
		
	// define wrapper function number
	if (CGOptions::identify_wrappers()) { 
		out << "#define N_WRAP " << SafeOpFlags::wrapper_names.size() << std::endl; 
	}

	OutputUserDefinedTypes();

	out << Comment2Str("--- GLOBAL VARIABLES ---") + newline_;
	out << VarDefs2Str(*(VariableSelector::GetGlobalVariables()));
	 
	vector<const Function*> funcList = Function::GetRandomFunctions();
	OutputForwardDeclarations(funcList);

	out << newline_ + newline_; 
	out << Comment2Str("--- FUNCTIONS ---") + newline_;
	for (size_t i=0; i<funcList.size(); i++) {
		OutputFunction(*funcList[i]); 
	}
		
	if (!CGOptions::nomain())
		OutputMain();
	OutputTail(); 
}   

// Output a type. For user defined types, this is just a delcaration
std::string
DefaultCOutputMgr::Type2Str(const Type& t)
{  
	string str;
	switch (t.eType) {
	case eSimple:
		if (t.simple_type == eVoid) {
			str += "void";
		} else {
			str += (t.is_signed() ? "int" : "uint");
			str += StringUtils::int2str(t.SizeInBytes() * 8);
			str += "_t";
		} 
		break;
    case ePointer:   str += Type2Str(*t.ptr_type) + "*"; break;
    case eUnion:     str += "union U" + StringUtils::int2str(t.sid); break;
    case eStruct:    str += "struct S" + StringUtils::int2str(t.sid); break;
	}
	return str;
}

// Output a type along with its qualifiers, if there are any
std::string
DefaultCOutputMgr::QualifiedType2Str(const Type& t, const TypeQualifiers* qfer)
{ 
	if (qfer == NULL)
		return Type2Str(t);

	std::string str;
	assert(qfer->SanityCheck(&t)); 
	const Type* base = t.get_base_type();   
	const vector<bool>& is_consts = qfer->get_consts();
	const vector<bool>& is_volatiles = qfer->get_volatiles();
	
	for (size_t i=0; i<is_consts.size(); i++) {
		if (i>0) {
			str += "*";
		}
		if (is_consts[i]) {
			if (!CGOptions::consts())
				assert(0);
			if (i > 0) str += " ";
			str += "const ";
		}
		if (is_volatiles[i]) {
			if (!CGOptions::volatiles())
				assert(0);
			if (i > 0) str += " ";
			str += "volatile ";
		}
		if (i==0) {
			str += Type2Str(*base) + " ";  
		}
	} 
	return str;
}     

std::string
DefaultCOutputMgr::Expression2Str(const Expression& e)
{ 
	string str;
	// output type case if there is one
	if (e.cast_type != NULL) { 
		str += "(" + Type2Str(*e.cast_type) + ")"; 
	}

	switch (e.term_type)
	{
		case eConstant: {
			const Constant& ec = (const Constant&)e;
			//enclose negative numbers in parenthesis to avoid syntax errors such as "--8"
			if (!ec.get_value().empty() && ec.get_value()[0] == '-') {
				str += "(" + ec.get_value() + ")";
			} 
			else if (ec.get_type().eType == ePointer && ec.equals(0)){ 
				str += "NULL";
			}  
			else {
				str += ec.get_value();
			}
			break;
		}
		case eVariable: {   
			const ExpressionVariable& ev = (const ExpressionVariable&)e;
			int indirect_level = ev.get_indirect_level(); 
			if (indirect_level > 0) {
				str += "(";
				for (int i=0; i<indirect_level; i++) 
					str += "*"; 
			} else if (indirect_level < 0) {
				assert(indirect_level == -1);
				str += "&";
			}
			str += VarRef2Str(*ev.get_var());
			if (indirect_level > 0)
				str += ")";
			break;
		}   
		case eLhs: {
			const Lhs& lhs = (const Lhs&)e; 
			ExpressionVariable ev(*(lhs.get_var()), &lhs.get_type());
			str = Expression2Str(ev);
			if (lhs.get_var()->is_volatile() && CGOptions::wrap_volatiles())  
				 str = "VOL_LVAL(" + str + ", " + Type2Str(lhs.get_type()) + ")";
			break;
		}
		case eFunction: {
			str += FunctionInvocation2Str(((const ExpressionFuncall&)e).get_invoke()); 
			break;
		}
		case eCommaExpr: {
			const ExpressionComma& ec = (const ExpressionComma&)e; 
			str += "(" + Expression2Str(*ec.get_lhs()) + ", " + Expression2Str(*ec.get_rhs()) + ")"; 
			break;
		}
		case eAssignment: 
			str += "(" + Assign2Str(*((const ExpressionAssign&)e).get_stm_assign()) + ")";  
			break; 
	}
	return str;
}  

void
DefaultCOutputMgr::OutputMain()
{
	ostream& out = Out();
	CGContext cg_context(GetFirstFunction() /* BOGUS -- not in first func. */,
						 Effect::get_empty_effect(),
						 0);
	
	FunctionInvocation *invoke = NULL;
	invoke = FunctionInvocation::make_random(GetFirstFunction(), cg_context);
	out << endl << endl;
	out << Comment2Str("----------------------------------------") + newline_;

	out << (CGOptions::accept_argc() ? "int main (int argc, char* argv[])" :
		"int main (void)");
	out << newline_ + ScopeOpener2Str(); 

	// output initializers for global array variables
	vector<Variable *>& globalVars = *VariableSelector::GetGlobalVariables();
	//for (size_t i=0; i<globalVars.size(); i++)
	//	if (globalVars[i]->isArray) {
	//		ArrayVariable* av = (ArrayVariable*)(globalVars[i]);
	//		if (av->collective == NULL)
	//			out << ArrayInit2Str(*av, *av->init);
	//	}

	if (CGOptions::blind_check_global()) {
		out << "    ";
        out << FunctionInvocation2Str(invoke);
        out << ";" << endl;
		 
		for (size_t i=0; i<globalVars.size(); i++) {
			VarValueDump2Str(*globalVars[i], "checksum ");
		}
	}
	else {
		// set up a global variable that controls if we print the hash value after computing it for each global
		out << "    int print_hash_value = 0;" << endl;
		if (CGOptions::accept_argc()) {
			out << "    if (argc == 2 && strcmp(argv[1], \"1\") == 0) print_hash_value = 1;" << endl;
		}

		out << "    platform_main_begin();" << endl;
		if (CGOptions::compute_hash()) {
			out << "    crc32_gentab();" << endl;
		}

		out << "    ";
        out << FunctionInvocation2Str(invoke);
        out << ";" << endl; 

		// resetting all global dangling pointer to null per Rohit's request
		if (!CGOptions::dangling_global_ptrs()) {
			OutputPtrResets(GetFirstFunction()->dead_globals);
		}

		for (size_t i=0; i<globalVars.size(); i++) { 
			out << VarHash2Str(*globalVars[i]); 
		}

		if (CGOptions::compute_hash()) {
			out << "    platform_main_end(crc32_context ^ 0xFFFFFFFFUL, print_hash_value);" << endl;
		} else {
			out << "    platform_main_end(0,0);" << endl;
		}
	}
	out << "    return 0;" << endl;
	out << "}" << endl;
	delete invoke;
}   

void
DefaultCOutputMgr::OutputForwardDeclarations(const vector<const Function*>& funcList)
{
	Out() << newline_ + newline_; 
	Out() << Comment2Str("--- FORWARD DECLARATIONS ---") + newline_;
	 
	for (size_t i=0; i<funcList.size(); i++) { 
		const Function* f = funcList[i];  
		Out() << FuncTitle2Str(*f) + ";" + newline_; 
	}
} 

// Output assertions for either input or output fact set of a statement
void
DefaultCOutputMgr::OutputStmtAssertions(const Statement* stm, bool forFactsOut)
{
	ostream& out = Out();
	vector<Fact*> facts;  
	FactMgr* fm = get_fact_mgr_for_func(stm->func);

	if (!forFactsOut) {
		facts = fm->map_facts_in_final[stm];
	} else {
		fm->find_updated_final_facts(stm, facts);
	}
	if (facts.empty()) return;

	if (stm->eType == eFor || stm->eType == eIfElse) {
		out << Tab2Str();
		string ss = "facts after " + string(stm->eType == eFor ? "for loop" : "branching");
		out << Comment2Str(ss) + newline_; 
	}
	if (stm->eType == eAssign || stm->eType == eInvoke || stm->eType == eReturn) {
		out << Tab2Str();
		std::ostringstream ss; 
		ss << "statement id: " << stm->stm_id;
		out << Comment2Str(ss.str()) + newline_;
	}
	for (size_t i=0; i<facts.size(); i++) {
		const Fact* f = facts[i];
		const Effect& eff = stm->func->feffect;
		const Variable* v = f->get_var();
		assert(v);
		// don't print facts regarding global variables that are neither read or written in this function
		if (v->is_global() && !eff.is_read(v) && !eff.is_written(v)) {
			continue;
		}
		out << Tab2Str();

		string factStr = "assert (" + f->ToString() + ")";
		if (f->is_assertable(stm)) 
			out << factStr + newline_;
		else
			out << Comment2Str(factStr) + newline_; 
	}
}  

// resetting pointers to null by outputing "p = 0;" to facilitate testing on certain platforms
void 
DefaultCOutputMgr::OutputPtrResets(const vector<const Variable*>& ptrs)
{ 
	for (size_t i=0; i<ptrs.size(); i++) {
		const Variable* v = ptrs[i]; 
		if (v->isArray) {
			const ArrayVariable* av = (const ArrayVariable*)v;
			Constant* zero = Constant::make_int(0);  
			Out() << ArrayInit2Str(*av, *zero); 
		}
		else { 
			Out() << Tab2Str(1) + VarRef2Str(*v) + " = 0;" + newline_; 
		}
	}
}   

std::string
DefaultCOutputMgr::VarValueDump2Str(const Variable& v, string dumpPrefix) 
{
	string str;
	// for item collection in an array, generate a loop and hash the itemized array member.
	if (v.isArray && ((const ArrayVariable&)v).collective == NULL) {
		const ArrayVariable& av = (const ArrayVariable&)v;
		ArrayVariable* arrayItem = NULL;
		str += ArrayLoopHead2Str(av, arrayItem);
		str += VarValueDump2Str(*arrayItem, dumpPrefix);
		str += ArrayLoopTail2Str(*arrayItem);
		delete arrayItem;
		return str;
	}
		  
	if (v.type->eType == eSimple) {
		str += Tab2Str() + "printf(\"" + dumpPrefix + VarRef2Str(v) + " = " + v.type->printf_directive() + "\", " + VarRef2Str(v) + ");" + newline_;   
		
		// For array item, we need to dump to control variables.
		if (v.isArray) {
			const ArrayVariable& av = (const ArrayVariable&)v;
			assert(av.collective != NULL && av.get_indices().size() > 0);
			string cvLine = Tab2Str() + "printf(\"";
			for (size_t i=0; i<av.get_indices().size(); i++) {
				string cvname = Expression2Str(*(av.get_indices()[i]));
				cvLine += cvname + "=%d, ";
			}
			cvLine += "\", ";
			for (size_t i=0; i<av.get_indices().size(); i++) {
				string cvname = Expression2Str(*(av.get_indices()[i]));
				if (i > 0) cvLine += ", ";
				cvLine += cvname;
			}
			cvLine += ");" + newline_;
			str += cvLine;
		}
	}
	else if (v.type->IsAggregate()) {
		const vector<const Fact*>& facts = FactMgr::get_program_end_facts();
		for (size_t i=0; i<v.field_vars.size(); i++) {
			if (v.type->eType == eUnion && FactUnion::is_field_readable(&v, i, facts)) 
				continue; 
			str += VarValueDump2Str(*v.field_vars[i], dumpPrefix);
		}
	} 
	return str;
}

// print the initializer recursively for multi-dimension arrays 
string 
DefaultCOutputMgr::BuildArrayInitRecursive(const ArrayVariable& av, size_t dimen, const vector<string>& init_strings) 
{
	assert (dimen < av.get_dimension());
	static unsigned seed = 0xABCDEF; 
	string ret = "{";
	for (size_t i=0; i<av.get_sizes()[dimen]; i++) {
		if (dimen == av.get_sizes().size() - 1) {
			// use magic number to choose an initial value 
			size_t rnd_index = ((seed * seed + (i+7) * (i+13)) * 52369) % (init_strings.size()); 
			ret += init_strings[rnd_index];
			seed++;
		 } else {
			ret += BuildArrayInitRecursive(av, dimen + 1, init_strings);
		 }
		 if (i != av.get_sizes()[dimen] - 1) ret += ",";
	}
	ret += "}";
	return ret;
}

// build the string initializer in form of "{...}"
std::string
DefaultCOutputMgr::ArrayInits2Str(const ArrayVariable& av, const vector<const Expression*>& inits) 
{   
	vector<string> init_strings;
	if (av.init->get_type().IsAggregate())
		init_strings.clear();
	if (av.init != NULL)
		init_strings.push_back(Expression2Str(*av.init));
	for (size_t i=0; i<inits.size(); i++) {
		assert(inits[i]);
		init_strings.push_back(Expression2Str(*inits[i]));
	} 
	return BuildArrayInitRecursive(av, 0, init_strings); 
}  

std::string
DefaultCOutputMgr::FunctionInvocation2Str(const FunctionInvocation* fi)
{
	assert(fi);
	string str;
	if (fi->invoke_type == eFuncCall) {
		const FunctionInvocationUser* fiu = (const FunctionInvocationUser*)fi;
		str += fiu->get_func()->name + "(";
		for (size_t i=0; i<fiu->param_value.size(); i++) {
			if (i > 0) str += ", ";
			str += Expression2Str(*fiu->param_value[i]); 
		}
		str += ")";
	} 
	else if (fi->invoke_type == eBinaryPrim) {
		const FunctionInvocationBinary* fib = (const FunctionInvocationBinary*)fi;  
		str += "(";

		// cases where a wrapper is justified
		eBinaryOps op = fib->get_operation();
		if ((op == eAdd || op == eSub || op == eMul || op == eMod || op == eDiv || op == eLShift || op == eRShift) &&
			CGOptions::avoid_signed_overflow() &&
			fib->GetOpFlags() != NULL &&
			// don't use safe math wrapper if this function is not specified in "--safe-math-wrapper"
			CGOptions::safe_math_wrapper(fib->GetOpFlags()->GetWrapperID(op))) {   
			string fname = fib->GetOpFlags()->to_string(op); 
			int id = SafeOpFlags::to_id(fname);
						 
			str += fname + "(" + (CGOptions::math_notmp() ? (fib->get_tmp_var1() + ", ") : "");  
			str += Expression2Str(*fib->param_value[0]) + ", ";
			str += CGOptions::math_notmp() ? (fib->get_tmp_var2() + ", ") : ""; 
			str += Expression2Str(*fib->param_value[1]);

			if (CGOptions::identify_wrappers()) {
				str += ", " + id;
			}
		} 
		else {
			str += Expression2Str(*fib->param_value[0]) + BinaryOperator2Str(op) + Expression2Str(*fib->param_value[1]);
		}
		str += ")";	
	}  
	else if (fi->invoke_type == eUnaryPrim) { 
		const FunctionInvocationUnary* fiu = (const FunctionInvocationUnary*)fi; 
		str += "(";
		eUnaryOps op = fiu->get_operation();
		if (op == eMinus && 
			CGOptions::avoid_signed_overflow() && 
			fiu->GetOpFlags() != NULL &&
			// don't use safe math wrapper if this function is specified in "--safe-math-wrapper"
			CGOptions::safe_math_wrapper(fiu->GetOpFlags()->GetWrapperID(op))) {
			string fname = fiu->GetOpFlags()->to_string(op);
			int id = SafeOpFlags::to_id(fname); 

			str += fname + "(" + (CGOptions::math_notmp() ? (fiu->get_tmp_var() + ", ") : ""); 
			str += Expression2Str(*fiu->param_value[0]);
			if (CGOptions::identify_wrappers()) {
				str += ", " + id;
			}
		}
		else {
			str += UnaryOperator2Str(op) + Expression2Str(*fiu->param_value[0]);
		}
		str += ")";  
	}
	return str;
}   
 
std::string
DefaultCOutputMgr::FactPointsTo2Str(const FactPointTo& fp)  
{ 
	string str;
	string vname = VarRef2Str(*fp.get_var());
	for (size_t i=0; i<fp.get_point_to_vars().size(); i++) { 
		if (i > 0) str += " || "; 

		const Variable* pointee = fp.get_point_to_vars()[i]; 
		if (pointee->isArray || pointee->is_array_field()) {
			const ArrayVariable* av = (const ArrayVariable*)pointee;
			str += "(" + vname + " >= &" + pointee->get_actual_name();
			for (size_t i=0; i<av->get_dimension(); i++) {
				str += "[0]";
			} 
			str += " && " + vname + " <= &" + pointee->get_actual_name();
			for (size_t i=0; i<av->get_dimension(); i++) {
				str += "[" + StringUtils::int2str(av->get_sizes()[i] - 1) + "]";
			} 
			str +=  ")"; 
		}
		else {
			str += vname + " == "; 
			if (pointee == FactPointTo::garbage_ptr) str += "dangling"; 
			else if (pointee == FactPointTo::tbd_ptr) str += "tbd";
			else if (pointee == FactPointTo::null_ptr) str += "0";
			else str += "&" + VarRef2Str(*pointee);
		}
    }
	return str;
}

std::string
DefaultCOutputMgr::FirstQualifiers2Str(const TypeQualifiers& qfer)
{
	string str;
	assert(qfer.get_consts().size() == qfer.get_volatiles().size()); 
	if (qfer.get_consts().size() > 0 && qfer.get_consts()[0]) { 
		str += "const ";
	}

	if (qfer.get_volatiles().size() > 0 && qfer.get_volatiles()[0]) { 
		str += "volatile ";
	}
	return str;
} 
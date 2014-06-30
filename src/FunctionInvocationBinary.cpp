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

#include "FunctionInvocationBinary.h"
#include <cassert>

#include "Common.h"
#include "AbsOutputMgr.h"
#include "CGOptions.h"
#include "Expression.h" 
#include "FactMgr.h"
#include "Type.h"
#include "SafeOpFlags.h"
#include "CGContext.h"
#include "Block.h"
#include "random.h"

using namespace std; 
///////////////////////////////////////////////////////////////////////////////

FunctionInvocationBinary *
FunctionInvocationBinary::CreateFunctionInvocationBinary(CGContext &cg_context, 
						eBinaryOps op,
						SafeOpFlags *flags)
{
	FunctionInvocationBinary *fi = NULL;

	if (flags && FunctionInvocationBinary::safe_ops(op)) {
		bool op1 = flags->get_op1_sign();
		bool op2 = flags->get_op2_sign();
		enum SafeOpSize size = flags->get_op_size();

		eSimpleType type1 = SafeOpFlags::flags_to_type(op1, size);
		eSimpleType type2 = SafeOpFlags::flags_to_type(op2, size);

		const Block *blk = cg_context.get_current_block();
		assert(blk);

		std::string tmp_var1 = blk->create_new_tmp_var(type1);
		std::string tmp_var2;
		if (op == eLShift || op == eRShift) 
			tmp_var2 = blk->create_new_tmp_var(type2);
		else 
			tmp_var2 = blk->create_new_tmp_var(type1);

		fi = new FunctionInvocationBinary(op, flags, tmp_var1, tmp_var2);
	}
	else {
		fi = new FunctionInvocationBinary(op, flags);
	}
	return fi;
}


/*
 * XXX: replace with a useful constructor.
 */
FunctionInvocationBinary::FunctionInvocationBinary(eBinaryOps op, const SafeOpFlags *flags)
	: FunctionInvocation(eBinaryPrim, flags),
	  eFunc(op),
	  tmp_var1(""),
	  tmp_var2("")
{
	// Nothing else to do.  Caller must build useful params.
}

FunctionInvocationBinary::FunctionInvocationBinary(eBinaryOps op, const SafeOpFlags *flags,
						std::string &name1, std::string &name2)
	: FunctionInvocation(eBinaryPrim, flags),
	  eFunc(op),
	  tmp_var1(name1),
	  tmp_var2(name2)
{
	// Nothing else to do.  Caller must build useful params.
}

/*
 * copy constructor
 */
FunctionInvocationBinary::FunctionInvocationBinary(const FunctionInvocationBinary &fbinary)
	: FunctionInvocation(fbinary),
	  eFunc(fbinary.eFunc),
	  tmp_var1(fbinary.tmp_var1),
	  tmp_var2(fbinary.tmp_var2)
{
	// Nothing to do
}

FunctionInvocationBinary::FunctionInvocationBinary(eBinaryOps op , const Expression* exp1, const Expression* exp2, const SafeOpFlags *flags)
	: FunctionInvocation(eBinaryPrim, flags),
	  eFunc(op)
{
	param_value.clear();
	add_operand(exp1);
	add_operand(exp2);
}

/*
 *
 */
FunctionInvocationBinary::~FunctionInvocationBinary(void)
{
	// Nothing to do.
}

/*
 *
 */
FunctionInvocation *
FunctionInvocationBinary::clone() const
{
	return new FunctionInvocationBinary(*this);
}
///////////////////////////////////////////////////////////////////////////////

bool
FunctionInvocationBinary::safe_ops(eBinaryOps op)
{
	switch(op) {
	case eAdd:
	case eSub:
	case eMul:
	case eMod:
	case eDiv:
	case eLShift:
	case eRShift:
		return true;
	default:
		return false;
	}
}

/* do some constant folding */
bool 
FunctionInvocationBinary::equals(int num) const 
{
	assert(param_value.size() == 2);
	if (num == 0) {
		if (param_value[0]->equals(0) && 
			(eFunc==eMul || eFunc==eDiv || eFunc==eMod || eFunc==eLShift || eFunc==eRShift || eFunc==eAnd || eFunc==eBitAnd)) {
			return true;
		}
		if (param_value[1]->equals(0) && (eFunc==eMul || eFunc==eAnd || eFunc==eBitAnd)) {
			return true;
		}
		if (param_value[0] == param_value[1] && (eFunc==eSub || eFunc==eCmpGt || eFunc==eCmpLt || eFunc==eCmpNe)) {
			return true;
		}
		if ((param_value[1]->equals(1) || param_value[1]->equals(-1)) && eFunc==eMod) {
			return true;
		}
	}
	return false;
}

bool 
FunctionInvocationBinary::is_0_or_1(void) const 
{ 
	return eFunc==eCmpGt || eFunc==eCmpLt || eFunc==eCmpGe || eFunc==eCmpLe || eFunc==eCmpEq || eFunc==eCmpNe;
}

/*
 * XXX --- we should memoize the types of "standard functions."
 */
const Type &
FunctionInvocationBinary::get_type(void) const
{
	switch (eFunc) {
	default:
		assert(!"invalid operator in FunctionInvocationBinary::get_type()");
		break;

	case eAdd:
	case eSub:
	case eMul:
	case eDiv:
	case eMod:
	case eBitXor:
	case eBitAnd:
	case eBitOr:
		{
			const Type &l_type = param_value[0]->get_type();
			const Type &r_type = param_value[1]->get_type();
			// XXX --- not really right!
			if ((l_type.is_signed()) && (r_type.is_signed())) {
				return Type::get_simple_type(eInt);
			} else {
				return Type::get_simple_type(eUInt);
			}
		}
		break;

	case eCmpGt:
	case eCmpLt:
	case eCmpGe:
	case eCmpLe:
	case eCmpEq:
	case eCmpNe:
	case eAnd:
	case eOr:
		return Type::get_simple_type(eInt);
		break;

	case eRShift:
	case eLShift:
		{
			const Type &l_type = param_value[0]->get_type();
			// XXX --- not really right!
			if (l_type.is_signed()) {
				return Type::get_simple_type(eInt);
			} else {
				return Type::get_simple_type(eUInt);
			}
		}
		break;
	}
	assert(0);
	return Type::get_simple_type(eInt);
}   

bool 
FunctionInvocationBinary::visit_facts(vector<const Fact*>& inputs, CGContext& cg_context) const
{   
	bool skippable = IsOrderedStandardFunc(eFunc); 
	assert(param_value.size() == 2); 
	if (skippable) {
		const Expression* value = param_value[0];  
		if (value->visit_facts(inputs, cg_context)) { 
			vector<const Fact*> inputs_copy = inputs; 
			value = param_value[1];   
			if (value->visit_facts(inputs, cg_context)) {
				// the second parameter may or may not be evaludated, thus need to 
				// merge with the post-param0 env.
				merge_facts(inputs, inputs_copy);
				return true;
			} 
		}
		return false;
	}   
	// for other binary invocations, use the standard visitor
	return FunctionInvocation::visit_facts(inputs, cg_context); 
}  
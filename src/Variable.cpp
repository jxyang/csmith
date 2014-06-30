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

//
// This file was derived from a random program generator written by Bryan
// Turner.  The attributions in that file was:
//
// Random Program Generator
// Bryan Turner (bryan.turner@pobox.com)
// July, 2005
//

#ifdef WIN32 
#pragma warning(disable : 4786)   /* Disable annoying warning messages */
#endif
#include "Variable.h"

#include <cassert>
#include <sstream>

#include "Common.h"
#include "Block.h"
#include "CGContext.h"
#include "CGOptions.h"
#include "Constant.h"
#include "Effect.h"
#include "Function.h"
#include "Type.h"
#include "Fact.h"
#include "FactMgr.h"
#include "FactPointTo.h"
#include "FactUnion.h"
#include "random.h"
#include "AbsOutputMgr.h"
#include "ProgramGenerator.h"
#include "Lhs.h"
#include "ExpressionVariable.h"
#include "Bookkeeper.h"
#include "Filter.h"
#include "ProgramGenerator.h"

#include "ArrayVariable.h"
#include "StringUtils.h"


using namespace std;
std::vector< std::vector<const Variable*>* > Variable::ctrl_vars_vectors;
unsigned long Variable::ctrl_vars_count;

const string Variable::sink_var_name = "csmith_sink_";

//////////////////////////////////////////////////////////////////////////////

int find_variable_in_set(const vector<const Variable*>& set, const Variable* v)
{
    size_t i;
    for (i=0; i<set.size(); i++) {
        if (set[i]->match(v)) {
            return i;
        } 
    }
    return -1;
}

int find_variable_in_set(const vector<Variable*>& set, const Variable* v)
{
    size_t i;
    for (i=0; i<set.size(); i++) {
        if (set[i]->match(v)) {
            return i;
        }
    }
    return -1;
}

int find_field_variable_in_set(const vector<const Variable*>& set, const Variable* v)
{
    size_t i;
	if (v->IsAggregate()) {
		for (i=0; i<v->field_vars.size(); i++) {
			const Variable* field = v->field_vars[i];
			int pos = find_variable_in_set(set, field);
			if (pos != -1) return pos;
			pos = find_field_variable_in_set(set, field);
			if (pos != -1) return pos;
		}
	}
    return -1;
}

bool is_variable_in_set(const vector<const Variable*>& set, const Variable* v)
{
    size_t i;
    for (i=0; i<set.size(); i++) {
        if (set[i] == v) {
            return true;
        }
    }
    return false;
}

bool add_variable_to_set(vector<const Variable*>& set, const Variable* v)
{
	if (!is_variable_in_set(set, v)) {
		set.push_back(v);
		return true;
	}
    return false;
}

bool add_variables_to_set(vector<const Variable*>& set, const vector<const Variable*>& new_set)
{
	size_t i;
	bool changed = false;
	for (i=0; i<new_set.size(); i++) {
		if (add_variable_to_set(set, new_set[i])) {
			changed = true;
		}
	}
	return changed;
}

// return true if two sets contains same variables
bool equal_variable_sets(const vector<const Variable*>& set1, const vector<const Variable*>& set2)
{
    size_t i;
    if (set1.size() == set2.size()) { 
        for (i=0; i<set1.size(); i++) {
            if (!is_variable_in_set(set2, set1[i])) {
                return false;
            }
        }
        return true;
    }
    return false;
}

// return true if set1 is subset of set2, or equal
bool sub_variable_sets(const vector<const Variable*>& set1, const vector<const Variable*>& set2)
{
    size_t i;
    if (set1.size() <= set2.size()) { 
        for (i=0; i<set1.size(); i++) {
            if (!is_variable_in_set(set2, set1[i])) {
                return false;
            }
        }
        return true;
    }
    return false;
}

// combine two variable sets into one, note struct field "s1.f1" and "s1" is combined into "s1"
void combine_variable_sets(const vector<const Variable*>& set1, const vector<const Variable*>& set2, vector<const Variable*>& set_all)
{
	size_t i;
	set_all = set1;
	for (i=0; i<set2.size(); i++) {
		const Variable* v = set2[i];
		if (find_variable_in_set(set1, v) == -1) {
			set_all.push_back(v);
		}
	}
}

/* replace all the field vars with their parent vars */
void remove_field_vars(vector<const Variable*>& set)
{
	size_t i;
	size_t len = set.size();
	for (i=0; i<len; i++) {
		const Variable* v = set[i];
		if (v->is_field_var()) {
			while (v->field_var_of) {
				v = v->field_var_of;
			}
			set.erase(set.begin() + i);
			add_variable_to_set(set, v);
			i--;
			len = set.size();
		}
	}
}

const Variable*
Variable::get_container_union(void) const
{
	if (type == NULL) return NULL;
	const Variable* p = this;
	for (; p && p->type->eType != eUnion; p = p->field_var_of)
		;
	return p;
}

/*
 * examples: array[0] "loose matches" array[1]; array[3] "loose matches" array[x].f1...
 * union.f1 "loose matches" union.f2.f3
 */
bool 
Variable::loose_match(const Variable* v) const
{
	const Variable* me = get_collective();
	const Variable* you = v->get_collective();
	if (me->match(you)) {
		return true;
	}
	// find the union variable(s) that contain me and you
	me = me->get_container_union();
	you = you->get_container_union();
	return you && me && (you == me);
}

/*
 * a struct variable "matches" it's field variable
 */
bool 
Variable::match(const Variable* v) const
{
	if (type && v->type && type->IsAggregate()) {
		return (this == v) || has_field_var(v);
	}
	return this == v;
}

int
Variable::get_seq_num(void) const
{
	size_t index = name.find('_');
	assert(index != string::npos);
	return StringUtils::str2int(name.substr(index+1));
}

/*
 * return if this is the field of an array member
 */
bool 
Variable::is_array_field(void) const 
{ 
	if (field_var_of) {
		return field_var_of->is_array_field();
	}
	return isArray;
}

/*
 * return if this is the field of an pre-itemized array member
 */
bool 
Variable::is_virtual(void) const 
{ 
	if (field_var_of) {
		return field_var_of->is_virtual();
	}
	if (isArray) {
		return ((const ArrayVariable*)this)->collective==0;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
 /* check if field variables exists in this struct
  ************************************************************************/
bool Variable::has_field_var(const Variable* v) const
{
	if (type->IsAggregate()) {
		const Variable* tmp = v;
		while (tmp) {
			if (tmp == this) {
				return true;
			}
			tmp = tmp->field_var_of;
		}
    }
    return false;
}

// return true if the var is inside a packed aggregate,
bool
Variable::is_packed_aggregate_field_var() const 
{
	if (!field_var_of)
		return false;
	if (field_var_of->type->packed_)
		return true;
	return field_var_of->is_packed_aggregate_field_var();
}

const Variable* 
Variable::get_top_container(void) const
{
	const Variable* v = this;
	for (; v && v->field_var_of; v = v->field_var_of) {
		/* Empty */
	}
	return v;
}

int  
Variable::get_field_id(void) const
{ 
	if (field_var_of) {
		for (size_t i=0; i<field_var_of->field_vars.size(); i++) {
			if (field_var_of->field_vars[i] == this) {
				return i;
			}
		}
	}
	return -1;
}

///////////////////////////////////////////////////////////////////////////////
 /* expand field variables of struct, assigned names, f0, f1, etc, to them
  ************************************************************************/
void Variable::create_field_vars(const Type *type)
{
	assert(type->IsAggregate());
    size_t i, j;
    assert(type->fields.size() == type->qfers_.size());
	j = 0;
	if (name == "g_481")
		j = 0;
    bool is_vol_var = qfer.is_volatile();
    bool is_const_var = qfer.is_const();
    for (i=0; i<type->fields.size(); i++) {
		if (type->is_unamed_padding(i))
			continue;
		ostringstream ss;
		if (isArray) {
			ss << ProgramGenerator::CurrentOutputMgr()->VarRef2Str(*this);
		}
		else {
			ss << name;
		}
		ss << ".f" << j++;
		TypeQualifiers quals = type->qfers_[i];  
		quals.set_const(is_const_var || quals.is_const());
		quals.set_volatile(is_vol_var || quals.is_volatile());
		bool isBitfield = type->is_bitfield(i);
		Variable *var = Variable::CreateVariable(ss.str(), type->fields[i],
			quals.get_consts(), quals.get_volatiles(), false, false, false, isBitfield, this);
		assert(var->qfer.SanityCheck(var->type));
		field_vars.push_back(var);
    }
}

Variable *
Variable::CreateVariable(const std::string &name, const Type *type,
			   bool isConst, bool isVolatile,
			   bool isAuto, bool isStatic, 
			   bool isRegister, bool isBitfield, const Variable* isFieldVarOf)
{
	vector<bool> isConsts, isVolatiles;
	isConsts.push_back(isConst);
	isVolatiles.push_back(isVolatile);
	return CreateVariable(name, type, isConsts, isVolatiles, isAuto, isStatic, isRegister, isBitfield, isFieldVarOf);
}

Variable *
Variable::CreateVariable(const std::string &name, const Type *type,
				   const vector<bool>& isConsts, const vector<bool>& isVolatiles,
				   bool isAuto, bool isStatic, bool isRegister, bool isBitfield, const Variable* isFieldVarOf)
{
	Variable *var = new Variable(name, type, isConsts, isVolatiles,
					isAuto, isStatic, isRegister, isBitfield, isFieldVarOf);
	assert(type);
	if (type->eType == eSimple)
		assert(type->simple_type != eVoid);

	const Variable* top = isFieldVarOf;
	while (top->field_var_of) top = top->field_var_of;
	var->init = (top->type->eType == eUnion) ? 0 : Constant::make_random(type);

	
	if (type->IsAggregate()) {
		var->create_field_vars(type);
	}
	
	return var;
}

Variable *
Variable::CreateVariable(const std::string &name, const Type *type, const Expression* init, const TypeQualifiers* qfer)
{
	assert(type);
	if (type->eType == eSimple)
		assert(type->simple_type != eVoid);

	Variable *var = new Variable(name, type, init, qfer);
	if (type->IsAggregate()) {
		var->create_field_vars(type);
	}
	
	return var;
}

/*
 *
 */
Variable::Variable(const std::string &name, const Type *type,
				   const vector<bool>& isConsts, const vector<bool>& isVolatiles,
				   bool isAuto, bool isStatic, bool isRegister, bool isBitfield, const Variable* isFieldVarOf)
	: name(name), type(type),
	  init(0),
	  isAuto(isAuto), isStatic(isStatic), isRegister(isRegister),
	  isBitfield_(isBitfield), isAddrTaken(false), isAccessOnce(false), 
	  field_var_of(isFieldVarOf), isArray(false),
	  qfer(isConsts, isVolatiles)
{
	// nothing else to do
}

/*
 *
 */
Variable::Variable(const std::string &name, const Type *type, const Expression* init, const TypeQualifiers* qfer)
	: name(name), type(type),
	  init(init),
	  isAuto(false), isStatic(false), isRegister(false), isBitfield_(false), 
	  isAddrTaken(false), isAccessOnce(false),
	  field_var_of(0), isArray(false),
	  qfer(*qfer)
{
	// nothing else to do
}

Variable::Variable(const std::string &name, const Type *type, const Expression* init, const TypeQualifiers* qfer, const Variable* isFieldVarOf, bool isArray)
	: name(name), type(type),
	  init(init),
	  isAuto(false), isStatic(false), isRegister(false), isBitfield_(false),
	  isAddrTaken(false), isAccessOnce(false),
	  field_var_of(isFieldVarOf),
	  isArray(isArray),
	  qfer(*qfer)
{
	// nothing else to do
}

/*
 *
 */
Variable::~Variable(void)
{ 
	// delete field vars explicitly because they are not stored in AllVars anymore
	vector<Variable *>::iterator i;
	for(i = field_vars.begin(); i != field_vars.end(); ++i)
		delete (*i); 
	field_vars.clear();
	if (init) {
		delete init;
		init = NULL;
	}
} 

// --------------------------------------------------------------
bool
Variable::is_global(void) const
{
	if (is_field_var()) {
		return field_var_of->is_global();
	}
	return (name.find("g_") == 0);
}

bool
Variable::is_local(void) const
{
	return (name.find("l_") == 0);
}

// ------------------------------------------------------------- 
bool
Variable::is_visible_local(const Block* blk) const
{  
	if (blk == 0) {
		return is_global();
	}
	if (is_field_var()) {
		return field_var_of->is_visible_local(blk);
	}
	size_t i;
	const Function* func = blk->func;
	for (i=0; i<func->param.size(); i++) {
		if (func->param[i]->match(this)) {
			return true;
 		}
	} 
	const Block* b = blk;
	while (b) {
		if (find_variable_in_set(b->local_vars, this) != -1) {
			return true;
		}
		b = b->parent;
	}
    return false;
} 

// --------------------------------------------------------------
bool
Variable::is_argument(void) const
{
	// JYTODO: need stronger criteria?
	return (name.find("p_") == 0);
}

// --------------------------------------------------------------
bool
Variable::is_tmp_var(void) const
{
	// JYTODO: need stronger criteria?
	return (name.find("t") == 0);
}

bool 
Variable::is_const(void) const 
{
	return is_const_after_deref(0);
}

bool 
Variable::is_volatile(void) const 
{
	return is_volatile_after_deref(0);
}

bool 
Variable::is_const_after_deref(int deref_level) const 
{
	if (deref_level < 0) {
		return false;
	}
	// check qualifiers
	if (qfer.is_const_after_deref(deref_level)) {
		return true;
	}
	if (type) {
		// check struct/union type
		int i;
		const Type* t = type;
		for (i=0; i<deref_level; i++) {
			t = t->ptr_type;
		}
		assert(t);
		return t->is_const_struct_union();
	}
	return false;
}

bool 
Variable::is_volatile_after_deref(int deref_level) const 
{
	if (deref_level < 0) {
		return false;
	}
	// check qualifiers
	if (qfer.is_volatile_after_deref(deref_level)) {
		return true;
	}
	if (type) {
		// check struct/union type
		int i;
		const Type* t = type;
		for (i=0; i<deref_level; i++) {
			t = t->ptr_type;
		}
		assert(t);
		return t->is_volatile_struct_union();
	}
	return false;
} 

const Variable* 
Variable::get_collective(void) const
{
	// special handling for array fields
	if (is_array_field()) {
		// find top-level parent, which should be an array
		const Variable* parent = field_var_of;
		for (; parent && !parent->isArray; parent = parent->field_var_of) {
			/* Empty. */
		}
		assert(parent);
		// if this is alreay a field of a collective array, return itself
		if (parent->get_collective() == parent) return this;

		// find the collective for top-level array, and return the corresponding field var
		const Variable* coll = parent->get_collective();
		size_t index, pos1, pos2;
		size_t pos3 = name.find_last_of("]");
		assert(pos3 != string::npos);
		string field_names = name.substr(pos3); 
		pos1 = field_names.find(".");
		while (pos1 != string::npos) {
			pos2 = field_names.find(".", pos1+1);
			string s = (pos2 == string::npos) ? field_names.substr(pos1+2) : field_names.substr(pos1+2, pos2-pos1-2);
			index = StringUtils::str2int(s);
			assert(index < coll->field_vars.size());
			coll = coll->field_vars[index];
			pos1 = pos2;
		}
		return coll;
	} else {
		return this;
	}
}

const Variable* 
Variable::get_named_var(void) const
{
	const Variable* v = this;
	while (v->field_var_of) {
		v = v->field_var_of;
	}
	return v->get_collective();
}

const ArrayVariable* 
Variable::get_array(string& field) const
{
	// special handling for array fields
	if (is_array_field()) {
		// find top-level parent, which should be an array
		const Variable* parent = field_var_of;
		for (; parent && !parent->isArray; parent = parent->field_var_of) {
			/* Empty. */
		}
		assert(parent);

		size_t bracket = name.find_last_of("]");
		if (bracket == string::npos) {
			bracket = 0;
		}
		size_t dot = name.find(".", bracket);
		assert(dot != string::npos);
		field = name.substr(dot);
		return (const ArrayVariable*)parent;
	}
	return NULL;
}  

std::string
Variable::get_actual_name() const
{
	std::string s = name;

	if (is_global())
		return get_prefixed_name(s);
	else
		return s;
} 

// --------------------------------------------------------------
// This function is a bit of hack, because ---
//   &VOL_RVAL(g_4)
// is invalid when VOL_RVAL expands into a function call.
void
Variable::OutputAddrOf(std::ostream &out) const
{
	out << "&" << get_actual_name();
}

// --------------------------------------------------------------
void
Variable::OutputForComment(std::ostream &out) const
{
	out << get_actual_name();
} 

// --------------------------------------------------------------
void
Variable::OutputUpperBound(std::ostream &out) const
{
	if (field_var_of) {
		field_var_of->OutputUpperBound(out);
		size_t dot = name.find_last_of(".");
		assert(dot != string::npos);
		string postfix = name.substr(dot, string::npos);
		out << postfix;
	}
	else {
		out << get_actual_name();
	}
}

// --------------------------------------------------------------
void
Variable::OutputLowerBound(std::ostream &out) const
{
	if (field_var_of) {
		field_var_of->OutputLowerBound(out);
		size_t dot = name.find_last_of(".");
		assert(dot != string::npos);
		string postfix = name.substr(dot, string::npos);
		out << postfix;
	}
	else {
		out << get_actual_name();
	}
}

// --------------------------------------------------------------
std::vector<const Variable*>&
Variable::get_new_ctrl_vars(size_t count)
{
	unsigned long ctrl_var_suffix = Variable::ctrl_vars_count;
	TypeQualifiers dummy;
	dummy.add_qualifiers(false, false);
	char name = 'i';
	vector<const Variable *> *ctrl_vars = new vector<const Variable *>();
	assert(ctrl_vars);

	for (int i=0; i<count; i++) { 
		stringstream name_stream;
		name_stream << name;
		//if (CGOptions::fresh_array_ctrl_var_names())
			name_stream << ctrl_var_suffix;
		Variable *v = new Variable(name_stream.str(), get_int_type(), 0, &dummy);
		assert(v);
		ctrl_vars->push_back(v);
		name++;
	}
	Variable::ctrl_vars_count++;
	ctrl_vars_vectors.push_back(ctrl_vars);
	return *ctrl_vars;
} 

std::vector<const Variable*>&
Variable::get_last_ctrl_vars()
{
	return *Variable::ctrl_vars_vectors.back();
}

// ------------------------------------------------------------
void
Variable::doFinalization(void)
{
	for (vector< vector<const Variable *>* >::iterator vi = ctrl_vars_vectors.begin(),
	     ve = ctrl_vars_vectors.end(); vi != ve; ++vi) {
		vector<const Variable *> *v = (*vi);
		for (vector<const Variable *>::iterator i = v->begin(),
		     e = v->end(); i != e; ++i) {
			delete (*i);
		}
		delete v;
	}
	ctrl_vars_vectors.clear();
}

// --------------------------------------------------------------
void
MapVariableList(const vector<Variable*> &var, std::ostream &out,
				int (*func)(Variable *var, std::ostream *pOut))
{
	for_each(var.begin(), var.end(), std::bind2nd(std::ptr_fun(func), &out));
} 

size_t
Variable::GetMaxArrayDimension(const vector<Variable*>& vars)
{
	// find the largest dimension of arrays, if there is any
	size_t dimen = 0; 

	for (size_t i=0; i<vars.size(); i++) {
		if (vars[i]->isArray) {
			ArrayVariable* av = (ArrayVariable*)(vars[i]);
			// const Array members were initialzed in ArrayVariable::OutputDef 
			if (av->get_dimension() > dimen) {
				dimen = av->get_dimension();
			} 
		}
	}
	return dimen;
} 

bool 
Variable::compatible(const Variable *v) const 
{
	if (is_volatile() || v->is_volatile())
		return false;
	else if (this == v)
		return true;
	else if (CGOptions::expand_struct())
		return (!v->is_field_var() && !is_field_var());
	else
		return false;
}  

bool
Variable::is_seen_name(vector<std::string> &seen_names, const std::string &name) const
{
	for (vector<string>::iterator i = seen_names.begin(); i != seen_names.end(); ++i) {
		std::string n = (*i);
		n += "[";
		if (!name.compare(0, n.length(), n)) {
			return true;
		}
	}
	return false;
}

bool 
Variable::is_valid_volatile(void) const
{
	if (is_inside_union_field()) {
		const Variable *uv = get_container_union();
		assert(uv && "NULL union var!");
		return uv->is_valid_volatile();
	}	

	assert(init && "NULL init expression!");
	if (!is_const() || init->not_equals(0) || (type->eType != ePointer))
		return true;

	return false;
} 

const Variable*
Variable::match_var_name(const string& vname) const
{
	// for simple variables
	if (name == vname) {
		return this;
	}
	// for array variables
	if (isArray || is_array_field()) {
		string s = ProgramGenerator::CurrentOutputMgr()->VarRef2Str(*this); 
		if (s == vname) {
			return this;
		}
	}
	// for struct variables 
	size_t i;
	for (i=0; i<field_vars.size(); i++) {
		const Variable* v = field_vars[i]->match_var_name(vname);
		if (v) {
			return v;
		}
	}
	return NULL;
}

void 
Variable::find_pointer_fields(vector<const Variable*>& ptr_fields) const
{
	for (size_t i=0; i<field_vars.size(); i++) {
		if (field_vars[i]->is_pointer()) {
			ptr_fields.push_back(field_vars[i]);
		}
		else if (field_vars[i]->IsAggregate()) {
			field_vars[i]->find_pointer_fields(ptr_fields);
		}
	}
}

/* a struct field packed after a bit-field could has nondeterministic offset due to incompatible packings between compilers */
bool
Variable::is_packed_after_bitfield(void) const 
{ 
	const Variable* parent = this->field_var_of; 
	if (parent == NULL) return false;
	if (parent->type->eType == eStruct && parent->type->packed_) {
		for (size_t i=0; i<parent->field_vars.size(); i++) {
			if (parent->field_vars[i] == this) {
				break;
			}
			if (parent->type->is_bitfield(i) || parent->field_vars[i]->type->has_bitfields()) {
				return true;
			} 
		}
	} 
	return parent->is_packed_after_bitfield();
} 

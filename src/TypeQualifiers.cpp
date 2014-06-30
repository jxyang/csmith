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

#include <assert.h>
#include <iostream>
#include "TypeQualifiers.h"
#include "Type.h"
#include "Effect.h"
#include "CGContext.h"
#include "CGOptions.h"
#include "random.h"

#include "Probabilities.h" 
#include "Enumerator.h"
 
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
TypeQualifiers::TypeQualifiers(void)
: wildcard(false),
  accept_stricter(false)
{
	// nothing else to do
}

TypeQualifiers::TypeQualifiers(bool wild, bool accept_stricter)
: wildcard(wild),
  accept_stricter(accept_stricter)
{
	// nothing else to do
}

TypeQualifiers::TypeQualifiers(const vector<bool>& isConsts, const vector<bool>& isVolatiles)
: wildcard(false),
  accept_stricter(false),
  is_consts(isConsts),
  is_volatiles(isVolatiles)
{
	// nothing else to do
}

TypeQualifiers::TypeQualifiers(const TypeQualifiers &qfer)
: wildcard(qfer.wildcard),
  accept_stricter(qfer.accept_stricter),
  is_consts(qfer.get_consts()),
  is_volatiles(qfer.get_volatiles())
{
	// nothing else to do
}

TypeQualifiers::~TypeQualifiers()
{
}

TypeQualifiers &
TypeQualifiers::operator=(const TypeQualifiers &qfer)
{	
	if (this == &qfer) {
		return *this;
	}
	wildcard = qfer.wildcard;
	accept_stricter = qfer.accept_stricter;
	is_consts = qfer.get_consts();
	is_volatiles = qfer.get_volatiles();
	return *this;
}

// --------------------------------------------------------------
 /* return true if this variable is more const-volatile qualified than v
  * some examples are:
  *    const is more qualified than none
  *    volatile is more qualified than none
  *    const volatile is more qualified than const
  *    const is NOT more qualified than volatile
  *    ...
  *  notice "const int**" is not convertable from "int**"
  *  as explained in 
  * http://www.embedded.com/columns/programmingpointers/180205632?_requestid=488055
  **************************************************************/
bool 
TypeQualifiers::stricter_than(const TypeQualifiers& qfer) const
{
	size_t i; 
	assert(is_consts.size() == is_volatiles.size());
	const vector<bool>& v_consts = qfer.get_consts();
	const vector<bool>& v_volatiles = qfer.get_volatiles();
	if (is_consts.size() != v_consts.size() || is_volatiles.size() != v_volatiles.size()) {
		return false;
	}
	
	size_t depth = is_consts.size();
	// check "const" qualifier first 
	for (i=0; i<depth; i++) {
		// for special rule: "const int**" is not convertable from "int**"
		// actually for a level that is followed by two "*"s, we have to match 
		// "const" qualifier
		if (depth - i > 2 && is_consts[i] != v_consts[i]) {
			return false;
		}
		if (v_consts[i] && !is_consts[i]) {
			return false;
		}
	}

	// check "volatile" qualifier second
	// special rule: the volatile property on storage (1st in vector) must match
	// can be relaxed???
	if (depth > 1 && is_volatiles[0] != v_volatiles[0]) {
		return false;
	}
	for (i=0; i<depth; i++) {
		// similiar to const: "volatile int**" is not convertable from "int**"
		// actually for a level that is followed by two "*"s, we have to match
		if (depth - i > 2 && is_volatiles[i] != v_volatiles[i]) {
			return false;
		}
		if (v_volatiles[i] && !is_volatiles[i]) {
			return false;
		}
	}		
	return true;
}

bool 
TypeQualifiers::match(const TypeQualifiers& qfer) const
{
	if (wildcard) {
		return true;
	}
	if (CGOptions::match_exact_qualifiers()) {
		return is_consts == qfer.get_consts() && is_volatiles == qfer.get_volatiles();
	}
	// return true if both variables are non-pointer (has only one level qualifier)
	if (is_consts.size() == qfer.get_consts().size() && is_consts.size()==1) {
		assert(is_consts.size() == is_volatiles.size());
		return true;
	}
	return (!accept_stricter && stricter_than(qfer)) || (accept_stricter && qfer.stricter_than(*this));
}

bool 
TypeQualifiers::match_indirect(const TypeQualifiers& qfer) const
{
	if (wildcard) {
		return true;
	}
	if (is_consts.size() == qfer.get_consts().size()) {
		return match(qfer);
	}
	int deref = qfer.get_consts().size() - is_consts.size();
	if (deref < -1) {
		return false;
	}
	return match(qfer.indirect_qualifiers(deref));
}

/*
 * make sure no volatile-pointers if volatile-pointers is false
 */
void
TypeQualifiers::make_scalar_volatiles(std::vector<bool> &volatiles)
{
	if (!CGOptions::volatile_pointers()) {
		for (size_t i=1; i<volatiles.size(); i++)
			volatiles[i] = false;
	}
}

/*
 * make sure no const-pointers if const_pointers is false
 */
void
TypeQualifiers::make_scalar_consts(std::vector<bool> &consts)
{
	if (!CGOptions::const_pointers()) {
		for (size_t i=1; i<consts.size(); i++)
			consts[i] = false;
	}
}

/* 
 * generate a random CV qualifier vector that is looser or stricter than this one
 */
TypeQualifiers 
TypeQualifiers::random_qualifiers(bool no_volatile, Effect::Access access, const CGContext &cg_context) const
{
	std::vector<bool> volatiles;
	std::vector<bool> consts;
	if (wildcard) {
		return TypeQualifiers(true, accept_stricter);
	}
	// use non-volatile for all levels if requested
	if (no_volatile) {
		for (size_t i=0; i<is_volatiles.size(); i++) {
			volatiles.push_back(false);
		}
	}
	else {
		volatiles = !accept_stricter ? random_looser_volatiles() : random_stricter_volatiles(); 
		if (!cg_context.get_effect_context().is_side_effect_free()) {
			volatiles[volatiles.size() - 1] = false;
		}
	}

	make_scalar_volatiles(volatiles);
	consts = !accept_stricter ? random_looser_consts() : random_stricter_consts();
	make_scalar_consts(consts); 
	if (access == Effect::WRITE) {
		consts[consts.size() - 1] = false;
	}
	return TypeQualifiers(consts, volatiles);
}

/* 
 * generate a random CV qualifier vector that is looser than this one
 */
TypeQualifiers 
TypeQualifiers::random_loose_qualifiers(bool no_volatile, Effect::Access access, const CGContext &cg_context) const
{
	std::vector<bool> volatiles;
	std::vector<bool> consts;
	if (wildcard) {
		return TypeQualifiers(true, accept_stricter);
	}
	// use non-volatile for all levels if requested
	if (no_volatile) {
		for (size_t i=0; i<is_volatiles.size(); i++) {
			volatiles.push_back(false);
		}
	}
	else {
		volatiles = random_looser_volatiles(); 
		if (!cg_context.get_effect_context().is_side_effect_free()) {
			volatiles[volatiles.size() - 1] = false;
		}
	} 
	make_scalar_volatiles(volatiles);

	consts = random_looser_consts();
	make_scalar_consts(consts); 
	if (access == Effect::WRITE) {
		consts[consts.size() - 1] = false;
	}
	return TypeQualifiers(consts, volatiles);
}

TypeQualifiers
TypeQualifiers::random_qualifiers(const Type* t, Effect::Access access, 
				const CGContext &cg_context, bool no_volatile)
{
	return random_qualifiers(t, access, cg_context, no_volatile, RegularConstProb, RegularVolatileProb);
}

TypeQualifiers 
TypeQualifiers::random_qualifiers(const Type* t, Effect::Access access, const CGContext &cg_context, bool no_volatile, 
					unsigned int const_prob, unsigned int volatile_prob)
{ 
	TypeQualifiers ret_qfer;
	if (t==0) {
		return ret_qfer;
	}
	bool isVolatile = false;
	bool isConst = false;
	std::vector<bool> is_consts, is_volatiles;
	const Effect &effect_context = cg_context.get_effect_context(); 

	// set random volatile/const properties for each level of indirection for pointers
	const Type* tmp = t->ptr_type;
	while (tmp) {    
		isVolatile = rnd_flipcoin(volatile_prob); 
		isConst = rnd_flipcoin(const_prob);  
		if (isVolatile && isConst && !CGOptions::allow_const_volatile()) {
			isConst = false;
		}
		is_consts.push_back(isConst);
		is_volatiles.push_back(isVolatile);
		tmp = tmp->ptr_type;
	}
	// set random volatile/const properties for variable itself
	bool volatile_ok = effect_context.is_side_effect_free();
	bool const_ok = (access != Effect::WRITE);

	isVolatile = false;
	isConst = false;

	if (volatile_ok && const_ok) { 
		isVolatile = rnd_flipcoin(volatile_prob); 
		isConst = rnd_flipcoin(const_prob); 
	}
	else if (volatile_ok) { 
		isVolatile = rnd_flipcoin(volatile_prob); 
	}
	else if (const_ok) { 
		isConst = rnd_flipcoin(const_prob); 
	}

	if (isVolatile && isConst && !CGOptions::allow_const_volatile()) {
		isConst = false;
	}
	is_consts.push_back(isConst);
	is_volatiles.push_back(isVolatile);
	// use non-volatile for all levels if requested
	if (no_volatile) {
		for (size_t i=0; i<is_volatiles.size(); i++) {
			is_volatiles[i] = false;
		}
	}
	make_scalar_volatiles(is_volatiles);
	make_scalar_consts(is_consts);
	return TypeQualifiers(is_consts, is_volatiles);
}

/* 
 * make a random qualifier for type t, assuming non context,
 * and no volatile allowed
 */
TypeQualifiers 
TypeQualifiers::random_qualifiers(const Type* t)
{ 
	return random_qualifiers(t, Effect::READ, CGContext::get_empty_context(), true);
}

/* 
 * be careful to use it because it will generate volatile without knowing the context. 
 * Only used to generate qulifiers for struct/unions
 */
TypeQualifiers 
TypeQualifiers::random_qualifiers(const Type* t, unsigned int const_prob, unsigned int volatile_prob)
{ 
	return random_qualifiers(t, Effect::READ, CGContext::get_empty_context(), false, const_prob, volatile_prob);
}

vector<bool> 
TypeQualifiers::random_stricter_consts(void) const
{
	vector<bool> consts;
	size_t i;
	size_t depth = is_consts.size();
	if (CGOptions::match_exact_qualifiers()) return is_consts;
	for (i=0; i<depth; i++) {
		// special case
		// const int** is not stricter than int**
		// int * const ** is not stricter than int***
		// and so on...
		if (is_consts[i] || (depth - i > 2)) {
			consts.push_back(is_consts[i]);
		}
		else if (is_volatiles[i] && !CGOptions::allow_const_volatile()) {
			consts.push_back(false);
		}
		else { 
			bool index = rnd_flipcoin(StricterConstProb); 
			consts.push_back(index);
		}
	}
	return consts;
}

vector<bool> 
TypeQualifiers::random_stricter_volatiles(void) const
{
	vector<bool> volatiles;
	size_t i;
	size_t depth = is_volatiles.size();
	if (CGOptions::match_exact_qualifiers()) return is_volatiles;
	for (i=0; i<depth; i++) {
		// first one (storage must match, any level followed by at least two more
		// indirections must match
		if (is_volatiles[i] || (i==0 && depth>1) || (depth - i > 2)) {
			volatiles.push_back(is_volatiles[i]);
		}
		else if (is_consts[i] && !CGOptions::allow_const_volatile()) {
			volatiles.push_back(false);
		}
		else { 
			bool index = rnd_flipcoin(RegularVolatileProb); 
			volatiles.push_back(index);
		}
	}
	make_scalar_volatiles(volatiles);
	return volatiles;
}

vector<bool> 
TypeQualifiers::random_looser_consts(void) const
{
	vector<bool> consts;
	size_t i;
	size_t depth = is_consts.size();
	if (CGOptions::match_exact_qualifiers()) return is_consts;
	for (i=0; i<depth; i++) {
		// special case
		if (!is_consts[i] || (depth - i > 2)) {
			consts.push_back(is_consts[i]);
		}
		else { 
			bool index = rnd_flipcoin(LooserConstProb); 
			consts.push_back(index);
		}
	}
	return consts;
}

vector<bool> 
TypeQualifiers::random_looser_volatiles(void) const
{
	vector<bool> volatiles;
	size_t i;
	size_t depth = is_volatiles.size();
	if (CGOptions::match_exact_qualifiers()) return is_volatiles;
	for (i=0; i<depth; i++) {
		if (!is_volatiles[i] || (i==0 && depth>1) || (depth - i > 2)) {
			volatiles.push_back(is_volatiles[i]);
		}
		else { 
			bool index = rnd_flipcoin(RegularVolatileProb); 
			volatiles.push_back(index);
		}
	}
	make_scalar_volatiles(volatiles);
	return volatiles;
}

void 
TypeQualifiers::add_qualifiers(bool is_const, bool is_volatile)
{
	is_consts.push_back(is_const);
	is_volatiles.push_back(is_volatile);
}


// actually add qualifiers to pointers
TypeQualifiers 
TypeQualifiers::random_add_qualifiers(bool no_volatile) const
{
	TypeQualifiers qfer = *this;
	if (CGOptions::match_exact_qualifiers()) {
		qfer.add_qualifiers(false, false);
		return qfer;
	}
	//bool is_const = rnd_upto(50); 

	bool is_const;
	if (!CGOptions::const_pointers())
		is_const = false;
	else
		is_const = rnd_flipcoin(RegularConstProb);

	//bool is_volatile = no_volatile ? false : rnd_upto(RegularVolatileProb);  
	bool is_volatile;
	if (no_volatile || !CGOptions::volatile_pointers())
		is_volatile = false;
	else
		is_volatile = rnd_flipcoin(RegularVolatileProb);
	 
	qfer.add_qualifiers(is_const, is_volatile);
	return qfer;
}

void 
TypeQualifiers::remove_qualifiers(int len)
{
	int i;
	for (i=0; i<len; i++) {
		is_consts.pop_back();
		is_volatiles.pop_back();
	}
}

TypeQualifiers 
TypeQualifiers::indirect_qualifiers(int level) const
{
	if (level == 0 || wildcard) {
		return *this;
	}
	// taking address
	else if (level < 0) {
		assert(level == -1);
		TypeQualifiers qfer = *this;
		qfer.add_qualifiers(false, false);
		return qfer;
	}
	// dereference
	else {
		TypeQualifiers qfer = *this;
		qfer.remove_qualifiers(level);
		return qfer;
	}
}

/*
 * check if the indirect depth of type matches qualifier size
 */
bool 
TypeQualifiers::SanityCheck(const Type* t) const
{
	assert(t);
	int level = t->get_indirect_level();
	assert(level >= 0);
	return wildcard || (is_consts.size() == is_volatiles.size() && (static_cast<size_t>(level)+1) == is_consts.size());
}  

bool 
TypeQualifiers::is_const_after_deref(int deref_level) const 
{
	if (deref_level < 0) {
		return false;
	}
	size_t len = is_consts.size();
	assert(len > static_cast<size_t>(deref_level));
	return is_consts[len - deref_level - 1];
}
	
bool 
TypeQualifiers::is_volatile_after_deref(int deref_level) const 
{
	if (deref_level < 0) {
		return false;
	}
	size_t len = is_volatiles.size();
	assert(len > static_cast<size_t>(deref_level));
	/*
	if (len <= static_cast<size_t>(deref_level)) {
		cout << "len = " << len << ", deref_level = " << deref_level << std::endl;
		assert(0);
	}
	*/
	return is_volatiles[len - deref_level - 1];
}

void 
TypeQualifiers::set_const(bool is_const, int pos)
{
	int len = is_consts.size();
	if (len > 0) {
		is_consts[len - pos - 1] = is_const;
	}
}

void 
TypeQualifiers::set_volatile(bool is_volatile, int pos)
{
	int len = is_volatiles.size();
	if (len > 0) {
		is_volatiles[len - pos - 1] = is_volatile;
	}
}

void
TypeQualifiers::restrict(Effect::Access access, const CGContext& cg_context)
{
	if (access == Effect::WRITE) {
		set_const(false);
	}
	if (!cg_context.get_effect_context().is_side_effect_free()) {
		set_volatile(false);
	}
}
	
/*
 * For now, only used to generate all qualifiers for struct fields. 
 * Also, since we don't support fields with pointer types, we only 
 * enumerate the first level of qualifiers.
 */
void
TypeQualifiers::get_all_qualifiers(vector<TypeQualifiers> &quals, unsigned int const_prob, unsigned int volatile_prob)
{
	Enumerator<string> qual_enumerator;
	qual_enumerator.add_bool_elem("const_prob", const_prob);
	qual_enumerator.add_bool_elem("volatile_prob", volatile_prob);
	Enumerator<string> *i;
	for (i = qual_enumerator.begin(); i != qual_enumerator.end(); i = i->next()) {
		bool isConst = i->get_elem("const_prob");
		bool isVolatile = i->get_elem("volatile_prob");

		vector<bool> consts;
		vector<bool> volatiles;

		consts.push_back(isConst);
		volatiles.push_back(isVolatile);
		TypeQualifiers qual(consts, volatiles);

		quals.push_back(qual);
	}
}



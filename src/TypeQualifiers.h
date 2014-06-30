// -*- mode: C++ -*-
//
// Copyright (c) 2008, 2009, 2010, 2011 The University of Utah
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

#ifndef TypeQualifiers_H
#define TypeQualifiers_H

#include <vector>
#include <string>
using namespace std;

#include "Effect.h"
class Type;
class CGContext;

class TypeQualifiers  
{
public:
	TypeQualifiers(void);
	TypeQualifiers(bool wild, bool accept_stricter);
	TypeQualifiers(const vector<bool>& isConsts, const vector<bool>& isVolatiles);
	virtual ~TypeQualifiers();

	TypeQualifiers(const TypeQualifiers &qfer); 
	TypeQualifiers &operator=(const TypeQualifiers &qfer);

	bool is_const(void) const { return is_const_after_deref(0);}
	bool is_volatile(void) const { return is_volatile_after_deref(0); }
	bool is_storage_const(void) const { return is_consts[0];}
	bool is_storage_volatile(void) const { return is_volatiles[0];}
	bool is_const_after_deref(int deref_level) const; 
	bool is_volatile_after_deref(int deref_level) const;
	void set_const(bool is_const, int pos=0);
	void set_volatile(bool is_volatile, int pos=0);
	void restrict(Effect::Access access, const CGContext& cg_context);

	bool stricter_than(const TypeQualifiers& qfer) const;

	bool match(const TypeQualifiers& qfer) const; 
	bool match_indirect(const TypeQualifiers& qfer) const;
	
	const vector<bool>& get_consts(void) const { return is_consts;}
	const vector<bool>& get_volatiles(void) const { return is_volatiles;}
	TypeQualifiers random_qualifiers(bool no_volatile, Effect::Access access, const CGContext &cg_context) const;
	TypeQualifiers random_loose_qualifiers(bool no_volatile, Effect::Access access, const CGContext &cg_context) const;
	static void make_scalar_volatiles(std::vector<bool> &volatiles);
	static void make_scalar_consts(std::vector<bool> &consts);
	static TypeQualifiers random_qualifiers(const Type* t, Effect::Access access, 
		const CGContext &cg_context, bool no_volatile);
	static TypeQualifiers random_qualifiers(const Type* t, Effect::Access access, 
		const CGContext &cg_context, bool no_volatile, unsigned int const_prob, unsigned int volatile_prob);
	static TypeQualifiers random_qualifiers(const Type* t);
	static TypeQualifiers random_qualifiers(const Type* t, unsigned int const_prob, unsigned int volatile_prob);

	static void get_all_qualifiers(std::vector<TypeQualifiers> &quals, 
			unsigned int const_prob, unsigned int volatile_prob);

	void add_qualifiers(bool is_const, bool is_volatile);
	TypeQualifiers random_add_qualifiers(bool no_volatile) const;
	void remove_qualifiers(int len);
	TypeQualifiers indirect_qualifiers(int level) const;

	bool SanityCheck(const Type* t) const;   
	bool wildcard;
	bool accept_stricter; 
private:
	// Type qualifiers.
	vector<bool> is_consts;
	vector<bool> is_volatiles;
	
	vector<bool> random_stricter_consts(void) const;
	vector<bool> random_stricter_volatiles(void) const;
	vector<bool> random_looser_consts(void) const;
	vector<bool> random_looser_volatiles(void) const;
};

#endif // TypeQualifiers_H

/*
 * Copyright (c) 2011, Regents of the University of California
 * All rights reserved.
 * Written by: Jeff Burke <jburke@ucla.edu>
 *             Derek Kulinski <takeda@takeda.tk>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Regents of the University of California nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL REGENTS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <Python.h>
#include <ccn/ccn.h>
#include <ccn/charbuf.h>
#include <ccn/signing.h>

#include <stdlib.h>

#include "pyccn.h"
#include "objects.h"
#include "util.h"

/*
static struct completed_closure *g_completed_closures;
 */

static struct type_to_name {
	enum _pyccn_capsules type;
	const char *name;
} g_types_to_names[] = {
	{CLOSURE, "Closure_ccn_data"},
	{CONTENT_OBJECT, "ContentObject_ccn_data"},
	{CONTENT_OBJECT_COMPONENTS, "ContentObjects_ccn_data_components"},
	{EXCLUSION_FILTER, "ExclusionFilter_ccn_data"},
	{HANDLE, "CCN_ccn_data"},
	{INTEREST, "Interest_ccn_data"},
	{KEY_LOCATOR, "KeyLocator_ccn_data"},
	{NAME, "Name_ccn_data"},
	{PARSED_CONTENT_OBJECT, "ParsedContentObject_ccn_data"},
	{PARSED_INTEREST, "ParsedInterest_ccn_data"},
	{PKEY, "PKEY_ccn_data"},
	{SIGNATURE, "Signature_ccn_data"},
	{SIGNED_INFO, "SignedInfo_ccn_data"},
	{SIGNING_PARAMS, "SigningParams_ccn_data"},
	{UPCALL_INFO, "UpcallInfo_ccn_data"},
	{0, NULL}
};

static inline const char *
type2name(enum _pyccn_capsules type)
{
	struct type_to_name *p;

	assert(type > 0);
	assert(type < sizeof(g_types_to_names) / sizeof(struct type_to_name));


	p = &g_types_to_names[type - g_types_to_names[0].type];
	assert(p->type == type);
	return p->name;
}

static inline enum _pyccn_capsules
name2type(const char *name)
{
	struct type_to_name *p;

	assert(name);

	for (p = g_types_to_names; p->type; p++)
		if (!strcmp(p->name, name))
			return p->type;

	debug("name = %s", name);
	panic("Got unknown type name");

	return 0; /* this shouldn't be reached */
}

static void
pyccn_Capsule_Destructor(PyObject *capsule)
{
	const char *name;
	void *pointer;
	enum _pyccn_capsules type;

	assert(PyCapsule_CheckExact(capsule));

	name = PyCapsule_GetName(capsule);
	type = name2type(name);

	pointer = PyCapsule_GetPointer(capsule, name);
	assert(pointer);

	switch (type) {
	case CLOSURE:
	{
		PyObject *py_obj_closure;
		struct ccn_closure *p = pointer;

		py_obj_closure = PyCapsule_GetContext(capsule);
		assert(py_obj_closure);
		Py_DECREF(py_obj_closure); /* No longer referencing Closure object */

		/* If we store something else, than ourselves, it probably is a bug */
		assert(capsule == p->data);

		free(p);
	}
		break;
	case CONTENT_OBJECT_COMPONENTS:
	{
		struct ccn_indexbuf *p = pointer;
		ccn_indexbuf_destroy(&p);
	}
		break;
	case HANDLE:
	{
		struct ccn *p = pointer;
		ccn_disconnect(p);
		ccn_destroy(&p);
	}
		break;
	case PARSED_CONTENT_OBJECT:
	case PARSED_INTEREST:
		free(pointer);
		break;
	case PKEY:
	{
		struct ccn_pkey *p = pointer;
		ccn_pubkey_free(p); // what about private keys?
	}
		break;
	case CONTENT_OBJECT:
	case EXCLUSION_FILTER:
	case INTEREST:
	case KEY_LOCATOR:
	case NAME:
	case SIGNATURE:
	case SIGNED_INFO:
	{
		struct ccn_charbuf *p = pointer;
		ccn_charbuf_destroy(&p);
	}
		break;
	case SIGNING_PARAMS:
	{
		struct ccn_signing_params *p = pointer;

		if (p->template_ccnb)
			ccn_charbuf_destroy(&p->template_ccnb);

		free(p);
	}
		break;
	case UPCALL_INFO:
		panic("For now we shouldn't use UPCALL_INFO at all");
	default:
		debug("Got capsule: %s\n", PyCapsule_GetName(capsule));
		panic("Unable to destroy the object: got an unknown capsule");
	}
}

PyObject *
CCNObject_New(enum _pyccn_capsules type, void *pointer)
{
	PyObject *r;

	assert(pointer);
	r = PyCapsule_New(pointer, type2name(type), pyccn_Capsule_Destructor);

	return r;
}

PyObject *
CCNObject_Borrow(enum _pyccn_capsules type, void *pointer)
{
	PyObject *r;

	assert(pointer);
	r = PyCapsule_New(pointer, type2name(type), NULL);
	assert(r);

	return r;
}

int
CCNObject_ReqType(enum _pyccn_capsules type, PyObject *capsule)
{
	int r;
	const char *t = type2name(type);

	r = PyCapsule_IsValid(capsule, t);
	if (!r)
		PyErr_Format(PyExc_TypeError, "Argument needs to be of %s type", t);

	return r;
}

int
CCNObject_IsValid(enum _pyccn_capsules type, PyObject *capsule)
{
	return PyCapsule_IsValid(capsule, type2name(type));
}

void *
CCNObject_Get(enum _pyccn_capsules type, PyObject *capsule)
{
	void *p;

	assert(CCNObject_IsValid(type, capsule));
	p = PyCapsule_GetPointer(capsule, type2name(type));
	assert(p);

	return p;
}

PyObject *
CCNObject_New_Closure(struct ccn_closure **closure)
{
	struct ccn_closure *p;
	PyObject *result;

	p = calloc(1, sizeof(*p));
	if (!p)
		return PyErr_NoMemory();

	result = CCNObject_New(CLOSURE, p);
	if (!result) {
		free(p);
		return NULL;
	}

	if (closure)
		*closure = p;

	return result;
}

PyObject *
CCNObject_New_ParsedContentObject(struct ccn_parsed_ContentObject **pco)
{
	struct ccn_parsed_ContentObject *p;
	PyObject *py_o;

	p = malloc(sizeof(*p));
	if (!p)
		return PyErr_NoMemory();

	py_o = CCNObject_New(PARSED_CONTENT_OBJECT, p);
	if (!py_o) {
		free(p);
		return NULL;
	}

	if (pco)
		*pco = p;

	return py_o;
}

PyObject *
CCNObject_New_ContentObjectComponents(struct ccn_indexbuf **comps)
{
	struct ccn_indexbuf *p;
	PyObject *py_o;

	p = ccn_indexbuf_create();
	if (!p)
		return PyErr_NoMemory();

	py_o = CCNObject_New(CONTENT_OBJECT_COMPONENTS, p);
	if (!py_o) {
		ccn_indexbuf_destroy(&p);
		return NULL;
	}

	if (comps)
		*comps = p;

	return py_o;
}

PyObject *
CCNObject_New_charbuf(enum _pyccn_capsules type,
		struct ccn_charbuf **charbuf)
{
	struct ccn_charbuf *p;
	PyObject *py_o;

	assert(type == CONTENT_OBJECT ||
			type == EXCLUSION_FILTER ||
			type == INTEREST ||
			type == KEY_LOCATOR ||
			type == NAME ||
			type == SIGNATURE ||
			type == SIGNED_INFO);

	p = ccn_charbuf_create();
	if (!p)
		return PyErr_NoMemory();

	py_o = CCNObject_New(type, p);
	if (!py_o) {
		ccn_charbuf_destroy(&p);
		return NULL;
	}

	if (charbuf)
		*charbuf = p;

	return py_o;
}

#if 0

void
CCNObject_Complete_Closure(PyObject *py_closure)
{
	struct completed_closure *p;

	debug("Adding called closure to be purged\n");

	assert(py_closure);
	p = malloc(sizeof(*p));
	p->closure = py_closure;
	p->next = g_completed_closures;
	g_completed_closures = p;
}

void
CCNObject_Purge_Closures()
{
	struct completed_closure *p;

	debug("Purging old closures\n");

	while (g_completed_closures) {
		p = g_completed_closures;
		Py_DECREF(p->closure);
		g_completed_closures = p->next;
		free(p);
	}
}
#endif
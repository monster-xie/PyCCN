/*
 * Copyright (c) 2011, Regents of the University of California
 * BSD license, See the COPYING file for more information
 * Written by: Derek Kulinski <takeda@takeda.tk>
 *             Jeff Burke <jburke@ucla.edu>
 */

#ifndef METHODS_NAME_H
#  define	METHODS_NAME_H

#  define NAME_TYPE_NORMAL 0
#  define NAME_TYPE_ANY 1

PyObject *_pyccn_Name_to_ccn(PyObject *self, PyObject *py_name_components);
PyObject *_pyccn_Name_from_ccn(PyObject *self, PyObject *py_cname);
PyObject *Name_obj_from_ccn(PyObject *ccn_data);
PyObject *Name_obj_to_ccn(PyObject *py_name);
PyObject *Name_from_ccn_tagged_bytearray(const unsigned char *buf,
		size_t size);
PyObject *_pyccn_name_from_uri(PyObject *self, PyObject *py_uri);
PyObject *_pyccn_name_to_uri(PyObject *self, PyObject *py_name);
PyObject *_pyccn_compare_names(PyObject *self, PyObject *args);

#endif	/* METHODS_NAME_H */


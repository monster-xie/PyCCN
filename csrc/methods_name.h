/*
 * File:   methods_name.h
 * Author: takeda
 *
 * Created on August 8, 2011, 2:42 AM
 */

#ifndef METHODS_NAME_H
#  define	METHODS_NAME_H

PyObject *_pyccn_Name_to_ccn(PyObject *self, PyObject *py_name_components);
PyObject *_pyccn_Name_from_ccn(PyObject *self, PyObject *py_cname);
PyObject *Name_from_ccn_parsed(struct ccn_charbuf *content_object,
		struct ccn_parsed_ContentObject *parsed_content_object);
PyObject *Name_from_ccn(PyObject *ccn_data);
struct ccn_charbuf *Name_to_ccn(PyObject *py_name);
PyObject *Name_from_ccn_tagged_bytearray(const unsigned char *buf,
		size_t size);

#endif	/* METHODS_NAME_H */

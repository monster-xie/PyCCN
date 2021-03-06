#
# Copyright (c) 2011, Regents of the University of California
# BSD license, See the COPYING file for more information
# Written by: Derek Kulinski <takeda@takeda.tk>
#             Jeff Burke <jburke@ucla.edu>
#

# Fronts ccn_pkey.
from . import _pyccn
from . import Name

class Key(object):
	def __init__(self):
		self.type = None
		self.publicKeyID = None # SHA256 hash
		# pyccn
		self.ccn_data_dirty = False
		self.ccn_data_public = None  # backing pkey
		self.ccn_data_private = None # backing pkey

	def __get_ccn(self):
		pass

	def generateRSA(self, numbits):
		_pyccn.generate_RSA_key(self, numbits)

	def privateToDER(self):
		if not self.ccn_data_private:
			raise _pyccn.CCNKeyError("Key is not private")
		return _pyccn.DER_write_key(self.ccn_data_private)

	def publicToDER(self):
		return _pyccn.DER_write_key(self.ccn_data_public)

	def privateToPEM(self, filename = None):
		if not self.ccn_data_private:
			raise _pyccn.CCNKeyError("Key is not private")

		if filename:
			f = open(filename, 'w')
			_pyccn.PEM_write_key(self.ccn_data_private, file=f)
			f.close()
		else:
			return _pyccn.PEM_write_key(self.ccn_data_private)

	def publicToPEM(self, filename = None):
		if filename:
			f = open(filename, 'w')
			_pyccn.PEM_write_key(self.ccn_data_public, file=f)
			f.close()
		else:
			return _pyccn.PEM_write_key(self.ccn_data_public)

	def fromDER(self, private = None, public = None):
		if private:
			(self.ccn_data_private, self.ccn_data_public, self.publicKeyID) = \
				_pyccn.DER_read_key(private=private)
			return
		if public:
			(self.ccn_data_private, self.ccn_data_public, self.publicKeyID) = \
				_pyccn.DER_read_key(public=public)
			return

	def fromPEM(self, filename = None, private = None, public = None):
		if filename:
			f = open(filename, 'r')
			(self.ccn_data_private, self.ccn_data_public, self.publicKeyID) = \
				_pyccn.PEM_read_key(file=f)
			f.close()
		elif private:
			(self.ccn_data_private, self.ccn_data_public, self.publicKeyID) = \
				_pyccn.PEM_read_key(private=private)
		elif public:
			(self.ccn_data_private, self.ccn_data_public, self.publicKeyID) = \
				_pyccn.PEM_read_key(public=public)

# plus library helper functions to generate and serialize keys?

class KeyLocator(object):
	def __init__(self, arg=None):
		#whichever one is not none will be used
		#if multiple set, checking order is: keyName, key, certificate
		self.key = arg if type(arg) is Key else None
		self.keyName = arg if type(arg) is Name.Name else None
		self.certificate = None

		# pyccn
		self.ccn_data_dirty = True
		self.ccn_data = None  # backing charbuf

	def __setattr__(self, name, value):
		if name != "ccn_data" and name != "ccn_data_dirty":
			self.ccn_data_dirty = True
		object.__setattr__(self, name, value)

	def __getattribute__(self, name):
		if name=="ccn_data":
			if object.__getattribute__(self, 'ccn_data_dirty'):
				if object.__getattribute__(self, 'keyName'):
					self.ccn_data = _pyccn.KeyLocator_to_ccn(
						name=self.keyName.ccn_data)
				elif object.__getattribute__(self, 'key'):
					self.ccn_data = _pyccn.KeyLocator_to_ccn(
						key=self.key.ccn_data_public)
				elif object.__getattribute__(self, 'certificate'):
					#same but with cert= arg
					raise NotImplementedError("certificate support is not implemented")
				else:
					raise TypeError("No name, key nor certificate defined")

				self.ccn_data_dirty = False
		return object.__getattribute__(self, name)
